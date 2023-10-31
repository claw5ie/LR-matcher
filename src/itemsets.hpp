#ifndef ITEMSETS_HPP
#define ITEMSETS_HPP

using TerminalType = char;
using SymbolType = uint32_t;

static_assert(sizeof(TerminalType) < sizeof(SymbolType));

constexpr SymbolType FIRST_RESERVED_SYMBOL_INDEX = 1 << (sizeof(TerminalType) * CHAR_BIT);
constexpr SymbolType FIRST_USER_SYMBOL_INDEX = FIRST_RESERVED_SYMBOL_INDEX + 1;
constexpr SymbolType SYMBOL_END = 0;

struct LineInfo
{
  size_t offset = 0, line = 1, column = 1;
};

enum TokenType
  {
    Token_Variable,
    Token_Terminals_Sequence,
    Token_Colon,
    Token_Semicolon,
    Token_Bar,
    Token_End_Of_File,
  };

struct Token
{
  TokenType type;
  std::string_view text;
  LineInfo line_info;
};

struct Tokenizer
{
  constexpr static uint8_t LOOKAHEAD = 2;

  Token tokens_buffer[LOOKAHEAD];
  uint8_t token_start = 0;
  uint8_t token_count = 0;
  LineInfo line_info;
  const char *source;

  static inline
  void report_error_start(LineInfo line_info)
  {
    std::cerr << line_info.line
              << ':'
              << line_info.column
              << ": error: ";
  }

  template<typename T>
  static inline
  void report_error_print(T value)
  {
    std::cerr << value;
  }

  static inline
  void report_error_end()
  {
    std::cerr << '\n';
  }

  Token grab()
  {
    assert(token_count > 0);
    return tokens_buffer[token_start];
  }

  TokenType peek()
  {
    if (token_count == 0)
      buffer_token();

    return tokens_buffer[token_start].type;
  }

  TokenType peek(uint8_t index)
  {
    assert(index < LOOKAHEAD);

    while (index >= token_count)
      buffer_token();

    return tokens_buffer[(token_start + index) % LOOKAHEAD].type;
  }

  void advance()
  {
    token_start += 1;
    token_start %= LOOKAHEAD;
    token_count -= 1;
  }

  bool expect(TokenType expected)
  {
    if (peek() != expected)
      return false;
    advance();
    return true;
  }

  void skip_to_next_semicolon()
  {
    auto tt = peek();
    while (tt != Token_End_Of_File
           && tt != Token_Semicolon)
      {
        advance();
        tt = peek();
      }
  }

  void advance_line_info()
  {
    ++line_info.offset;
    ++line_info.column;
    if (source[line_info.offset - 1] == '\n')
      {
        ++line_info.line;
        line_info.column = 1;
      }
  }

  void buffer_token()
  {
    auto failed_to_tokenize = false;
    auto at = &source[line_info.offset];

    while (isspace(*at))
      {
        advance_line_info();
        at++;
      }

    Token token;
    token.type = Token_End_Of_File;
    token.text = { at, 0 };
    token.line_info = line_info;

    switch (*at)
      {
      case '\0':
        break;
      case ':':
        token.type = Token_Colon;
        token.text = { token.text.data(), 1 };
        advance_line_info();
        at++;
        break;
      case ';':
        token.type = Token_Semicolon;
        token.text = { token.text.data(), 1 };
        advance_line_info();
        at++;
        break;
      case '|':
        token.type = Token_Bar;
        token.text = { token.text.data(), 1 };
        advance_line_info();
        at++;
        break;
      default:
        if (isupper(*at))
          {
            do
              {
                advance_line_info();
                at++;
              }
            while (isalnum(*at)
                   || *at == '\''
                   || *at == '-'
                   || *at == '_');

            token.type = Token_Variable;
            token.text = { token.text.data(), size_t(at - token.text.data()) };
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

            while (*at != '\0' && !is_escape_char(*at))
              {
                if (*at == '\\')
                  {
                    advance_line_info();
                    at++;

                    if (*at != '\\' && !is_escape_char(*at))
                      {
                        failed_to_tokenize = true;
                        report_error_start(line_info);
                        report_error_print(": error: invalid escape sequence '\\");
                        report_error_print(*at);
                        report_error_print("'");
                        report_error_end();
                      }
                  }

                advance_line_info();
                at++;
              }

            token.type = Token_Terminals_Sequence;
            token.text = { token.text.data(), size_t(at - token.text.data()) };
          }
      }

    if (failed_to_tokenize)
      exit(EXIT_FAILURE);

    assert(token_count < LOOKAHEAD);
    uint8_t index = (token_start + token_count) % LOOKAHEAD;
    tokens_buffer[index] = token;
    token_count++;
  }
};

struct Grammar
{
  // Rule stores the index of a symbol that is being defined in the beginning of the vector. In that way all rules that define the same symbol are consecutive in set.
  using Rule = std::vector<SymbolType>;

  std::set<Rule> rules;
  std::vector<std::string> lookup;

  std::string &grab_variable_name(SymbolType index)
  {
    assert(index >= FIRST_RESERVED_SYMBOL_INDEX);
    return lookup[index - FIRST_RESERVED_SYMBOL_INDEX];
  }

  std::set<Rule>::iterator find_first_rule(SymbolType symbol)
  {
    return rules.lower_bound({ symbol });
  }
};

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
    Item result = *this;
    result.dot_index += (symbol_at_dot() != SYMBOL_END);
    return result;
  }

  bool operator==(const Item &other) const
  {
    return this->dot_index == other.dot_index
      && this->rule == other.rule;
  }
};

struct ItemIsLess
{
  bool operator()(const Item &, const Item &) const;
};

using ItemSet = std::set<Item, ItemIsLess>;

struct State;

enum ActionType
  {
    Action_Shift,
    Action_Reduce,
  };

struct Action
{
  ActionType type;
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
  StateId id = 0;
  uint8_t flags = 0;
};

using ParsingTable = std::list<State>;

#endif // ITEMSETS_HPP
