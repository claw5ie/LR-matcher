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

    auto const is_keyword =
      [](char ch) -> bool
      {
        return ch == ';' || ch == ':' || ch == '|';
      };

    while (
      isprint(**str) &&
      !isupper(**str) &&
      !isspace(**str) &&
      !is_keyword(**str)
      )
    {
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

Grammar parse_grammar(char const *str)
{
  std::vector<Token> tokens; tokens.reserve(128);
  std::map<Token, int, TokenIsLess> vars_to_int;

  Token next = next_token(&str);

  for (int var_index = -65535; next.type != Token::Eof; )
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

    // Don't check that "index" is valid, because it is guaranteed to be
    // in the map.

    auto const find_last_non_terminal =
      [](std::vector<Token> const &toks, size_t start) -> std::pair<size_t, size_t>
      {
        size_t size_of_the_rule = 0,
          i = start;

        while (i < toks.size())
        {
          switch (toks[i].type)
          {
          case Token::Variable:
            size_of_the_rule += 1;
            break;
          case Token::TerminalSequence:
            size_of_the_rule += toks[i].end - toks[i].begin;
            break;
          default:
            return { i, size_of_the_rule };
          }

          i++;
        }

        return { i, size_of_the_rule };
      };

    do
    {
      i++;
      auto res = find_last_non_terminal(tokens, i);

      if (res.first == i)
      {
        grammar.insert({ index->second, 0 });
      }
      else
      {
        std::vector<int> rule; rule.reserve(res.second + 1 + 1);
        rule.push_back(index->second);

        while (i < res.first)
        {
          auto const &tok = tokens[i];
          switch (tok.type)
          {
          case Token::Variable:
          {
            auto other_index = vars_to_int.find(tok);

            if (other_index == vars_to_int.end())
            {
              fputs("error: unrecognized variable.\n", stderr);

              exit(EXIT_FAILURE);
            }

            rule.push_back(other_index->second);
            break;
          }
          case Token::TerminalSequence:
          {
            for (char const *beg = tok.begin; beg < tok.end; beg++)
              rule.push_back(*beg);
            break;
          }
          default:
            ; // Don't do anything to avoid annoying warning.
          }

          i++;
        }

        rule.push_back(0);
        grammar.insert(std::move(rule));
      }
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

  return grammar;
}
