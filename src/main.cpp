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
  auto last_non_option_index = parse_options(&config, argc, argv, options, sizeof(options) / sizeof(*options), 1);

  if (argc - last_non_option_index == 0)
    {
      std::cerr << "error: missing grammar\nusage: [OPTIONS] [GRAMMAR] [STRINGS_TO_MATCH]\n";
      return EXIT_FAILURE;
    }

  auto grammar = parse_context_free_grammar(argv[last_non_option_index], config.use_bnf);
  auto table = compute_parsing_table(grammar);
  auto pda = PDA{
    .grammar = &grammar,
    .table = &table,
  };

  if (config.automaton_filepath)
    generate_automaton_json(table, config.automaton_filepath);

  for (int i = last_non_option_index + 1, j = 0; i < argc; i++, j++)
    {
      auto string = argv[i];
      auto result = pda.match(string);
      std::cout << "'" << string << "': ";
      std::cout << (result ? "accepted" : "rejected") << '\n';

      if (config.automaton_steps_filepath)
        {
          auto name = std::string{ config.automaton_steps_filepath } + std::to_string(j);
          pda.generate_automaton_steps_json(string, name.c_str());
        }
    }

  print_grammar(grammar);
  print_pushdown_automaton(grammar, table);
}
