#ifndef GRAMMAR_HPP
#define GRAMMAR_HPP

#include <vector>
#include <set>
#include <string>

#define MIN_VAR_INDEX 256

using Rule = std::vector<int>;
using Grammar = std::set<Rule>;

std::pair<Grammar, std::vector<std::string>> parse_grammar(char const *str);

#endif // GRAMMAR_HPP
