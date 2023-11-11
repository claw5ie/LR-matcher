Grammar
parse_context_free_grammar(const char *string)
{
  struct VariableInfo
  {
    LineInfo line_info;
    SymbolType index;
    bool is_defined;
  };

  using VariableTable = std::map<std::string_view, VariableInfo>;

  auto t = Tokenizer{ };
  auto g = Grammar{ };
  auto variables = VariableTable{ };
  auto next_symbol_index = FIRST_USER_SYMBOL;
  auto failed_to_parse = false;

  t.source = string;

  do
    {
      if (t.peek() != Token_Variable)
        {
          failed_to_parse = true;

          auto line_info = t.grab().line_info;
          t.report_error_start(line_info);
          t.report_error_print("expected a variable to start production");
          t.report_error_end();

          // Skip to next production.
          if (t.peek(1) != Token_Colon)
            {
              t.skip_to_next_semicolon();
              if (t.peek() == Token_Semicolon)
                t.advance();
              continue;
            }
        }

      SymbolType variable_definition_index = 0;

      {
        auto token = t.grab();
        t.advance();

        auto info = VariableInfo{ };
        info.line_info = token.line_info;
        info.index = next_symbol_index;
        info.is_defined = true;

        auto [it, was_inserted] = variables.emplace(token.text, info);
        auto &[key, value] = *it;
        next_symbol_index += was_inserted;

        variable_definition_index = value.index;
        value.is_defined = true;
      }

      if (!t.expect(Token_Colon))
        {
          failed_to_parse = true;

          auto token = t.grab();
          t.report_error_start(token.line_info);
          t.report_error_print("expected ':' before '");
          t.report_error_print(token.text);
          t.report_error_print("'");
          t.report_error_end();
        }

      do
        {
          auto rule = Grammar::Rule{ };
          rule.push_back(variable_definition_index);

          do
            {
              auto tt = t.peek();
              switch (tt)
                {
                case Token_Variable:
                  {
                    auto token = t.grab();
                    auto info = VariableInfo{ };
                    info.line_info = token.line_info;
                    info.index = next_symbol_index;
                    info.is_defined = false;

                    auto [it, was_inserted] = variables.emplace(token.text, info);
                    auto &[key, value] = *it;
                    next_symbol_index += was_inserted;

                    rule.push_back(value.index);
                  }

                  break;
                case Token_Terminals_Sequence:
                  {
                    auto text = t.grab().text;
                    for (size_t i = 0; i < text.size(); i++)
                      {
                        i += (text[i] == '\\');
                        rule.push_back(text[i]);
                      }
                  }

                  break;
                case Token_Colon:
                  {
                    failed_to_parse = true;

                    auto line_info = t.grab().line_info;
                    t.report_error_start(line_info);
                    t.report_error_print("expected variable or terminal, not colon (':')");
                    t.report_error_end();
                    t.skip_to_next_semicolon();
                  }

                  [[fallthrough]];
                default:
                  goto finish_parsing_sequence_of_terminals_and_variables;
                }

              t.advance();
              tt = t.peek();
            }
          while (true);
        finish_parsing_sequence_of_terminals_and_variables:

          rule.push_back(SYMBOL_END);
          g.rules.insert(std::move(rule));
        }
      while (t.expect(Token_Bar));

      if (t.peek() == Token_Semicolon)
        t.advance();
    }
  while (t.peek() != Token_End_Of_File);

  if (failed_to_parse)
    exit(EXIT_FAILURE);

  g.lookup.resize(variables.size() + 1);
  g.rules.insert({
      FIRST_RESERVED_SYMBOL,
      FIRST_USER_SYMBOL,
      SYMBOL_END,
    });
  g.lookup[0] = "start";

  for (auto &[name, variable]: variables)
    {
      if (!variable.is_defined)
        {
          failed_to_parse = true;
          t.report_error_start(variable.line_info);
          t.report_error_print("variable '");
          t.report_error_print(name);
          t.report_error_print("' is not defined");
          t.report_error_end();
        }

      auto &variable_name = g.grab_variable_name(variable.index);
      variable_name = name;
    }

  // Another exit if there are not defined symbols.
  if (failed_to_parse)
    exit(EXIT_FAILURE);

  return g;
}

bool
is_variable(SymbolType symbol)
{
  return symbol >= FIRST_RESERVED_SYMBOL;
}

std::string
rule_to_string(Grammar &grammar, const Grammar::Rule &rule)
{
  assert(rule.size() > 0 && is_variable(rule[0]));

  auto result = grammar.grab_variable_name(rule[0]);
  result.push_back(':');
  result.push_back(' ');

  for (size_t i = 1; i + 1 < rule.size(); i++)
    {
      auto symbol = rule[i];
      if (is_variable(symbol))
        result.append(grammar.grab_variable_name(symbol));
      else
        result.push_back((TerminalType)symbol);
    }

  return result;
}

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
          Item item;
          item.rule = (Grammar::Rule *)&(*it);
          item.dot_index = 1;
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
  auto next_state_id = StateId{ 0 };

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
    auto item = Item{ };
    item.rule = (Grammar::Rule *)&(*grammar.rules.begin());
    item.dot_index = 1;

    auto state = State{ };
    state.id = next_state_id;
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
                  auto action = Action{ };
                  action.type = Action_Reduce;
                  action.as.reduce = { it->rule };
                  actions.push_back(action);
                  it++;
                }
              while (it != itemset.end() && it->symbol_at_dot() == SYMBOL_END);
            }

          while (it != itemset.end())
            {
              auto shift_symbol = it->symbol_at_dot();
              auto new_state = State{ };
              new_state.id = next_state_id;

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
              auto action = Action{ };
              action.type = Action_Shift;
              action.as.shift = { shift_symbol, where_to_transition };
              state.actions.push_back(action);
            }
        }
    }

  return table;
}

Action &
find_action(ActionType type, std::list<Action> &actions)
{
  for (auto &action: actions)
    if (action.type == type)
      return action;

  assert(false);
}

Action *
find_action(ActionType type, std::list<Action> &actions, SymbolType symbol)
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
          auto &action = find_action(Action_Reduce, state->actions);
          auto &rule = *action.as.reduce.to_rule;

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
              auto action = find_action(Action_Shift, state->actions, symbol);
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
