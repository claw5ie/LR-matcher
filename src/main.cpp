#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <set>
#include <map>

#include <cstring>
#include <cstdint>
#include <cassert>

#include "utils.hpp"
#include "itemsets.hpp"
using namespace std;
#include "utils.cpp"
#include "itemsets.cpp"

int
main(int argc, char **argv)
{
  if (argc != 2)
    {
      cerr << "error: the number of arguments should be 2.\n";
      return EXIT_FAILURE;
    }

  auto grammar = parse_grammar(argv[1]);

  cout << "Grammar:\n";
  for (auto &elem: grammar.rules)
    cout << "  " << rule_to_string(grammar, elem) << '\n';
  cout << '\n';

  auto item_sets = find_item_sets(grammar);

  for (size_t i = 0; i < item_sets.size(); i++)
    {
      cout << "State " << i << ":\n  ";
      for (auto &trans: item_sets[i].actions)
        {
          switch (trans.type)
            {
            case Action_Reduce:
              cout << "r("
                   << rule_to_string(grammar, *trans.reduce_to)
                   << ")";
              break;
            case Action_Shift:
              cout << '\''
                   << (char)trans.label
                   << "\' -> s"
                   << trans.dst;
              break;
            case Action_Goto:
              cout << lookup(grammar, trans.label)
                   << " -> g" << trans.dst;
              break;
            }

          cout << "; ";
        }

      cout << '\n';

      for (auto &item: item_sets[i].itemset)
        {
          cout << lookup(grammar, (*item.rule)[0])
               << ": ";

          size_t i = 1;
          for (; i + 1 < item.rule->size(); i++)
            {
              if (i == item.dot)
                cout << '.';

              uint32_t symbol = (*item.rule)[i];

              if (is_variable(symbol))
                cout << lookup(grammar, symbol);
              else
                cout << (char)symbol;
            }

          if (i == item.dot)
            cout << '.';

          cout << '\n';
        }

      cout << '\n';
    }
}
