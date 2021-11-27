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

  char const *str = argv[1];
  auto grammar = parse_grammar(str);
  grammar.insert({ -65536, -65535, 0 });

  auto const print_rule =
    [](Rule const &rule) -> void
    {
      for (size_t i = 0; i < rule.size(); i++)
        std::cout << rule[i] << (i + 1 < rule.size() ? " " : "");
    };

  puts("Grammar:");
  for (auto const &elem : grammar)
  {
    print_rule(elem);

    std::cout << std::endl;
  }

  std::cout << std::endl;

  auto item_sets = find_item_sets(grammar);

  for (size_t i = 0; i < item_sets.size(); i++)
  {
    std::cout << "State " << i << ":\n  ";
    for (auto const &trans : item_sets[i].second)
    {
      switch (trans.action)
      {
      case Action::Reduce:
        std::cout << "r(";
        print_rule(*trans.reduce_to);
        std::cout << ")";
        break;
      case Action::Shift:
        std::cout << trans.source << " -> s" << trans.destination;
        break;
      case Action::Goto:
        std::cout << trans.source << " -> g" << trans.destination;
        break;
      }

      std::cout << "; ";
    }

    std::cout << std::endl;

    for (auto const &item : item_sets[i].first)
    {
      for (size_t i = 0, end = item.first->size(); i < end; i++)
      {
        if (i == item.second)
          fputs(". ", stdout);

        std::cout << (*item.first)[i] << (i + 1 < end ? ' ' : '\n');
      }
    }

    std::cout << std::endl;
  }
}
