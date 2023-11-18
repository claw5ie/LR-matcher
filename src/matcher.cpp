struct Item
{
  Grammar::Rule *rule;
  size_t dot_index;

  SymbolType symbol_at_dot() const
  {
    return (*rule)[dot_index];
  }

  Item shift_dot() const
  {
    auto index = this->dot_index;
    index += (symbol_at_dot() != END_SYMBOL);

    return { .rule = rule,
             .dot_index = index, };
  }

  bool operator==(const Item &other) const
  {
    return this->dot_index == other.dot_index && this->rule == other.rule;
  }
};

struct ItemIsLess
{
  bool operator()(const Item &, const Item &) const;
};

using ItemSet = std::set<Item, ItemIsLess>;

struct State;

struct Action
{
  enum Type
    {
      Shift,
      Reduce,
    };

  Type type;

  union
  {
    struct
    {
      SymbolType label;
      State *item;
    } shift;

    struct
    {
      Grammar::Rule *to_rule;
    } reduce;
  } as;
};

using StateId = uint32_t;

struct State
{
  constexpr static uint8_t HAS_SHIFT = 0x1;
  constexpr static uint8_t HAS_REDUCE = 0x2;
  constexpr static uint8_t HAS_SHIFT_REDUCE = HAS_SHIFT | HAS_REDUCE;

  ItemSet itemset;
  // TODO: could seperate actions into two lists: one for shift and one for reduce actions. Also helps to check for shift/reduce and reduce/reduce conflicts.
  std::list<Action> actions;
  StateId id;
  uint8_t flags = 0;
};

using ParsingTable = std::list<State>;

Action *
find_action(Action::Type type, std::list<Action> &actions)
{
  for (auto &action: actions)
    if (action.type == type)
      return &action;

  assert(false);
}

Action *
find_action(Action::Type type, std::list<Action> &actions, SymbolType symbol)
{
  for (auto &action: actions)
    if (action.type == type && action.as.shift.label == symbol)
      return &action;

  return nullptr;
}

struct PDAStepResult
{
  enum Type
    {
      Reject,
      Accept,
      None,
    };

  Action *action;
  Type type;
};

struct PDAState
{
  State *state;
  SymbolType symbol;
};

// 'shift' and 'goto' operations are supposed to be separate, but in this implementation they are the same.
struct PDA
{
  Grammar *grammar;
  ParsingTable *table;

  std::stack<PDAState> stack = { };
  const char *to_match = "";
  size_t consumed = 0;
  State *state = nullptr;

  void reset(const char *string)
  {
    assert(grammar && table);

    to_match = string;
    auto empty = std::stack<PDAState>{ };
    stack.swap(empty); // Remove all elements.
    consumed = 0;
    state = &table->front();

    stack.push({
        .state = state,
        .symbol = 0,
      });
  }

  bool match(const char *string)
  {
    reset(string);

    do
      {
        auto [_, type] = step();
        switch (type)
          {
          case PDAStepResult::Reject: return false;
          case PDAStepResult::Accept: return true;
          case PDAStepResult::None:   break;
          }
      }
    while (true);
  }

  PDAStepResult step()
  {
    // TODO: need to check for reduce/reduce conflicts.
    assert((state->flags & State::HAS_SHIFT_REDUCE) != State::HAS_SHIFT_REDUCE);

    if (state->flags & State::HAS_REDUCE)
      {
        auto reduce_action = find_action(Action::Reduce, state->actions);
        auto &rule = *reduce_action->as.reduce.to_rule;
        auto symbol = rule[0];

        // Account for first symbol (variable definition) and last symbol (null terminator).
        for (size_t i = rule.size() - 1 - 1; i-- > 0; )
          stack.pop();

        state = stack.top().state;

        auto goto_action = find_action(Action::Shift, state->actions, symbol);
        assert(goto_action);
        state = goto_action->as.shift.item;

        stack.push({
            .state = state,
            .symbol = symbol,
          });

        return { .action = reduce_action,
                 .type = PDAStepResult::None, };
      }
    else
      {
        auto terminal = to_match[consumed++];
        auto action = find_action(Action::Shift, state->actions, terminal);

        if (action == nullptr)
          {
            return { .action = nullptr,
                     .type = PDAStepResult::Reject, };
          }
        else if (terminal == '\0')
          {
            return { .action = nullptr,
                     .type = PDAStepResult::Accept, };
          }
        else
          {
            state = action->as.shift.item;
            stack.push({
                .state = state,
                .symbol = terminal,
              });

            return { .action = action,
                     .type = PDAStepResult::None, };
          }
      }
  }

  void generate_automaton_steps_json(const char *string, const char *filepath)
  {
    reset(string);

    auto result = std::string{ };
    result.append("{\n    \"string\": \"");
    result.append(to_match);
    result.append("\",\n    \"actions\": [");

    auto first_run = true;

    do
      {
        if (!first_run)
          result.append(",\n                ");

        auto [action, type] = step();
        switch (type)
          {
          case PDAStepResult::Reject:
            {
              result.append("{ \"type\": \"finish\", \"result\": 0 }]\n}");
              goto finish;
            }
          case PDAStepResult::Accept:
            {
              result.append("{ \"type\": \"finish\", \"result\": 1 }]\n}");
              goto finish;
            }
          case PDAStepResult::None:
            switch (action->type)
              {
              case Action::Shift:
                {
                  result.append("{ \"type\": \"shift\" }");
                }

                break;
              case Action::Reduce:
                {
                  auto &rule = *action->as.reduce.to_rule;
                  result.append("{ \"type\": \"reduce\", \"to\": { \"symbol\": \"");
                  result.append(grammar->grab_variable_name(rule[0]));
                  result.append("\", \"size\": ");
                  result.append(std::to_string(rule.size() - 1 - 1));
                  result.append(" } }");
                }

                break;
              }

            break;
          }

        first_run = false;
      }
    while (true);
  finish:
    result.push_back('\n');

    auto file = std::ofstream{ filepath, std::ofstream::trunc };
    if (!file.is_open())
      {
        std::cerr << "error: failed to open '"
                  << filepath
                  << "'\n";
        exit(EXIT_FAILURE);
      }
    file.write(&result[0], result.size());
    file.close();
  }
};

