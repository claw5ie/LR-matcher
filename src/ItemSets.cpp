#include "ItemSets.hpp"

bool is_variable(uint32_t symbol)
{
  return symbol >= MIN_VAR_INDEX;
}

uint32_t symbol_at_dot(Item const &item)
{
  return (*item.rule)[item.dot];
}

bool are_items_different(const Item &left, const Item &right)
{
  return left.rule != right.rule && left.dot != right.dot;
}

bool ItemIsLess::operator()(
  Item const &left, Item const &right
  ) const
{
  uint32_t const sym_left = symbol_at_dot(left),
    sym_right = symbol_at_dot(right);

  if (sym_left == sym_right)
  {
    if (left.dot == right.dot)
      return left.rule < right.rule;
    else
      return left.dot < right.dot;
  }
  else
  {
    return sym_left < sym_right;
  }
}

void closure(Grammar const &grammar, ItemSet &item_set)
{
  std::vector<uint32_t> to_visit;

  auto const should_visit =
    [](uint32_t symbol,
       std::vector<uint32_t> const &to_visit) -> bool
    {
      for (uint32_t elem: to_visit)
      {
        if (elem == symbol)
          return false;
      }

      return true;
    };

  for (auto const &item: item_set)
  {
    uint32_t const symbol = symbol_at_dot(item);
    if (is_variable(symbol) && should_visit(symbol, to_visit))
      to_visit.push_back(symbol);
  }

  for (size_t i = 0; i < to_visit.size(); i++)
  {
    uint32_t const symbol = to_visit[i];

    for (auto it = grammar.rules.lower_bound({ symbol });
         it != grammar.rules.end() && (*it)[0] == symbol;
         it++)
    {
      item_set.emplace(Item{ &(*it), 1 });

      uint32_t const new_candidate = (*it)[1];
      if (is_variable(new_candidate) && should_visit(new_candidate, to_visit))
        to_visit.push_back(new_candidate);
    }
  }
}

ParsingTable find_item_sets(const Grammar &grammar)
{
  auto const shift_dot =
    [](Item const &item) -> Item
    {
      return { item.rule, item.dot + ((*item.rule)[item.dot] != 0) };
    };

  auto const are_item_sets_equal =
    [](ItemSet const &left, ItemSet const &right) -> bool
    {
      if (left.size() != right.size())
        return false;

      for (auto lit = left.begin(), rit = right.begin();
           lit != left.end();
           lit++, rit++)
      {
        if (are_items_different(*lit, *rit))
          return false;
      }

      return true;
    };

  if (grammar.rules.empty())
    return { };

  ParsingTable table;

  table.push_back({ { { &(*grammar.rules.begin()), 1 } }, { } });
  closure(grammar, table[0].itemset);

  for (size_t i = 0; i < table.size(); i++)
  {
    auto *item_set = &table[i].itemset;

    for (auto it = item_set->begin(); it != item_set->end(); )
    {
      if (symbol_at_dot(*it) == 0)
      {
        auto &actions = table[i].actions;
        while (it != item_set->end() && symbol_at_dot(*it) == 0)
        {
          actions.push_back({ Action::Reduce, 0, 0, it->rule });
          it++;
        }
      }

      while (it != item_set->end())
      {
        uint32_t const shift_symbol = symbol_at_dot(*it);

        table.push_back({ { }, { } });
        // Update reference in case of reallocation.
        item_set = &table[i].itemset;

        while (it != item_set->end() &&
               symbol_at_dot(*it) == shift_symbol)
        {
          table.back().itemset.insert(shift_dot(*it));
          it++;
        }

        closure(grammar, table.back().itemset);

        uint32_t where_to_transition = table.size() - 1;
        for (size_t j = 0; j + 1 < table.size(); j++)
        {
          if (are_item_sets_equal(
                table[j].itemset, table.back().itemset
                ))
          {
            table.pop_back();
            where_to_transition = j;
            break;
          }
        }

        table[i].actions.push_back(
          { is_variable(shift_symbol) ? Action::Goto : Action::Shift,
            shift_symbol,
            where_to_transition,
            nullptr }
          );
      }
    }
  }

  return table;
}
