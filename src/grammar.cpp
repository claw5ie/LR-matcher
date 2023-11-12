using TerminalType = char;
using SymbolType = uint32_t;

static_assert(sizeof(TerminalType) < sizeof(SymbolType));

constexpr SymbolType FIRST_RESERVED_SYMBOL = 1 << (sizeof(TerminalType) * CHAR_BIT);
constexpr SymbolType FIRST_USER_SYMBOL = FIRST_RESERVED_SYMBOL + 1;
constexpr SymbolType SYMBOL_END = 0;

struct Grammar
{
  // Rule stores the index of a symbol that is being defined in the beginning of the vector. In that way all rules that define the same symbol are consecutive in set.
  using Rule = std::vector<SymbolType>;

  std::set<Rule> rules;
  std::vector<std::string> lookup;

  std::string &grab_variable_name(SymbolType index)
  {
    assert(index >= FIRST_RESERVED_SYMBOL);
    return lookup[index - FIRST_RESERVED_SYMBOL];
  }

  std::set<Rule>::iterator find_first_rule(SymbolType symbol)
  {
    return rules.lower_bound({ symbol });
  }
};

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

  auto t = Tokenizer{
    .source = string,
  };
  auto g = Grammar{ };
  auto variables = VariableTable{ };
  auto next_symbol_index = FIRST_USER_SYMBOL;
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

        auto info = VariableInfo{
          .line_info = token.line_info,
          .index = next_symbol_index,
          .is_defined = true,
        };
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
                    auto info = VariableInfo{
                      .line_info = token.line_info,
                      .index = next_symbol_index,
                      .is_defined = false,
                    };
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
  g.lookup[0] = "<start>";

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
      auto new_name = std::string{ };
      new_name.reserve(name.size() + 2);
      new_name.push_back('<');
      new_name.append(name);
      new_name.push_back('>');
      variable_name = std::move(new_name);
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
