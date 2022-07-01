#ifndef ITEM_SETS_HPP
#define ITEM_SETS_HPP

#include <list>
#include <cstddef>
#include "Grammar.hpp"

struct ItemIsLess;
struct Action;

using Item = std::pair<Grammar::Rule const *, size_t>;
using ItemSet = std::set<Item, ItemIsLess>;
using ParsingTable =
  std::vector<std::pair<ItemSet, std::list<Action>>>;

struct ItemIsLess
{
  bool operator()(Item const &, Item const &) const;
};

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
  Grammar::Rule const *reduce_to;
};

ParsingTable find_item_sets(Grammar const &grammar);

#endif // ITEM_SETS_HPP
