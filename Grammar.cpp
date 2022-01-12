#include <map>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "Grammar.hpp"

struct Token
{
  enum
  {
    Variable = 0,
    TerminalSequence,
    Colon,
    Semicolon,
    Bar,
    Eof,
  } type;

  char const *begin;
  char const *end;

  void expect(decltype(type) token) const;

  char const *to_string() const;

private:
  static char const *const type_names[6];
};

void Token::expect(decltype(type) token) const
{
  if (this->type != token)
  {
    fprintf(
      stderr,
      "error: unexpected token. In this place should be token \"%s\".\n",
      type_names[token]
      );

    exit(EXIT_FAILURE);
  }
}

char const *Token::to_string() const
{
  return type_names[this->type];
}

char const *const Token::type_names[6] = {
  "variable",
  "terminal",
  ":",
  ";",
  "|",
  "end-of-file"
};

Token next_token(char const **str)
{
  while (isspace(**str))
    (*str)++;

  if (isupper(**str))
  {
    char const *const start = *str;

    auto const is_special_char =
      [](char ch) -> bool
      {
        return ch == '\'' || ch == '-' || ch == '_';
      };

    while (
      isalpha(**str) || isdigit(**str) || is_special_char(**str)
      )
    {
      (*str)++;
    }

    return { Token::Variable, start, *str };
  }
  else if (**str == ':')
  {
    (*str)++;

    return { Token::Colon, *str - 1, *str };
  }
  else if (**str == ';')
  {
    (*str)++;

    return { Token::Semicolon, *str - 1, *str };
  }
  else if (**str == '|')
  {
    (*str)++;

    return { Token::Bar, *str - 1, *str };
  }
  else if (**str == '\0')
  {
    return { Token::Eof, *str, *str };
  }
  else if (isprint(**str))
  {
    char const *const start = *str;

    auto const is_reserved =
      [](char ch) -> bool
      {
        return ch == ';' || ch == ':' || ch == '|' || ch == '\\' || ch == ' ';
      };

    while (
      (isprint(**str) &&
        !isupper(**str) &&
        !is_reserved(**str)) ||
      **str == '\\'
      )
    {
      if (**str == '\\')
      {
        char const next = *(*str + 1);
        if (!is_reserved(next) && !isupper(next))
        {
          fputs("error: invalid escape sequence.\n", stderr);
          exit(EXIT_FAILURE);
        }

        (*str)++;
      }

      (*str)++;
    }

    return { Token::TerminalSequence, start, *str };
  }
  else
  {
    fputs("error: non printable character in string.\n", stderr);

    (*str)++;

    exit(EXIT_FAILURE);

    return { Token::TerminalSequence, *str - 1, *str };
  }
}

struct TokenIsLess
{
  bool operator()(Token const &left, Token const &right) const;
};

bool TokenIsLess::operator()(Token const &left, Token const &right) const
{
  size_t const lend = left.end - left.begin,
    rend = right.end - right.begin;
  for (size_t i = 0; i < lend && i < rend; i++)
  {
    if (left.begin[i] != right.begin[i])
      return left.begin[i] < right.begin[i];
  }

  return lend < rend;
}

std::pair<Grammar, std::vector<std::string>> parse_grammar(char const *str)
{
  std::vector<Token> tokens; tokens.reserve(128);
  std::map<Token, int, TokenIsLess> vars_to_int;

  Token next = next_token(&str);

  for (int var_index = MIN_VAR_INDEX + 1; next.type != Token::Eof; )
  {
    if (next.type == Token::Colon)
    {
      if (!tokens.empty() && tokens.back().type == Token::Variable)
      {
        bool const was_inserted  =
          vars_to_int.emplace(tokens.back(), var_index).second;
        var_index += was_inserted;
      }
    }

    tokens.push_back(next);
    next = next_token(&str);
  }

  tokens.push_back(next);

  std::set<Rule> grammar;

  for (size_t i = 0; i < tokens.size(); i++)
  {
    tokens[i].expect(Token::Variable);
    auto const index = vars_to_int.find(tokens[i]);
    tokens[++i].expect(Token::Colon);

    do
    {
      i++;

      std::vector<int> rule; rule.reserve(32);
      rule.push_back(index->second);

      while (
        tokens[i].type == Token::Variable ||
        tokens[i].type == Token::TerminalSequence
        )
      {
        auto &token = tokens[i];

        if (token.type == Token::Variable)
        {
          auto const it = vars_to_int.find(token);

          if (it == vars_to_int.end())
          {
            fputs("error: unrecognized variable.\n", stderr);
            exit(EXIT_FAILURE);
          }

          rule.push_back(it->second);
        }
        else
        {
          for (char const *beg = token.begin; beg < token.end; beg++)
          {
            if (*beg == '\\')
              rule.push_back(*(++beg));
            else
              rule.push_back(*beg);
          }
        }

        i++;
      }

      rule.push_back(0);
      grammar.insert(std::move(rule));

    } while (tokens[i].type == Token::Bar);

    switch (tokens[i].type)
    {
    case Token::Semicolon:
    case Token::Eof:
      break;
    default:
      fputs(
        "error: expected semicolon or end-of-file to end the productions.\n",
        stderr
        );
      exit(EXIT_FAILURE);
      break;
    }
  }

  std::vector<std::string> vars_lookup;
  vars_lookup.resize(vars_to_int.size() + 1);

  for (auto const &tok : vars_to_int)
  {
    vars_lookup[tok.second - MIN_VAR_INDEX] =
      std::string(tok.first.begin, tok.first.end);
  }

  grammar.insert({ MIN_VAR_INDEX, MIN_VAR_INDEX + 1, 0 });
  vars_lookup[0] = "start";

  return { grammar, vars_lookup };
}
