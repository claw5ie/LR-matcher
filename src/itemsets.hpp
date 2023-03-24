#ifndef ITEMSETS_HPP
#define ITEMSETS_HPP

#define LOWEST_VAR_IDX 256

using sym_t = uint32_t;

enum TokenType
{
  Token_Variable,
  Token_Term_Seq,
  Token_Colon,
  Token_Semicolon,
  Token_Bar,
  Token_End_Of_File,
};

struct Token
{
  TokenType type;
  View<char> text;
  size_t line, column;
};

using GrammarRule = std::vector<sym_t>;

struct Grammar
{
  std::set<GrammarRule> rules;
  std::vector<std::string> lookup;

  std::vector<Token> tokens;
  size_t current, line, column;

  void
  advance()
  {
    assert(current < tokens.size());
    ++current;
  }
};

enum ActionType
{
  Action_Shift,
  Action_Goto,
  Action_Reduce,
};

struct Action
{
  ActionType type;
  // Transition label when shifting.
  sym_t label;
  // Where to transition to.
  size_t dst;
  GrammarRule const *reduce_to;
};

struct Item
{
  const GrammarRule *rule;
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
