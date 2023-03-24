void
advance_line_info(Grammar &g, char ch)
{
  ++g.column;
  if (ch == '\n')
    {
      ++g.line;
      g.column = 1;
    }
}

void
tokenize(Grammar &g, const char *at)
{
  g.line = 1;
  g.column = 1;

  bool failed_to_tokenize = false;

  while (*at != '\0')
    {
      while (isspace(*at))
        advance_line_info(g, *at++);

      Token token;
      token.text.data = at;
      token.text.count = 0;
      token.line = g.line;
      token.column = g.column;

      if (*at == '\0')
        break;
      else if (isupper(*at))
        {
          do
            advance_line_info(g, *at++);
          while (isalnum(*at)
                 || *at == '\''
                 || *at == '-'
                 || *at == '_');

          token.type = Token_Variable;
          token.text.count = at - token.text.data;
        }
      else if (*at == ':')
        {
          token.type = Token_Colon;
          token.text.count = 1;
          advance_line_info(g, *at++);
        }
      else if (*at == ';')
        {
          token.type = Token_Semicolon;
          token.text.count = 1;
          advance_line_info(g, *at++);
        }
      else if (*at == '|')
        {
          token.type = Token_Bar;
          token.text.count = 1;
          advance_line_info(g, *at++);
        }
      else
        {
          auto const is_escape_char =
            [](char ch) -> bool
            {
              return isupper(ch)
                || ch == ':'
                || ch == ';'
                || ch == '|'
                || ch == ' ';
            };

          while (!is_escape_char(*at) && *at != '\0')
            {
              if (*at == '\\')
                {
                  advance_line_info(g, *at++);

                  if (!is_escape_char(*at) && *at != '\\')
                    {
                      cerr << g.line << ':'
                           << g.column << ": error: invalid escape sequence \'\\"
                           << *at
                           << "\'.\n";
                      failed_to_tokenize = true;
                    }
                }

              advance_line_info(g, *at++);
            }

          token.type = Token_Term_Seq;
          token.text.count = at - token.text.data;
        }

      g.tokens.push_back(token);
    }

  if (failed_to_tokenize)
    exit(EXIT_FAILURE);

  Token token;
  token.type = Token_End_Of_File;
  token.text.data = at;
  token.text.count = 0;
  token.line = g.line;
  token.column = g.column;
  g.tokens.push_back(token);
}

size_t
peek_idx(Grammar &g)
{
  assert(g.current < g.tokens.size());
  return g.current;
}

TokenType
peek(Grammar &g, size_t idx = 0)
{
  return g.tokens[g.current + idx].type;
}

Token &
grab_prev_token(Grammar &g)
{
  return g.tokens[g.current];
}

Token &
grab_token(Grammar &g, size_t idx)
{
  return g.tokens[idx];
}

bool
expect_token_is(Grammar &g, TokenType expected)
{
  if (peek(g) != expected)
    return false;
  ++g.current;
  return true;
}

string &
lookup(Grammar &g, size_t index)
{
  return g.lookup[index - LOWEST_VAR_IDX];
}

const string &
lookup(const Grammar &g, size_t index)
{
  return g.lookup[index - LOWEST_VAR_IDX];
}

Grammar
parse_grammar(const char *str)
{
  struct Variable
  {
    size_t token_idx;
    size_t var_idx;
    bool is_resolved;
  };

  Grammar g = { };
  map<View<char>, Variable> vars;
  // "MIN_VAR_INDEX" is reserved for starting variable.
  sym_t last_unused_var_idx = LOWEST_VAR_IDX + 1;
  bool failed_to_parse = false;

  tokenize(g, str);

  while (peek(g) != Token_End_Of_File)
    {
      if (peek(g) != Token_Variable)
        {
          {
            failed_to_parse = true;
            auto token = grab_prev_token(g);
            cerr << token.line << ':'
                 << token.column << ": error: expected a variable to start production.\n";
          }

          if (peek(g, 1) != Token_Colon)
            {
              auto tt = peek(g);
              while (tt != Token_Semicolon
                     && tt != Token_End_Of_File)
                {
                  g.advance();
                  tt = peek(g);
                }

              if (tt != Token_End_Of_File)
                g.advance();

              continue;
            }
        }

      size_t curr_var_index = 0;

      {
        Variable variable;
        variable.token_idx = peek_idx(g);
        variable.var_idx = last_unused_var_idx;
        variable.is_resolved = true;

        View<char> name = g.tokens[variable.token_idx].text;
        auto it = vars.emplace(name, variable);

        curr_var_index = it.first->second.var_idx;
        it.first->second.is_resolved = true;
        last_unused_var_idx += it.second;
      }

      g.advance();

      if (!expect_token_is(g, Token_Colon))
        {
          auto token = grab_prev_token(g);
          cerr << token.line << ':'
               << token.column << ": error: expected \':\' before \'"
               << token.text
               << "\'.\n";
          failed_to_parse = true;
        }

      do
        {
          vector<sym_t> rule;
          rule.push_back(curr_var_index);

          auto tt = peek(g);
          while (tt == Token_Variable
                 || tt == Token_Term_Seq)
            {
              if (tt == Token_Variable)
                {
                  Variable variable;
                  variable.token_idx = peek_idx(g);
                  variable.var_idx = last_unused_var_idx;
                  variable.is_resolved = false;

                  View<char> name
                    = g.tokens[variable.token_idx].text;
                  auto it = vars.emplace(name, variable);

                  rule.push_back(it.first->second.var_idx);
                  last_unused_var_idx += it.second;
                }
              else
                {
                  auto text = grab_prev_token(g).text;
                  for (size_t i = 0; i < text.count; i++)
                    {
                      if (text.data[i] == '\\')
                        rule.push_back(text.data[++i]);
                      else
                        rule.push_back(text.data[i]);
                    }
                }

              g.advance();
              tt = peek(g);
            }

          rule.push_back(0);
          g.rules.insert(move(rule));

          // Should I check if next token is colon and report an error??
        }
      while (expect_token_is(g, Token_Bar));

      if (peek(g) == Token_Semicolon)
        g.advance();
    }

  g.lookup.resize(vars.size() + 1);
  g.rules.insert({ LOWEST_VAR_IDX, LOWEST_VAR_IDX + 1, 0 });
  g.lookup[0] = "start";

  for (auto &elem: vars)
    {
      if (!elem.second.is_resolved)
        {
          auto token = grab_token(g, elem.second.token_idx);
          cerr << token.line << ':'
               << token.column << ": error: unresolved variable \'"
               << token.text
               << "\'.\n";
          failed_to_parse = true;
        }

      lookup(g, elem.second.var_idx) = view_to_string(elem.first);
    }

  if (failed_to_parse)
    exit(EXIT_FAILURE);

  return g;
}

