#ifndef ITEM_SETS_HPP
#define ITEM_SETS_HPP

#include <list>
#include <cstddef>
#include "Grammar.hpp"

using Item = std::pair<Rule const *, size_t>;

struct ItemIsLess
{
  bool operator()(Item const &, Item const &) const;
};

using ItemSet = std::set<Item, ItemIsLess>;

struct Action
{
  enum
  {
    Shift,
    Goto,
    Reduce
  } action;

  int source,
    destination;
  Rule const *reduce_to;
};

using ParsingTable =
  std::pair<std::vector<ItemSet>, std::vector<std::list<Action>>>;

ParsingTable find_item_sets(Grammar const &grammar);

#endif // ITEM_SETS_HPP
