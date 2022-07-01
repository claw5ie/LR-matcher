#include <iostream>

#include "src/Grammar.hpp"
#include "src/ItemSets.hpp"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fputs("error: the number of arguments should be 2.\n", stderr);

    return EXIT_FAILURE;
  }

  auto grammar = parse_grammar(argv[1]);

  auto const print_rule =
    [](Rule const &rule, std::vector<std::string> const &lookup) -> void
    {
      std::cout << lookup[rule[0] - MIN_VAR_INDEX] << " : ";

      for (size_t i = 1; i + 1 < rule.size(); i++)
      {
        if (rule[i] >= MIN_VAR_INDEX)
          std::cout << lookup[rule[i] - MIN_VAR_INDEX];
        else
          std::cout << (char)rule[i];
      }
    };

  std::cout << "Grammar:\n";
  for (auto const &elem : grammar.first)
  {
    std::cout << "  ";
    print_rule(elem, grammar.second);
    std::cout << '\n';
  }
  std::cout << '\n';

  auto item_sets = find_item_sets(grammar.first);

  for (size_t i = 0; i < item_sets.size(); i++)
  {
    std::cout << "State " << i << ":\n  ";
    for (auto const &trans : item_sets[i].second)
    {
      switch (trans.action)
      {
      case Action::Reduce:
        std::cout << "r(";
        print_rule(*trans.reduce_to, grammar.second);
        std::cout << ")";
        break;
      case Action::Shift:
        std::cout << '\'' << (char)trans.source
                  << "\' -> s" << trans.destination;
        break;
      case Action::Goto:
        std::cout << grammar.second[trans.source - MIN_VAR_INDEX]
                  << " -> g" << trans.destination;
        break;
      }

      std::cout << "; ";
    }

    std::cout << std::endl;

    for (auto const &item : item_sets[i].first)
    {
      std::cout << grammar.second[(*item.first)[0] - MIN_VAR_INDEX] << " : ";

      for (size_t i = 1, end = item.first->size(); i < end; i++)
      {
        if (i == item.second)
          std::cout << (i == 1 ? ". " : " . ");

        int const val = (*item.first)[i];

        if (val == 0)
          std::cout << '\n';
        else if (val >= MIN_VAR_INDEX)
          std::cout << grammar.second[val - MIN_VAR_INDEX];
        else
          std::cout << (char)val;
      }
    }

    std::cout << std::endl;
  }
}
