#include <iostream>

#include "Grammar.hpp"
#include "ItemSets.hpp"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fputs("error: the number of arguments should be 2.\n", stderr);

    return EXIT_FAILURE;
  }

  auto const grammar = parse_grammar(argv[1]);

  std::cout << "Grammar:\n";
  for (const auto &elem: grammar.rules)
    std::cout << "  " << rule_to_string(grammar, elem) << '\n';
  std::cout << '\n';

  auto const item_sets = find_item_sets(grammar);

  for (size_t i = 0; i < item_sets.size(); i++)
  {
    std::cout << "State " << i << ":\n  ";
    for (const auto &trans: item_sets[i].actions)
    {
      switch (trans.type)
      {
      case Action::Reduce:
        std::cout << "r("
                  << rule_to_string(grammar, *trans.reduce_to)
                  << ")";
        break;
      case Action::Shift:
        std::cout << '\''
                  << (char)trans.src
                  << "\' -> s"
                  << trans.dst;
        break;
      case Action::Goto:
        std::cout << grammar.lookup[trans.src - MIN_VAR_INDEX]
                  << " -> g" << trans.dst;
        break;
      }

      std::cout << "; ";
    }

    std::cout << '\n';

    for (const auto &item: item_sets[i].itemset)
    {
      std::cout << grammar.lookup[(*item.rule)[0] - MIN_VAR_INDEX]
                << ": ";

      size_t i = 1;
      for (; i + 1 < item.rule->size(); i++)
      {
        if (i == item.dot)
          std::cout << '.';

        uint32_t const symbol = (*item.rule)[i];

        if (symbol >= MIN_VAR_INDEX)
          std::cout << grammar.lookup[symbol - MIN_VAR_INDEX];
        else
          std::cout << (char)symbol;
      }

      if (i == item.dot)
        std::cout << '.';

      std::cout << '\n';
    }

    std::cout << '\n';
  }
}
