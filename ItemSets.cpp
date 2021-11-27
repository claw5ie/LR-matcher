#include "ItemSets.hpp"

bool is_variable(int symbol)
{
  return symbol < 0;
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
    {
      return left.first < right.first;
    }
    else
    {
      return left.second < right.second;
    }
  }
  else
  {
    return sym_left < sym_right;
  }
}

void closure(Grammar const &grammar, ItemSet &item_set)
{
  std::vector<int> to_visit; to_visit.reserve(32);

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

  for (auto const &item : item_set)
  {
    int const symbol = symbol_at_dot(item);
    if (is_variable(symbol) && should_visit(symbol, to_visit))
      to_visit.push_back(symbol);
  }

  for (size_t i = 0; i < to_visit.size(); i++)
  {
    int const symbol = to_visit[i];

    for (
      auto it = grammar.lower_bound({ symbol });
      it != grammar.end() && (*it)[0] == symbol;
      it++
      )
    {
      item_set.emplace(&(*it), 1);

      int const new_candidate = (*it)[1];
      if (is_variable(new_candidate) && should_visit(new_candidate, to_visit))
        to_visit.push_back(new_candidate);
    }
  }
}

ParsingTable find_item_sets(Grammar const &grammar)
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

  if (grammar.empty())
    return { };

  std::vector<ItemSet> item_sets; item_sets.reserve(32);
  // Warning: access out of bound if there are more than 64 states
  std::vector<std::list<Action>> table; table.resize(64);

  item_sets.emplace_back(ItemSet{ { &(*grammar.begin()), 1 } });

  closure(grammar, item_sets[0]);

  for (size_t i = 0; i < item_sets.size(); i++)
  {
    auto &curr_set = item_sets[i];

    for (auto it = curr_set.begin(); it != curr_set.end(); )
    {
      int const symbol = symbol_at_dot(*it);
      if (symbol == 0)
      {
        while (it != curr_set.end() && symbol_at_dot(*it) == 0)
        {
          table[i].push_back({ Action::Reduce, 0, 0, it->first });

          it++;
        }
      }

      while (it != curr_set.end())
      {
        int const shift_symbol = symbol_at_dot(*it);

        item_sets.emplace_back(ItemSet{ });

        while (it != curr_set.end() && symbol_at_dot(*it) == shift_symbol)
        {
          item_sets.back().insert(shift_dot(*it));

          it++;
        }

        closure(grammar, item_sets.back());

        int where_to_transition = item_sets.size() - 1;
        for (size_t j = 0; j + 1 < item_sets.size(); j++)
        {
          if (are_item_sets_equal(item_sets[j], item_sets.back()))
          {
            item_sets.pop_back();
            where_to_transition = j;
            break;
          }
        }

        table[i].push_back({
            is_variable(shift_symbol) ? Action::Goto : Action::Shift,
            shift_symbol,
            where_to_transition,
            nullptr
          });
      }
    }
  }

  return { item_sets, table };
}
