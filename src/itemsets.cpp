#include "itemsets.hpp"
#include <map>
#include <cassert>

struct Token
{
  enum Type
  {
    Variable = 0,
    Term_Seq,

    Colon,
    Semicolon,
    Bar,

    End_Of_File,
    Count
  };

  Type type;
  const char *text;
  size_t size;
};

void assert_token_type(
  const Token &token,
  Token::Type type
  )
{
  assert(type < Token::Count);

  const char *lookup[Token::Count] = {
    "variable",
    "sequence of terminals",
    "`:`",
    "`;`",
    "`|`",
    "end of file",
  };

  if (token.type != type)
  {
    std::fprintf(
      stderr,
      "ERROR: unexpected token: I was expecting to see a %s.\n",
      lookup[type]
      );
    std::exit(EXIT_FAILURE);
  }
}

Token next_token(const char *&at)
{
  while (std::isspace(*at))
    ++at;

  const char *const text = at;

  at += (*at != '\0');

  switch (*text)
  {
  case ':':
    return { Token::Colon, text, 1 };
  case ';':
    return { Token::Semicolon, text, 1 };
  case '|':
    return { Token::Bar, text, 1 };
  case '\0':
    return { Token::End_Of_File, text, 1 };
  }

  if (std::isupper(*text))
  {
    auto const is_special_char =
      [](char ch) -> bool
      {
        return ch == '\'' || ch == '-' || ch == '_';
      };

    while (std::isalpha(*at) ||
           std::isdigit(*at) ||
           is_special_char(*at))
    {
      ++at;
    }

    return { Token::Variable, text, size_t(at - text) };
  }
  else
  {
    auto const is_escape_char =
      [](char ch) -> bool
      {
        return std::isupper(ch) ||
          ch == ':' ||
          ch == ';' ||
          ch == '|' ||
          ch == ' ';
      };

    // Undo the previous increment.
    --at;
    while (!is_escape_char(*at) && *at != '\0')
    {
      if (*at == '\\')
      {
        ++at;

        if (!is_escape_char(*at) && *at != '\\')
        {
          std::fprintf(
            stderr,
            "ERROR: invalid escape sequence `\\%c`.\n",
            *at
            );
          std::exit(EXIT_FAILURE);
        }
      }

      at++;
    }

    return { Token::Term_Seq, text, size_t(at - text) };
  }
}

Grammar parse_grammar(const char *str)
{
  struct VarInfo
  {
    uint32_t index;
    bool is_unresolved;
  };

  std::map<std::string, VarInfo> vars;
  Token token = next_token(str);
  // "MIN_VAR_INDEX" is reserved for additional variable.
  uint32_t next_var_index = MIN_VAR_INDEX + 1;

  Grammar grammar;

  while (token.type != Token::End_Of_File)
  {
    assert(token.type == Token::Variable);

    uint32_t curr_var_index;
    {
      auto it = vars.emplace(std::string(token.text, token.size),
                             VarInfo{ next_var_index, false });

      it.first->second.is_unresolved = false;
      next_var_index += it.second;
      curr_var_index = it.first->second.index;
    }

    token = next_token(str);
    assert(token.type == Token::Colon);

  insert_rule:
    token = next_token(str);
    std::vector<uint32_t> rule;
    rule.push_back(curr_var_index);

    while (token.type == Token::Variable ||
           token.type == Token::Term_Seq)
    {
      if (token.type == Token::Variable)
      {
        auto it = vars.emplace(std::string(token.text, token.size),
                               VarInfo{ next_var_index, true });

        rule.push_back(it.first->second.index);
        next_var_index += it.second;
      }
      else
      {
        for (size_t i = 0; i < token.size; i++)
        {
          if (token.text[i] == '\\')
            rule.push_back(token.text[++i]);
          else
            rule.push_back(token.text[i]);
        }
      }

      token = next_token(str);
    }

    rule.push_back(0);
    grammar.rules.insert(std::move(rule));

    switch (token.type)
    {
    case Token::Bar:
      goto insert_rule;
    case Token::Colon:
      std::fputs("ERROR: unexpected `:`.\n", stderr);
      std::exit(EXIT_FAILURE);
    default:
      ;
    }

    token = next_token(str);
  }

  grammar.lookup.resize(vars.size() + 1);
  grammar.rules.insert({ MIN_VAR_INDEX, MIN_VAR_INDEX + 1, 0 });
  grammar.lookup[0] = "start";

  for (const auto &elem: vars)
  {
    if (elem.second.is_unresolved)
    {
      std::fprintf(
        stderr,
        "ERROR: unresolved variable `%s`.\n",
        elem.first.c_str()
        );
      std::exit(EXIT_FAILURE);
    }

    grammar.lookup[elem.second.index - MIN_VAR_INDEX] =
      std::move(elem.first);
  }

  return grammar;
}

