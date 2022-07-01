#include <map>
#include <cassert>
#include "Grammar.hpp"

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
  char const *text;
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

  char const *const text = at;

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

Grammar parse_grammar(char const *str)
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

  for (auto const &elem: vars)
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