bool
ItemIsLess::operator()(const Item &left, const Item &right) const
{
  auto lsymbol = left.symbol_at_dot();
  auto rsymbol = right.symbol_at_dot();

  if (lsymbol == rsymbol)
    {
      if (left.dot_index == right.dot_index)
        return left.rule < right.rule;
      else
        return left.dot_index < right.dot_index;
    }
  else
    {
      return lsymbol < rsymbol;
    }
}

void
compute_closure(Grammar &grammar, ItemSet &itemset)
{
  using Iterator = typename std::set<SymbolType>::iterator;

  // Keep track of order in which elements were inserted.
  auto to_visit = std::set<SymbolType>{ };
  auto order = std::list<Iterator>{ };

  auto const insert =
    [&to_visit, &order](SymbolType symbol) -> void
    {
      if (is_variable(symbol))
        {
          auto [it, was_inserted] = to_visit.insert(symbol);
          if (was_inserted)
            order.push_back(it);
        }
    };

  for (auto &item: itemset)
    insert(item.symbol_at_dot());

  for (auto &sit: order)
    {
      auto symbol = *sit;

      for (auto it = grammar.find_first_rule(symbol);
           it != grammar.rules.end() && (*it)[0] == symbol;
           it++)
        {
          auto item = Item{
            .rule = (Grammar::Rule *)&(*it),
            .dot_index = 1,
          };
          itemset.insert(item);
          insert(item.symbol_at_dot());
        }
    }
}

ParsingTable
compute_parsing_table(Grammar &grammar)
{
  assert(!grammar.rules.empty());

  auto table = ParsingTable{ };
  StateId next_state_id = 0;

  auto const insert =
    [&table, &next_state_id](State &&state) -> State *
    {
      State *state_ptr = nullptr;

      for (auto &self: table)
        {
          if (self.itemset == state.itemset)
            {
              state_ptr = &self;
              break;
            }
        }

      if (state_ptr == nullptr)
        {
          table.push_back(state);
          next_state_id++;
          state_ptr = &table.back();
        }

      return state_ptr;
    };

  {
    auto item = Item{
      .rule = (Grammar::Rule *)&(*grammar.rules.begin()),
      .dot_index = 1,
    };
    auto state = State{
      .itemset = { },
      .actions = { },
      .id = next_state_id,
    };
    state.itemset.insert(item);
    compute_closure(grammar, state.itemset);

    insert(std::move(state));
  }

  for (auto &state: table)
    {
      auto &itemset = state.itemset;
      auto it = itemset.begin();

      while (it != itemset.end())
        {
          if (it->symbol_at_dot() == END_SYMBOL)
            {
              auto &actions = state.actions;
              do
                {
                  state.flags |= State::HAS_REDUCE;
                  auto action = Action{
                    .type = Action::Reduce,
                    .as = { .reduce = {
                        .to_rule = it->rule,
                      } },
                  };
                  actions.push_back(action);
                  it++;
                }
              while (it != itemset.end() && it->symbol_at_dot() == END_SYMBOL);
            }

          while (it != itemset.end())
            {
              auto shift_symbol = it->symbol_at_dot();
              auto new_state = State{
                .itemset = { },
                .actions = { },
                .id = next_state_id,
              };

              do
                {
                  new_state.itemset.insert(it->shift_dot());
                  it++;
                }
              while (it != itemset.end()
                     && it->symbol_at_dot() == shift_symbol);

              compute_closure(grammar, new_state.itemset);

              auto where_to_transition = insert(std::move(new_state));

              state.flags |= State::HAS_SHIFT;
              auto action = Action{
                .type = Action::Shift,
                .as = { .shift = {
                    .label = shift_symbol,
                    .item = where_to_transition,
                  } },
              };
              state.actions.push_back(action);
            }
        }
    }

  return table;
}

void
generate_automaton_json(ParsingTable &table, const char *filepath)
{
  auto result = std::string{ };
  result.push_back('[');

  size_t i = 0;
  for (auto &state: table)
    {
      assert(state.id == i++);
      result.push_back('[');

      size_t j = 0;
      for (auto &action: state.actions)
        {
          if (action.type == Action::Shift)
            {
              result.append("{ \"label\": ");
              result.append(std::to_string(action.as.shift.label));
              result.append(", \"dst\": ");
              result.append(std::to_string(action.as.shift.item->id));
              result.append(" }");
            }

          if (++j < state.actions.size())
            result.append(", ");
        }

      result.push_back(']');

      if (i < table.size())
        result.append(", ");
    }

  result.append("]\n");

  auto file = std::ofstream{ filepath };
  file.write(&result[0], result.size());
  file.close();
}
