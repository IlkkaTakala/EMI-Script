#pragma once
#include <vector>
#include <unordered_map>
#include "Lexer.h"

struct Grammar;
struct ActionNode;

typedef std::vector<std::vector<Token>> RuleType;

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
	Item(int r, int index) : rule(r), dotIndex(index) {
		if (r == 0)
			lookaheads.push_back(None);
		Count++;
	}
	~Item() {
		Count--;
	}

	static int Count;
	int rule;
	int dotIndex;
	std::vector<Token> lookaheads;

	bool operator==(const Item& rhs) const {

		return rule == rhs.rule && dotIndex == rhs.dotIndex;
	}

	std::vector<Item*> TokensAfterDot(const Grammar& g) const;

	bool AddToClosure(std::vector<Item*>& closure);

	bool NextItemAfterShift(Item& result, const Rule& r) const;
};

struct Kernel
{
	Kernel(int i, const std::vector<Item*>& itemList) : index(i), items(itemList), closure(itemList) {}

	int index;
	std::vector<Item*> items;
	std::vector<Item*> closure;
	std::unordered_map<Token, int> gotos;
	std::vector<Token> keys;

	bool operator==(const Kernel& rhs) const {
		if (items.size() != rhs.items.size())
			return false;

		for (const auto& e : items) {
			if (std::find_if(rhs.items.begin(), rhs.items.end(), [e](const Item* item) { return *e == *item; }) == rhs.items.end()) {
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