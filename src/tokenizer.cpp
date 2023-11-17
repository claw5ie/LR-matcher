#define PRINT_ERROR0(line_info, message) fprintf(stderr, "%zu:%zu: error: " message "\n", (line_info).line, (line_info).column)
#define PRINT_ERROR(line_info, message, ...) fprintf(stderr, "%zu:%zu: error: " message "\n", (line_info).line, (line_info).column, __VA_ARGS__)

struct LineInfo
{
  size_t offset = 0, line = 1, column = 1;
};

struct Token
{
  enum Type
    {
      Variable,
      Terminals_Sequence,
      Define,
      Delimiter,
      Bar,
      End_Of_File,
    };

  Type type;
  std::string_view text;
  LineInfo line_info;
};

struct TokenizerContext
{
  constexpr static uint8_t LOOKAHEAD = 2;

  Token tokens_buffer[LOOKAHEAD] = { };
  uint8_t token_start = 0;
  uint8_t token_count = 0;
  LineInfo line_info = { };
  const char *source;
};

void
advance_line_info(TokenizerContext &ctx, char ch)
{
  ++ctx.line_info.offset;
  ++ctx.line_info.column;
  if (ch == '\n')
    {
      ++ctx.line_info.line;
      ctx.line_info.column = 1;
    }
}

void
advance_line_info_assume_no_new_line(TokenizerContext &ctx, size_t count)
{
  ctx.line_info.offset += count;
  ctx.line_info.column += count;
}

void
buffer_token_custom(TokenizerContext &ctx)
{
  auto failed_to_tokenize = false;
  auto at = &ctx.source[ctx.line_info.offset];

  while (isspace(*at))
    advance_line_info(ctx, *at++);

  auto token = Token{
    .type = Token::End_Of_File,
    .text = { at, 0 },
    .line_info = ctx.line_info,
  };

  switch (*at)
    {
    case '\0':
      break;
    case ':':
      token.type = Token::Define;
      token.text = { token.text.data(), 1 };
      advance_line_info(ctx, *at++);
      break;
    case ';':
      token.type = Token::Delimiter;
      token.text = { token.text.data(), 1 };
      advance_line_info(ctx, *at++);
      break;
    case '|':
      token.type = Token::Bar;
      token.text = { token.text.data(), 1 };
      advance_line_info(ctx, *at++);
      break;
    default:
      if (isupper(*at))
        {
          do
            advance_line_info(ctx, *at++);
          while (isalnum(*at)
                 || *at == '\''
                 || *at == '-'
                 || *at == '_');

          token.type = Token::Variable;
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
                  advance_line_info(ctx, *at++);

                  if (*at != '\\' && !is_escape_char(*at))
                    {
                      failed_to_tokenize = true;
                      PRINT_ERROR(ctx.line_info, "invalid escape sequence '\\%c'", *at);
                    }
                }

              advance_line_info(ctx, *at++);
            }

          token.type = Token::Terminals_Sequence;
          token.text = { token.text.data(), size_t(at - token.text.data()) };
        }
    }

  if (failed_to_tokenize)
    exit(EXIT_FAILURE);

  assert(ctx.token_count < ctx.LOOKAHEAD);
  uint8_t index = (ctx.token_start + ctx.token_count) % ctx.LOOKAHEAD;
  ctx.tokens_buffer[index] = token;
  ctx.token_count++;
}

