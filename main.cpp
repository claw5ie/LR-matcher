#include <iostream>
#include "Grammar.hpp"

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

  for (auto const &elem : grammar)
  {
    for (auto ch : elem)
    {
      std::cout << ch << ' ';
    }

    std::cout << std::endl;
  }

  std::cout << std::endl;
}