std::string rule_to_string(
  const Grammar &grammar, const Grammar::Rule &rule
  )
{
  assert(rule.size() > 0 && rule[0] >= MIN_VAR_INDEX);

  std::string res = grammar.lookup[rule[0] - MIN_VAR_INDEX];

  res.push_back(':');
  res.push_back(' ');

  for (size_t i = 1; i + 1 < rule.size(); i++)
  {
    uint32_t const elem = rule[i];

    if (elem >= MIN_VAR_INDEX)
      res.append(grammar.lookup[elem - MIN_VAR_INDEX]);
    else
      res.push_back((char)elem);
  }

  return res;
}

bool is_variable(uint32_t symbol)
{
  return symbol >= MIN_VAR_INDEX;
}

uint32_t symbol_at_dot(const Item &item)
{
  return (*item.rule)[item.dot];
}

bool are_items_different(const Item &left, const Item &right)
{
  return left.rule != right.rule || left.dot != right.dot;
}

bool ItemIsLess::operator()(
  const Item &left, const Item &right
  ) const
{
  uint32_t const sym_left = symbol_at_dot(left),
    sym_right = symbol_at_dot(right);

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

void closure(const Grammar &grammar, ItemSet &item_set)
{
  std::vector<uint32_t> to_visit;

  auto const should_visit =
    [](uint32_t symbol,
       const std::vector<uint32_t> &to_visit) -> bool
    {
      for (uint32_t elem: to_visit)
      {
        if (elem == symbol)
          return false;
      }

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

ParsingTable find_item_sets(const Grammar &grammar)
{
  auto const shift_dot =
    [](const Item &item) -> Item
    {
      return { item.rule, item.dot + ((*item.rule)[item.dot] != 0) };
    };

  auto const are_item_sets_equal =
    [](const ItemSet &left, const ItemSet &right) -> bool
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
  closure(grammar, table[0].itemset);

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
          actions.push_back({ Action::Reduce, 0, 0, it->rule });
          it++;
        }
      }

      while (it != item_set->end())
      {
        uint32_t const shift_symbol = symbol_at_dot(*it);

        table.push_back({ { }, { } });
        // Update reference in case of reallocation.
        item_set = &table[i].itemset;

        while (it != item_set->end() &&
               symbol_at_dot(*it) == shift_symbol)
        {
          table.back().itemset.insert(shift_dot(*it));
          it++;
        }

        closure(grammar, table.back().itemset);

        uint32_t where_to_transition = table.size() - 1;
        for (size_t j = 0; j + 1 < table.size(); j++)
        {
          if (are_item_sets_equal(
                table[j].itemset, table.back().itemset
                ))
          {
            table.pop_back();
            where_to_transition = j;
            break;
          }
        }

        table[i].actions.push_back(
          { is_variable(shift_symbol) ? Action::Goto : Action::Shift,
            shift_symbol,
            where_to_transition,
            nullptr }
          );
      }
    }
  }

  return table;
}
