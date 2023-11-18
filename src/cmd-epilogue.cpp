enum OptionType
  {
    Grammar_Form,
    Generate_Automaton,
    Generate_Automaton_Steps,
  };

struct Config
{
  bool use_bnf = false;
  bool generate_automaton = false;
  bool generate_automaton_steps = false;
  const char *automaton_filepath = nullptr;
  const char *automaton_steps_filepath = nullptr;
  const char *args[32] = { };
  size_t arg_count = 0;
};

bool
apply_option(void *ctx_ptr, const Option *option, const char *argument)
{
  Config &ctx = *(Config *)ctx_ptr;

  switch ((OptionType)option->id)
    {
    case Grammar_Form:
      {
        if (strcmp("bnf", argument) == 0)
          ctx.use_bnf = true;
        else if (strcmp("custom", argument) == 0)
          ctx.use_bnf = false;
        else
          {
            std::cerr << "error: '"
                      << argument
                      << "' is not a valid notation\n";
            return true;
          }
      }

      break;
    case Generate_Automaton:
      {
        ctx.generate_automaton = true;
        ctx.automaton_filepath = argument;
      }

      break;
    case Generate_Automaton_Steps:
      {
        ctx.generate_automaton_steps = true;
        ctx.automaton_steps_filepath = argument;
      }

      break;
    }

  return false;
}

bool
apply_non_option(void *ctx_ptr, const char *string)
{
  Config &ctx = *(Config *)ctx_ptr;

  if (ctx.arg_count >= sizeof(ctx.args) / sizeof(*ctx.args))
    {
      std::cerr << "error: too many arguments.\n";
      return true;
    }

  ctx.args[ctx.arg_count++] = string;

  return false;
}
