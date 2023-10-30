#include <iostream>
#include <vector>
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

#include "itemsets.hpp"
#include "itemsets.cpp"

int
main(int argc, char **argv)
{
  if (argc != 2)
    {
      std::cerr << "error: the number of arguments should be 2.\n";
      return EXIT_FAILURE;
    }

  auto grammar = parse_context_free_grammar(argv[1]);
  auto states = compute_parsing_table(grammar);

  std::cout << "Grammar:\n";
  for (auto &rule: grammar.rules)
    std::cout << "  " << rule_to_string(grammar, rule) << '\n';
  std::cout << '\n';

  for (auto &state: states)
    {
      std::cout << "State " << state.id << ":\n  ";
      for (auto &actions: state.actions)
        {
          switch (actions.type)
            {
            case Action_Reduce:
              std::cout << "r("
                        << rule_to_string(grammar, *actions.as.reduce.to_rule)
                        << ")";
              break;
            case Action_Shift:
              std::cout << '\''
                        << actions.as.shift.label
                        << "\' -> s"
                        << actions.as.shift.item->id;
              break;
            case Action_Go_To:
              std::cout << grammar.grab_variable_name(actions.as.go_to.label)
                        << " -> g" << actions.as.go_to.item->id;
              break;
            }

          std::cout << "; ";
        }

      std::cout << '\n';

      for (auto &item: state.itemset)
        {
          std::cout << grammar.grab_variable_name((*item.rule)[0])
                    << ": ";

          size_t i = 1;
          for (; i + 1 < item.rule->size(); i++)
            {
              if (i == item.dot_index)
                std::cout << '.';

              auto symbol = (*item.rule)[i];

              if (is_variable(symbol))
                std::cout << grammar.grab_variable_name(symbol);
              else
                std::cout << (TerminalType)symbol;
            }

          if (i == item.dot_index)
            std::cout << '.';

          std::cout << '\n';
        }

      std::cout << '\n';
    }
}