void
buffer_token_bnf(TokenizerContext &ctx)
{
  auto failed_to_tokenize = false;
  auto at = &ctx.source[ctx.line_info.offset];
  auto has_new_line = false;

  while (isspace(*at))
    {
      has_new_line = (*at == '\n') || has_new_line;
      advance_line_info(ctx, *at++);
    }

  auto token = Token{
    .type = Token::End_Of_File,
    .text = { at, 0 },
    .line_info = ctx.line_info,
  };

  if (has_new_line)
    {
      token.type = Token::Delimiter;
      goto push_token;
    }

  switch (*at)
    {
    case '\0':
      break;
    case '<':
      {
        do
          advance_line_info(ctx, *at++);
        while (*at != '\0' && *at != '>');

        if (*at != '>')
          {
            failed_to_tokenize = true;
            PRINT_ERROR0(ctx.line_info, "expected '>' to terminate variable name");
            exit(EXIT_FAILURE);
          }

        advance_line_info(ctx, *at++);

        size_t size = size_t(at - token.text.data());
        if (size <= 2)
          {
            failed_to_tokenize = true;
            PRINT_ERROR0(ctx.line_info, "empty variable name");
            exit(EXIT_FAILURE);
          }

        token.type = Token::Variable;
        token.text = { token.text.data() + 1, size - 2 };
      }

      break;
    case '|':
      {
        advance_line_info(ctx, *at++);
        token.type = Token::Bar;
        token.text = { token.text.data(), 1 };
      }

      break;
    default:
      if (at[0] == ':' && at[1] == ':' && at[2] == '=')
        {
          token.type = Token::Define;
          token.text = { token.text.data(), 3 };

          at += token.text.size();
          advance_line_info_assume_no_new_line(ctx, token.text.size());
        }
      else if (*at == '\"')
        {
          auto const is_escape_char =
            [](char ch) -> bool
            {
              return ch == '\"';
            };

          advance_line_info(ctx, *at++);

          while (*at != '\0' && *at != '\"')
            {
              if (*at == '\\')
                {
                  advance_line_info(ctx, *at++);

                  if (*at != '\\' && !is_escape_char(*at))
                    {
                      failed_to_tokenize = true;
                      PRINT_ERROR(ctx.line_info, "invalid escape sequence '\\%c'", *at);
                    }
                }

              advance_line_info(ctx, *at++);
            }

          if (*at != '\"')
            {
              failed_to_tokenize = true;
              PRINT_ERROR0(ctx.line_info, "expected '\"' to terminate string");
              exit(EXIT_FAILURE);
            }

          advance_line_info(ctx, *at++);

          token.type = Token::Terminals_Sequence;
          token.text = { token.text.data() + 1, size_t(at - token.text.data()) - 2 };
        }
      else
        {
          failed_to_tokenize = true;
          PRINT_ERROR(ctx.line_info, "expected '<' or '\"', but got '%c'", *at);

          while (*at != '\0' && *at != '<' && *at != '\"')
            advance_line_info(ctx, *at++);
        }
    }

  if (failed_to_tokenize)
    exit(EXIT_FAILURE);

 push_token:
  assert(ctx.token_count < ctx.LOOKAHEAD);
  uint8_t index = (ctx.token_start + ctx.token_count) % ctx.LOOKAHEAD;
  ctx.tokens_buffer[index] = token;
  ctx.token_count++;
}

using TokenizerBufferTokenFunction = void (*)(TokenizerContext &);

struct Tokenizer
{
  TokenizerContext ctx;
  TokenizerBufferTokenFunction buffer_token;

  Token grab()
  {
    assert(ctx.token_count > 0);
    return ctx.tokens_buffer[ctx.token_start];
  }

  Token::Type peek()
  {
    if (ctx.token_count == 0)
      buffer_token(ctx);

    return ctx.tokens_buffer[ctx.token_start].type;
  }

  Token::Type peek(uint8_t index)
  {
    assert(index < ctx.LOOKAHEAD);

    while (index >= ctx.token_count)
      buffer_token(ctx);

    return ctx.tokens_buffer[(ctx.token_start + index) % ctx.LOOKAHEAD].type;
  }

  void advance()
  {
    ctx.token_start += 1;
    ctx.token_start %= ctx.LOOKAHEAD;
    ctx.token_count -= 1;
  }

  bool expect(Token::Type expected)
  {
    if (peek() != expected)
      return false;
    advance();
    return true;
  }

  void skip_to_next_delimiter()
  {
    auto tt = peek();
    while (tt != Token::End_Of_File
           && tt != Token::Delimiter)
      {
        advance();
        tt = peek();
      }
  }
};
