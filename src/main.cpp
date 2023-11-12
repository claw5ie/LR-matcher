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

int
main(int argc, char **argv)
{
  if (argc < 2)
    {
      std::cerr << "error: the number of arguments should be at least 2.\n";
      return EXIT_FAILURE;
    }

  auto grammar = parse_context_free_grammar(argv[1]);
  auto table = compute_parsing_table(grammar);
  auto pda = PDA{
    .grammar = &grammar,
    .table = &table,
  };

  for (int i = 2; i < argc; i++)
    {
      auto result = pda.match(argv[i]);
      std::cout << "'" << argv[i] << "': ";
      std::cout << (result ? "accepted" : "rejected") << '\n';
    }

  print_grammar(grammar);
  print_pushdown_automaton(grammar, table);
}
