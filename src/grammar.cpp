using TerminalType = char;
using SymbolType = int32_t;

static_assert(sizeof(TerminalType) < sizeof(SymbolType));

constexpr SymbolType START_SYMBOL = 1 << (sizeof(TerminalType) * CHAR_BIT);
constexpr SymbolType FIRST_SYMBOL = START_SYMBOL + 1;
constexpr SymbolType END_SYMBOL = -1;

struct Grammar
{
  // Rule stores the index of a symbol that is being defined in the beginning of the vector. In that way all rules that define the same symbol are consecutive in set.
  using Rule = std::vector<SymbolType>;

  std::set<Rule> rules;
  std::vector<std::string> lookup;

  std::string &grab_variable_name(SymbolType index)
  {
    assert(index >= START_SYMBOL);
    return lookup[index - START_SYMBOL];
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
    .ctx = {
      .source = string,
    },
    .buffer_token = buffer_token_bnf,
  };
  auto g = Grammar{ };
  auto variables = VariableTable{ };
  auto next_symbol_index = FIRST_SYMBOL;
  auto failed_to_parse = false;

  do
    {
      if (t.peek() != Token::Variable)
        {
          failed_to_parse = true;

          auto line_info = t.grab().line_info;
          PRINT_ERROR0(line_info, "expected a variable to start production");

          // Skip to next production.
          if (t.peek(1) != Token::Define)
            {
              t.skip_to_next_delimiter();
              if (t.peek() == Token::Delimiter)
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

      if (!t.expect(Token::Define))
        {
          failed_to_parse = true;

          auto token = t.grab();
          PRINT_ERROR(token.line_info, "expected ':' or '::=' before '%.*s'", (int)token.text.size(), token.text.data());
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
                case Token::Variable:
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
                case Token::Terminals_Sequence:
                  {
                    auto text = t.grab().text;
                    for (size_t i = 0; i < text.size(); i++)
                      {
                        i += (text[i] == '\\');
                        rule.push_back(text[i]);
                      }
                  }

                  break;
                case Token::Define:
                  {
                    failed_to_parse = true;

                    auto line_info = t.grab().line_info;
                    PRINT_ERROR0(line_info, "expected variable or terminal");
                    t.skip_to_next_delimiter();
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

          rule.push_back(END_SYMBOL);
          g.rules.insert(std::move(rule));
        }
      while (t.expect(Token::Bar));

      if (t.peek() == Token::Delimiter)
        t.advance();
    }
  while (t.peek() != Token::End_Of_File);

  if (failed_to_parse)
    exit(EXIT_FAILURE);

  g.lookup.resize(variables.size() + 1);
  g.rules.insert({
      START_SYMBOL,
      FIRST_SYMBOL,
      '\0',
      END_SYMBOL,
    });
  g.lookup[0] = "<start>";

  for (auto &[name, variable]: variables)
    {
      if (!variable.is_defined)
        {
          failed_to_parse = true;
          PRINT_ERROR(variable.line_info, "variable '%.*s' is not defined", (int)name.size(), name.data());
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
  return symbol >= START_SYMBOL;
}
