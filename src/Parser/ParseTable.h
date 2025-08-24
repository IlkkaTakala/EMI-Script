#pragma once
#include <vector>
#include "Lexer.h"
#include "Defines.h"

struct Grammar;
struct ActionNode;

typedef std::vector<std::vector<Token>> RuleType;
struct ItemHandle {
	size_t id;
};

enum Action : uint8_t {
	ACCEPT, SHIFT, REDUCE, DECIDE, ERROR, GOTO
};

bool IsTerminal(Token token);

struct Rule
{
	int index;
	Token nonTerminal;
	std::vector<Token> development;
};

struct RuleData
{
	Token dataToken = Token::None;
	Token nodeType = Token::None;
	Token mergeToken = Token::None;
};

struct Item
{
	Item() {
		dotIndex = 0;
		rule = 0;
	}

	int rule;
	int dotIndex;
	std::vector<Token> lookaheads;

	bool operator==(const Item& rhs) const {

		return rule == rhs.rule && dotIndex == rhs.dotIndex;
	}
};

std::vector<Item>& Items();
Item& Items(ItemHandle);

bool operator==(const ItemHandle& lhs, const ItemHandle& rhs);

struct Kernel
{
	Kernel(int i, const std::vector<ItemHandle>& itemList) : index(i), items(itemList), closure(itemList) {}

	int index;
	std::vector<ItemHandle> items;
	std::vector<ItemHandle> closure;
	ankerl::unordered_dense::map<Token, int> gotos;
	std::vector<Token> keys;

	bool operator==(const Kernel& rhs) const {
		if (items.size() != rhs.items.size())
			return false;

		for (const auto& e : items) {
			if (std::find_if(rhs.items.begin(), rhs.items.end(), [e](const ItemHandle item) { return e == item; }) == rhs.items.end()) {
				return false;
			}
		}
		return true;
	}
};

struct ActionNode
{
	Action type = ERROR;
	short shift = -1;
	short reduce = -1;
};

typedef std::vector<std::vector<ActionNode>> ParseTable_t;
typedef std::vector<Rule> RuleTable_t;

struct Grammar
{
	Token Axiom = Token::Start;
	ankerl::unordered_dense::map<Token, std::vector<Token>> Firsts;
	ankerl::unordered_dense::map<Token, std::vector<Token>> Follows;
	std::vector<Kernel> ClosureKernels;

	void GetSequenceFirsts(std::vector<Token>& result, const std::vector<Token>& sequence, int start) const;
};

void CreateParser(ParseTable_t& ParseTable, RuleTable_t& RuleTable, std::vector<RuleData>& Data, const RuleType& rules);
