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

Grammar parse_grammar(const char *str);

std::string rule_to_string(
  const Grammar &grammar, const Grammar::Rule &rule
  );

#endif // GRAMMAR_HPP
