#ifndef ITEM_SETS_HPP
#define ITEM_SETS_HPP

#include <list>
#include <cstddef>
#include "Grammar.hpp"

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

bool are_items_different(const Item &left, const Item &right);

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

ParsingTable find_item_sets(const Grammar &grammar);

#endif // ITEM_SETS_HPP
