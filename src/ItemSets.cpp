#include "ItemSets.hpp"

bool is_variable(int symbol)
{
  return symbol >= MIN_VAR_INDEX;
}

int symbol_at_dot(Item const &item)
{
  return (*item.first)[item.second];
}

bool ItemIsLess::operator()(Item const &left, Item const &right) const
{
  int const sym_left = symbol_at_dot(left),
    sym_right = symbol_at_dot(right);

  if (sym_left == sym_right)
  {
    if (left.second == right.second)
      return left.first < right.first;
    else
      return left.second < right.second;
  }
  else
  {
    return sym_left < sym_right;
  }
}

void closure(Grammar const &grammar, ItemSet &item_set)
{
  std::vector<int> to_visit;
  to_visit.reserve(32);

  auto const should_visit =
    [](int symbol, std::vector<int> const &to_visit) -> bool
    {
      for (int elem: to_visit)
      {
        if (elem == symbol)
          return false;
      }

      return true;
    };

  for (auto const &item: item_set)
  {
    int const symbol = symbol_at_dot(item);
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
      item_set.emplace(&(*it), 1);

      int const new_candidate = (*it)[1];
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
      return { item.first, item.second + ((*item.first)[item.second] != 0) };
    };

  auto const are_item_sets_equal =
    [](ItemSet const &left, ItemSet const &right) -> bool
    {
      if (left.size() != right.size())
        return false;

      for (
        auto lit = left.begin(), rit = right.begin();
        lit != left.end();
        lit++, rit++
        )
      {
        if (*lit != *rit)
          return false;
      }

      return true;
    };

  if (grammar.rules.empty())
    return { };

  ParsingTable table;

  table.reserve(32);
  table.push_back({ { { &(*grammar.rules.begin()), 1 } }, { } });
  closure(grammar, table[0].first);

  for (size_t i = 0; i < table.size(); i++)
  {
    auto &item_set = table[i].first;

    for (auto it = item_set.begin(); it != item_set.end(); )
    {
      if (symbol_at_dot(*it) == 0)
      {
        auto &actions = table[i].second;
        while (it != item_set.end() && symbol_at_dot(*it) == 0)
        {
          actions.push_back({ Action::Reduce, 0, 0, it->first });

          it++;
        }
      }

      while (it != item_set.end())
      {
        int const shift_symbol = symbol_at_dot(*it);

        table.push_back({ { }, { } });

        while (it != item_set.end() && symbol_at_dot(*it) == shift_symbol)
        {
          table.back().first.insert(shift_dot(*it));

          it++;
        }

        closure(grammar, table.back().first);

        int where_to_transition = table.size() - 1;
        for (size_t j = 0; j + 1 < table.size(); j++)
        {
          if (are_item_sets_equal(table[j].first, table.back().first))
          {
            table.pop_back();
            where_to_transition = j;
            break;
          }
        }

        table[i].second.push_back({
            is_variable(shift_symbol) ? Action::Goto : Action::Shift,
            shift_symbol,
            where_to_transition,
            nullptr
          });
      }
    }
  }

  return table;
}
