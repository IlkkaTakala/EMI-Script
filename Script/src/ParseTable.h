#pragma once
#include <vector>
#include <unordered_map>
#include "Lexer.h"

struct Grammar;
struct ActionNode;

typedef std::vector<std::vector<Token>> RuleType;
struct ItemHandle {
	size_t id;
};

enum Action {
	ACCEPT, SHIFT, REDUCE, DECIDE, ERROR, GOTO
};

bool IsTerminal(Token token);

struct Rule
{
	int index;
	Token nonTerminal;
	std::vector<Token> development;
};

struct Item
{
	Item() {
		
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
	std::unordered_map<Token, int> gotos;
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
	int shift = -1;
	int reduce = -1;
};

struct Grammar
{
	Token Axiom = Start;
	std::vector<Rule> RuleTable;
	std::unordered_map<Token, std::vector<Token>> Firsts;
	std::unordered_map<Token, std::vector<Token>> Follows;

	std::vector<Kernel> ClosureKernels;
	std::vector<std::vector<ActionNode>> ParseTable;

	std::vector<Token> GetSequenceFirsts(const std::vector<Token>& sequence, int start) const;
};

Grammar& CreateParser(Grammar& g, const RuleType& rules);
void ReleaseGrammar(Grammar& g);