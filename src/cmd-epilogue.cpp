enum OptionType
  {
    Grammar_Form,
    Generate_Automaton,
    Generate_Automaton_Steps,
  };

struct Config
{
  bool use_bnf = false;
  const char *automaton_filepath = nullptr;
  const char *automaton_steps_filepath = nullptr;
};

bool
apply_option(void *ctx_ptr, const Option *option, const char *argument)
{
  auto &ctx = *(Config *)ctx_ptr;

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
      ctx.automaton_filepath = argument;
      break;
    case Generate_Automaton_Steps:
      ctx.automaton_steps_filepath = argument;
      break;
    }

  return false;
}
