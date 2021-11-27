#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <vector>
#include <set>

using Rule = std::vector<int>;
using Grammar = std::set<Rule>;

Grammar parse_grammar(char const *str);

#endif // GRAMMAR_HPP
