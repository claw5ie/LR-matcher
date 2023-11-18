#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <string>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>

#include <cstring>
#include <cstdint>
#include <climits>
#include <cassert>

#include "tokenizer.cpp"
#include "grammar.cpp"
#include "matcher.cpp"
#include "other-stuff.cpp"
#include "cmd.cpp"
#include "cmd-epilogue.cpp"

constexpr Option options[] = {
  { .short_name = 'f', .has_arg = true, .id = Grammar_Form },
  { .short_name = '\0', .long_name = "generate-automaton", .has_arg = true, .id = Generate_Automaton },
  { .short_name = '\0', .long_name = "generate-steps", .has_arg = true, .id = Generate_Automaton_Steps },
};

int
main(int argc, char **argv)
{
  auto config = Config{ };
  parse_options(&config, argc, argv, options, sizeof(options) / sizeof(*options), 1);

  if (config.arg_count == 0)
    {
      std::cerr << "error: no grammar given.\n";
      return EXIT_FAILURE;
    }

  auto grammar = parse_context_free_grammar(config.args[0], config.use_bnf);
  auto table = compute_parsing_table(grammar);
  auto pda = PDA{
    .grammar = &grammar,
    .table = &table,
  };

  if (config.generate_automaton)
      generate_automaton_json(table, config.automaton_filepath);

  for (size_t i = 1; i < config.arg_count; i++)
    {
      auto result = pda.match(config.args[i]);
      std::cout << "'" << config.args[i] << "': ";
      std::cout << (result ? "accepted" : "rejected") << '\n';

      if (config.generate_automaton_steps)
        {
          auto name = std::string{ config.automaton_steps_filepath } + std::to_string(i - 1);
          pda.generate_json_of_steps(config.args[i], name.c_str());
        }
    }

  print_grammar(grammar);
  print_pushdown_automaton(grammar, table);
}
