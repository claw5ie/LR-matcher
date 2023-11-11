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
    index += (symbol_at_dot() != SYMBOL_END);

    return Item{
      .rule = rule,
      .dot_index = index,
    };
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
  std::list<Action> actions;
  StateId id;
  uint8_t flags = 0;
};

using ParsingTable = std::list<State>;

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
          if (it->symbol_at_dot() == SYMBOL_END)
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
              while (it != itemset.end() && it->symbol_at_dot() == SYMBOL_END);
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

bool
matches(ParsingTable &table, std::string_view string)
{
  auto symbols = std::vector<SymbolType>{ };
  auto trace = std::stack<State *>{ };
  auto state = &table.front();

  symbols.resize(string.size() + 1);
  symbols[0] = SYMBOL_END;
  std::copy(string.rbegin(), string.rend(), symbols.begin() + 1);

  do
    {
      // TODO: need to check for reduce/reduce conflicts.
      assert((state->flags & State::HAS_SHIFT_REDUCE) != State::HAS_SHIFT_REDUCE);

      if (state->flags & State::HAS_REDUCE)
        {
          auto action = find_action(Action::Reduce, state->actions);
          auto &rule = *action->as.reduce.to_rule;

          // Account for first symbol (variable definition) and last symbol (null terminator).
          for (size_t i = rule.size() - 1 - 1; i-- > 1; )
            trace.pop();

          auto backtrack_state = trace.top();
          trace.pop();

          symbols.push_back(rule[0]);
          state = backtrack_state;
        }
      else
        {
          auto symbol = symbols.back();
          symbols.pop_back();

          if (symbol == SYMBOL_END)
            return false;
          else if (symbol == FIRST_RESERVED_SYMBOL)
            return trace.empty() && symbols.back() == SYMBOL_END;
          else
            {
              auto action = find_action(Action::Shift, state->actions, symbol);
              if (action == nullptr)
                return false;

              trace.push(state);
              state = action->as.shift.item;
            }
        }
    }
  while (true);

  return false;
}
