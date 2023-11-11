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

  Token tokens_buffer[LOOKAHEAD] = { };
  uint8_t token_start = 0;
  uint8_t token_count = 0;
  LineInfo line_info = { };
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

    auto token = Token{
      .type = Token_End_Of_File,
      .text = { at, 0 },
      .line_info = line_info,
    };

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
