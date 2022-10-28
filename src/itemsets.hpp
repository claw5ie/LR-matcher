#ifndef ITEMSETS_HPP
#define ITEMSETS_HPP

#include <vector>
#include <string>
#include <list>
#include <set>
#include <cstdint>

#define MIN_VAR_INDEX 256

struct Grammar
{
  using Rule = std::vector<uint32_t>;

  std::set<Rule> rules;
  std::vector<std::string> lookup;
};

struct Action
{
  enum Type
  {
    Shift,
    Goto,
    Reduce
  };

  Type type;
  uint32_t src;
  uint32_t dst;
  Grammar::Rule const *reduce_to;
};

struct Item
{
  const Grammar::Rule *rule;
  size_t dot;
};

struct ItemIsLess
{
  bool operator()(const Item &, const Item &) const;
};

using ItemSet = std::set<Item, ItemIsLess>;

struct State
{
  ItemSet itemset;
  std::list<Action> actions;
};

using ParsingTable = std::vector<State>;

#endif // ITEMSETS_HPP
