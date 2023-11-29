struct Option
{
  char short_name;
  const char *long_name = nullptr;
  bool has_arg;
  int id;
};

const char *
is_prefix(const char *prefix, const char *string)
{
  if (!prefix || !string)
    return nullptr;

  char p, s;

  do
    {
      p = *prefix++;
      s = *string++;
    }
  while (p == s && p != '\0');

  return p == '\0' ? string - 1 : nullptr;
}

bool apply_option(void *ctx, const Option *option, const char *argument);

int
parse_options(void *ctx, int argc, char **argv, const Option *options, size_t option_count, int line)
{
  auto last_line = argc;
  auto failed_to_parse = false;

  for (; line < last_line; line++)
    {
      auto arg = argv[line];

      if (arg[0] == '-' && arg[1] == '-' && isalpha(arg[2]))
        {
          const char *after_prefix = nullptr;

          size_t i = 0;
          for (; i < option_count; i++)
            if ((after_prefix = is_prefix(options[i].long_name, &arg[2])))
              break;

          if (i >= option_count)
            {
              failed_to_parse = true;
              std::cerr << "error: couldn't find option '"
                        << arg
                        << "'\n";
              continue;
            }
          else if (options[i].has_arg)
            {
              if (*after_prefix == '=')
                failed_to_parse = apply_option(ctx, &options[i], after_prefix + 1) || failed_to_parse;
              else if (line + 1 < last_line)
                failed_to_parse = apply_option(ctx, &options[i], argv[++line]) || failed_to_parse;
              else
                {
                  failed_to_parse = true;
                  std::cerr << "error: no argument for option '"
                            << arg
                            << "'\n";
                  continue;
                }
            }
          else if (*after_prefix != '\0')
            {
              failed_to_parse = true;
              std::cerr << "error: unused argument for option '"
                        << options[i].long_name
                        << "'\n";
              continue;
            }
          else
            failed_to_parse = apply_option(ctx, &options[i], nullptr) || failed_to_parse;
        }
      else if (arg[0] == '-' && isalpha(arg[1]))
        {
          for (int column = 1; arg[column] != '\0'; column++)
            {
              auto short_name = arg[column];

              if (!isalpha(short_name))
                {
                  failed_to_parse = true;
                  std::cerr << "error: short option must be alphabetic character, but got '"
                            << short_name
                            << "'\n";

                  break;
                }

              size_t i = 0;
              for (; i < option_count; i++)
                if (options[i].short_name == short_name)
                  break;

              if (i >= option_count)
                {
                  failed_to_parse = true;
                  std::cerr << "error: couldn't find option '"
                            << short_name
                            << "'\n";
                  break;
                }
              else if (options[i].has_arg)
                {
                  if (arg[column + 1] != '\0')
                    {
                      failed_to_parse = apply_option(ctx, &options[i], &arg[++column]) || failed_to_parse;
                      break;
                    }
                  else if (line + 1 < last_line)
                    {
                      failed_to_parse = apply_option(ctx, &options[i], argv[++line]) || failed_to_parse;
                      break;
                    }
                  else
                    {
                      failed_to_parse = true;
                      std::cerr << "error: no argument for option '"
                                << short_name
                                << "'\n";
                      break;
                    }
                }
              else
                failed_to_parse = apply_option(ctx, &options[i], nullptr) || failed_to_parse;
            }
        }
      else
        {
          memmove(&argv[line], &argv[line + 1], sizeof(*argv) * (argc - line - 1));
          argv[argc - 1] = arg;
          --last_line;
          --line;
        }
    }

  if (failed_to_parse)
    exit(EXIT_FAILURE);

  return last_line;
}
