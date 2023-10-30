Grammar
parse_context_free_grammar(const char *string)
{
  struct VariableInfo
  {
    LineInfo line_info;
    SymbolType index;
    bool is_resolved;
  };

  Tokenizer t = { };
  t.source = string;

  Grammar g = { };
  std::map<std::string_view, VariableInfo> variables;

  auto next_symbol_index = FIRST_USER_SYMBOL_INDEX;
  auto failed_to_parse = false;

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

        VariableInfo info;
        info.line_info = token.line_info;
        info.index = next_symbol_index;
        info.is_resolved = true;

        auto [it, was_inserted] = variables.emplace(token.text, info);
        auto &[key, value] = *it;
        next_symbol_index += was_inserted;

        variable_definition_index = value.index;
        value.is_resolved = true;
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
          Grammar::Rule rule = { };
          rule.push_back(variable_definition_index);

          do
            {
              auto tt = t.peek();
              switch (tt)
                {
                case Token_Variable:
                  {
                    auto token = t.grab();
                    VariableInfo info;
                    info.line_info = token.line_info;
                    info.index = next_symbol_index;
                    info.is_resolved = false;

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
      FIRST_RESERVED_SYMBOL_INDEX,
      FIRST_USER_SYMBOL_INDEX,
      SYMBOL_END,
    });
  g.lookup[0] = "start";

  for (auto &[name, variable]: variables)
    {
      if (!variable.is_resolved)
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

  // Another exit if there are unresolved symbols.
  if (failed_to_parse)
    exit(EXIT_FAILURE);

  return g;
}

bool
is_variable(SymbolType symbol)
{
  return symbol >= FIRST_RESERVED_SYMBOL_INDEX;
}

std::string
rule_to_string(Grammar &grammar,
               const Grammar::Rule &rule)
{
  assert(rule.size() > 0 && is_variable(rule[0]));

  std::string result = grammar.grab_variable_name(rule[0]);
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
ItemIsLess::operator()(const Item &left,
                       const Item &right) const
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
compute_closure(Grammar &grammar,
                ItemSet &itemset)
{
  using Iterator = typename std::set<SymbolType>::iterator;

  // Keep track of order in which elements were inserted.
  std::set<SymbolType> to_visit = { };
  std::list<Iterator> order = { };

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

  ParsingTable table = { };
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
    Item item;
    item.rule = (Grammar::Rule *)&(*grammar.rules.begin());
    item.dot_index = 1;

    State state = { };
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
                  Action action = { };
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

              State new_state = { };
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
              Action action;

              if (is_variable(shift_symbol))
                {
                  action.type = Action_Go_To;
                  action.as.go_to = { shift_symbol, where_to_transition };
                }
              else
                {
                  action.type = Action_Shift;
                  action.as.shift = { (TerminalType)shift_symbol, where_to_transition };
                }

              state.actions.push_back(action);
            }
        }
    }

  return table;
}