bool
is_variable(uint32_t symbol)
{
  return symbol >= LOWEST_VAR_IDX;
}

string
rule_to_string(const Grammar &grammar,
               const GrammarRule &rule)
{
  assert(rule.size() > 0 && is_variable(rule[0]));

  string res = lookup(grammar, rule[0]);

  res.push_back(':');
  res.push_back(' ');

  for (size_t i = 1; i + 1 < rule.size(); i++)
    {
      uint32_t const elem = rule[i];

      if (is_variable(elem))
        res.append(lookup(grammar, elem));
      else
        res.push_back((char)elem);
    }

  return res;
}

uint32_t
symbol_at_dot(const Item &item)
{
  return (*item.rule)[item.dot];
}

bool
are_items_different(const Item &left, const Item &right)
{
  return left.rule != right.rule || left.dot != right.dot;
}

bool
ItemIsLess::operator()(const Item &left,
                       const Item &right) const
{
  uint32_t const sym_left = symbol_at_dot(left);
  uint32_t const sym_right = symbol_at_dot(right);

  if (sym_left == sym_right)
    {
      if (left.dot == right.dot)
        return left.rule < right.rule;
      else
        return left.dot < right.dot;
    }
  else
    {
      return sym_left < sym_right;
    }
}

void
compute_closure(const Grammar &grammar,
                ItemSet &item_set)
{
  vector<uint32_t> to_visit;

  auto const should_visit
    = [](uint32_t symbol,
         const vector<uint32_t> &to_visit) -> bool
    {
      for (uint32_t elem: to_visit)
        if (elem == symbol)
          return false;

      return true;
    };

  for (const auto &item: item_set)
    {
      uint32_t const symbol = symbol_at_dot(item);
      if (is_variable(symbol) && should_visit(symbol, to_visit))
        to_visit.push_back(symbol);
    }

  for (size_t i = 0; i < to_visit.size(); i++)
    {
      uint32_t const symbol = to_visit[i];

      for (auto it = grammar.rules.lower_bound({ symbol });
           it != grammar.rules.end() && (*it)[0] == symbol;
           it++)
        {
          item_set.emplace(Item{ &(*it), 1 });

          uint32_t const new_candidate = (*it)[1];
          if (is_variable(new_candidate) && should_visit(new_candidate, to_visit))
            to_visit.push_back(new_candidate);
        }
    }
}

ParsingTable
find_item_sets(const Grammar &grammar)
{
  auto const shift_dot
    = [](const Item &item) -> Item
    {
      return { item.rule, item.dot + ((*item.rule)[item.dot] != 0) };
    };

  auto const are_item_sets_equal
    = [](const ItemSet &left, const ItemSet &right) -> bool
    {
      if (left.size() != right.size())
        return false;

      for (auto lit = left.begin(), rit = right.begin();
           lit != left.end();
           lit++, rit++)
        {
          if (are_items_different(*lit, *rit))
            return false;
        }

      return true;
    };

  if (grammar.rules.empty())
    return { };

  ParsingTable table;

  table.push_back({ { { &(*grammar.rules.begin()), 1 } }, { } });
  compute_closure(grammar, table[0].itemset);

  for (size_t i = 0; i < table.size(); i++)
    {
      auto *item_set = &table[i].itemset;

      for (auto it = item_set->begin(); it != item_set->end(); )
        {
          if (symbol_at_dot(*it) == 0)
            {
              auto &actions = table[i].actions;
              while (it != item_set->end() && symbol_at_dot(*it) == 0)
                {
                  actions.push_back({ Action_Reduce, 0, 0, it->rule });
                  it++;
                }
            }

          while (it != item_set->end())
            {
              uint32_t const shift_symbol = symbol_at_dot(*it);

              table.push_back({ { }, { } });
              // Update reference in case of reallocation.
              item_set = &table[i].itemset;

              while (it != item_set->end()
                     && symbol_at_dot(*it) == shift_symbol)
                {
                  table.back().itemset.insert(shift_dot(*it));
                  it++;
                }

              compute_closure(grammar, table.back().itemset);

              uint32_t where_to_transition = table.size() - 1;
              for (size_t j = 0; j + 1 < table.size(); j++)
                {
                  if (are_item_sets_equal(table[j].itemset, table.back().itemset))
                    {
                      table.pop_back();
                      where_to_transition = j;
                      break;
                    }
                }

              table[i].actions.push_back({ is_variable(shift_symbol) ? Action_Goto : Action_Shift,
                  shift_symbol,
                  where_to_transition,
                  nullptr });
            }
        }
    }

  return table;
}
