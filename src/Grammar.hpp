#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <vector>
#include <set>
#include <string>

#define MIN_VAR_INDEX 256

struct Grammar
{
  using Rule = std::vector<uint32_t>;

  std::set<Rule> rules;
  std::vector<std::string> lookup;
};

Grammar parse_grammar(char const *str);

#endif // GRAMMAR_HPP
