#include "itemsets.cpp"
#include <iostream>

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fputs("error: the number of arguments should be 2.\n", stderr);

    return EXIT_FAILURE;
  }

  auto const grammar = parse_grammar(argv[1]);

  cout << "Grammar:\n";
  for (const auto &elem: grammar.rules)
    cout << "  " << rule_to_string(grammar, elem) << '\n';
  cout << '\n';

  auto const item_sets = find_item_sets(grammar);

  for (size_t i = 0; i < item_sets.size(); i++)
  {
    cout << "State " << i << ":\n  ";
    for (const auto &trans: item_sets[i].actions)
    {
      switch (trans.type)
      {
      case Action::Reduce:
        cout << "r("
             << rule_to_string(grammar, *trans.reduce_to)
             << ")";
        break;
      case Action::Shift:
        cout << '\''
             << (char)trans.src
             << "\' -> s"
             << trans.dst;
        break;
      case Action::Goto:
        cout << lookup(grammar, trans.src)
             << " -> g" << trans.dst;
        break;
      }

      cout << "; ";
    }

    cout << '\n';

    for (const auto &item: item_sets[i].itemset)
    {
      cout << lookup(grammar, (*item.rule)[0])
           << ": ";

      size_t i = 1;
      for (; i + 1 < item.rule->size(); i++)
      {
        if (i == item.dot)
          cout << '.';

        uint32_t const symbol = (*item.rule)[i];

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
