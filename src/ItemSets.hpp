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
  uint32_t source,
    destination;
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
  bool operator()(Item const &, Item const &) const;
};

using ItemSet = std::set<Item, ItemIsLess>;

struct State
{
  ItemSet itemset;
  std::list<Action> actions;
};

using ParsingTable = std::vector<State>;

ParsingTable find_item_sets(Grammar const &grammar);

#endif // ITEM_SETS_HPP
