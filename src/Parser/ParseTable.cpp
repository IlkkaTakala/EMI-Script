#include "ParseTable.h"
#include "Defines.h"
#include <stack>
#include <filesystem>
#ifdef EMI_PARSE_GRAMMAR
/*
	Translated and optimized to C++ from https://jsmachines.sourceforge.net/machines/lalr1.html
*/

bool IsTerminal(Token token) {
	return token < Token::Start && token > Token::None;
}

bool IsNonTerminal(Token token) {
	return token >= Token::Start && token < Token::Last;
}

template <typename T>
bool AddUnique(std::vector<T>& vector, const T& value) {
	if (std::find(vector.begin(), vector.end(), value) == vector.end()) {
		vector.push_back(value);
		return true;
	}
	return false;
}

static std::stack<size_t> ItemFreeIdx;
static size_t LastItem;

std::vector<Item>& Items() {
	static std::vector<Item> items;
	return items;
}

Item& Items(ItemHandle h)
{
	return Items()[h.id];
}

ItemHandle NewItem(int rule = 0, int dot = 0) {
	ItemHandle h{ 0 };
	if (ItemFreeIdx.empty()) {
		if (LastItem >= Items().size()) {
			Items().emplace_back();
		}
		h.id = LastItem++;
	}
	else {
		h.id = ItemFreeIdx.top();
		ItemFreeIdx.pop();
	}
	Items(h).lookaheads.clear();
	Items(h).lookaheads.reserve(3);
	Items(h).rule = rule;
	Items(h).dotIndex = dot;

	if (rule == 0)
		Items(h).lookaheads.push_back(Token::None);

	return h;
}

void ReleaseItem(ItemHandle h) {
	ItemFreeIdx.push(h.id);
}

bool operator==(const ItemHandle& lhs, const ItemHandle& rhs)
{
	return lhs.id == rhs.id || Items(lhs) == Items(rhs);
}

bool CollectDevelopmentFirsts(Grammar& g, const std::vector<Token>& rhs, std::vector<Token>& nonTerminalFirsts) {
	bool result = false;
	bool noneInFirsts = true;

	for (auto& s : rhs) {
		noneInFirsts = false;

		if (IsTerminal(s)) {
			result |= AddUnique(nonTerminalFirsts, s);
			break;
		}

		for (auto& k : g.Firsts[s]) {
			noneInFirsts |= k == Token::None;
			result |= AddUnique(nonTerminalFirsts, k);
		}

		if (!noneInFirsts) {
			break;
		}
	}

	if (noneInFirsts) {
		result |= AddUnique(nonTerminalFirsts, Token::None);
	}

	return result;
}

bool AddToClosure(std::vector<ItemHandle>& closure, ItemHandle item)
{
	bool result = false;
	for (auto& i : closure) {
		auto& old = Items(i);
		auto& next = Items(item);
		if (item == i) {
			for (auto& l : next.lookaheads) {
				result |= AddUnique(old.lookaheads, l);
			}
			ReleaseItem(item);
			return result;
		}
	}
	closure.push_back(item);

	return false;
}

bool NextItemAfterShift(Item& result, const Rule& r, ItemHandle h)
{
	auto& item = Items(h);
	if (item.dotIndex < r.development.size() && r.development[item.dotIndex] != Token::None) {
		result.rule = item.rule;
		result.dotIndex = item.dotIndex + 1;
		return true;
	}

	return false;
}

void Grammar::GetSequenceFirsts(std::vector<Token>& result, const std::vector<Token>& sequence, int start) const
{
	result.clear();
	result.reserve(sequence.size() - start);
	bool noneFound = true;

	for (int i = start; i < sequence.size(); ++i) {
		auto& symbol = sequence[i];
		noneFound = false;

		if (IsTerminal(symbol)) {
			AddUnique(result, symbol);
			break;
		}

		for (auto& first : Firsts.at(symbol)) {
			noneFound |= first == Token::None;
			AddUnique(result, first);
		}

		noneFound |= Firsts.at(symbol).size() == 0;

		if (!noneFound) {
			break;
		}
	}

	if (noneFound) {
		AddUnique(result, Token::None);
	}
}

void TransformGrammar(Grammar& grammar, RuleTable_t& RuleTable, const RuleType& rules) {

	RuleTable.reserve(rules.size());
	for (int i = 0; i < rules.size(); ++i) {
		RuleTable.push_back({ i, rules[i][0], std::vector<Token>{rules[i].begin() + 1, rules[i].end()} });
	}

	// Firsts
	bool notDone = true;
	do {
		notDone = false;

		for (auto& r : RuleTable) {

			std::vector<Token>& nonTerminalFirsts = grammar.Firsts[r.nonTerminal];

			if (r.development.size() == 1 && r.development[0] == Token::None) {
				notDone |= AddUnique(nonTerminalFirsts, Token::None);
			}
			else {
				notDone |= CollectDevelopmentFirsts(grammar, r.development, nonTerminalFirsts);
			}
		}
	} while (notDone);


	// Follows
	do {
		notDone = false;

		for (int i = 0; i < RuleTable.size(); ++i) {
			auto& rule = RuleTable[i];

			if (i == 0) {
				auto& nonTerminalFollows = grammar.Follows[rule.nonTerminal];
				notDone |= AddUnique(nonTerminalFollows, Token::None);
			}

			for (int j = 0; j < rule.development.size(); ++j) {
				auto& symbol = rule.development[j];

				if (IsNonTerminal(symbol)) {
					auto& symbolFollows = grammar.Follows[symbol];
					thread_local static std::vector<Token> afterSymbolFirsts;
					grammar.GetSequenceFirsts(afterSymbolFirsts, rule.development, j + 1);

					for (auto& first : afterSymbolFirsts) {

						if (first == Token::None) {
							auto& nonTerminalFollows = grammar.Follows[rule.nonTerminal];

							for (auto& l : nonTerminalFollows) {
								notDone |= AddUnique(symbolFollows, l);
							}
						}
						else {
							notDone |= AddUnique(symbolFollows, first);
						}
					}
				}
			}
		}
	} while (notDone);
}

void UpdateClosure(const Grammar& g, const RuleTable_t& RuleTable, Kernel& k) {
	for (int i = 0; i < k.closure.size(); i++) {
		auto h = k.closure[i];
		const Rule& actualRule = RuleTable[Items(h).rule];
		int dotIndex = Items(h).dotIndex;

		if (dotIndex < actualRule.development.size()) {
			Token nonTerminal = actualRule.development[dotIndex];
			for (const auto& r : RuleTable) {
				if (r.nonTerminal != nonTerminal) continue;

				auto item = NewItem(r.index, 0);


				auto& lookahead = Items(item).lookaheads;
				lookahead.clear();
				bool epsilonPresent = false;
				thread_local static std::vector<Token> firstsAfterSymbolAfterDot;
				g.GetSequenceFirsts(firstsAfterSymbolAfterDot, actualRule.development, dotIndex + 1);

				for (const auto& first : firstsAfterSymbolAfterDot) {
					if (Token::None == first) {
						epsilonPresent = true;
					}
					else {
						AddUnique(lookahead, first);
					}
				}

				if (epsilonPresent) {
					for (auto& it : Items(h).lookaheads) {
						AddUnique(lookahead, it);
					}
				}

				AddToClosure(k.closure, item);

			}
		}
	}
}

bool AddGotos(Grammar& g, RuleTable_t& RuleTable, int k) {
	bool result = false;
	ankerl::unordered_dense::map<Token, std::vector<ItemHandle>> newKernels;
	auto& kernels = g.ClosureKernels;

	for (auto& i : kernels[k].closure) {
		Item temp{ };
		
		Rule& rule = RuleTable[Items(i).rule];
		if (NextItemAfterShift(temp, rule, i)) {
			ItemHandle next = NewItem(temp.rule, temp.dotIndex);
			Items(next).lookaheads = Items(i).lookaheads;
			Token symbolAfterDot = rule.development[Items(i).dotIndex];

			AddUnique(kernels[k].keys, symbolAfterDot);
			AddToClosure(newKernels[symbolAfterDot], next);
		}
	}

	for (int i = 0; i < kernels[k].keys.size(); ++i) {
		auto key = kernels[k].keys[i];
		//Kernel newKernel{ (int)kernels.size(), newKernels[key] };
		auto& items = newKernels[key];
		int targetKernelIndex = -1;
		for (auto& kernel : kernels) {
			if (kernel.items.size() != items.size())
				continue;
			bool found = true;

			for (const auto& e : kernel.items) {
				if (std::find_if(items.begin(), items.end(), [e](const ItemHandle item) { return e == item; }) == items.end()) {
					found = false;
					break;
				}
			}
			if (found) {
				targetKernelIndex = kernel.index;
				break;
			}
		}

		if (targetKernelIndex < 0) {
			targetKernelIndex = (int)kernels.size();
			kernels.emplace_back(targetKernelIndex, items);
		}
		else {
			for (auto& item : items) {
				result |= AddToClosure(kernels[targetKernelIndex].items, item);
			}
		}

		kernels[k].gotos[key] = targetKernelIndex;
	}

	return result;
}

void CreateClosureTables(Grammar& g, RuleTable_t& RuleTable) {

	g.ClosureKernels.push_back({ 0, { NewItem() } });

	size_t i = 0;
	while (i < g.ClosureKernels.size()) {
		auto& kernel = g.ClosureKernels[i];

		UpdateClosure(g, RuleTable, kernel);

		if (AddGotos(g, RuleTable, (int)i)) {
			i = 0;
		}
		else {
			++i;
		}
	}
}

void CreateParseTable(Grammar& g, ParseTable_t& ParseTable, RuleTable_t& RuleTable) {
	ParseTable.resize(g.ClosureKernels.size());

	for (int i = 0; i < g.ClosureKernels.size(); ++i) {
		const auto& kernel = g.ClosureKernels[i];
		auto& state = ParseTable[i];
		state.resize((size_t)Token::Last);

		for (const auto& key : kernel.keys) {
			short nextState = (short)kernel.gotos.at(key);
			size_t k = (size_t)key;

			if (state[k].type != ERROR && state[k].type != ACCEPT)
				state[k].type = DECIDE;
			else
				state[k].type = SHIFT;

			state[k].shift = nextState;
		}

		for (const auto& it : kernel.closure) {
			auto& item = Items(it);
			Rule& rule = RuleTable[item.rule];
			if (item.dotIndex == rule.development.size() || rule.development[0] == Token::None) {
				for (const auto& key : item.lookaheads) {
					size_t k = (size_t)key;

					switch (state[k].type)
					{
					case REDUCE:
						if (state[k].reduce > item.rule)
							state[k].reduce = (short)item.rule;
						break;

					case SHIFT:
						state[k].type = DECIDE;
						state[k].reduce = (short)item.rule;
						break;
					case ACCEPT:
					case ERROR:
						state[k].type = item.rule == 0 ? ACCEPT : REDUCE;
						state[k].reduce = (short)item.rule;
						break;
					default:
						break;
					}

				}
			}
		}
	}
}

const char* ActionToName(Action act) {
	switch (act)
	{
	case ACCEPT:
		return "ACCEPT";
	case SHIFT:
		return "SHIFT";
	case REDUCE:
		return "REDUCE";
	case DECIDE:
		return "DECIDE";
	case ERROR:
		return "ERROR";
	case GOTO:
		return "GOTO";
	default:
		return "";
	}
}

void CreateParser(ParseTable_t& ParseTable, RuleTable_t& RuleTable, std::vector<RuleData>& Data, const RuleType& rules)
{
	Grammar g;
	TransformGrammar(g, RuleTable, rules);
	Items().reserve((size_t)pow(RuleTable.size(), 1.75f));
	CreateClosureTables(g, RuleTable);
	CreateParseTable(g, ParseTable, RuleTable);

	Items().clear();
	Items().shrink_to_fit();
	while (!ItemFreeIdx.empty()) ItemFreeIdx.pop();
	LastItem = 0;

	g.ClosureKernels.clear();
	g.ClosureKernels.shrink_to_fit();
	g.Firsts.clear();
	g.Follows.clear();

	std::ofstream outh(GRAMMAR_PATH"/grammar.h");
	std::ofstream outcpp(GRAMMAR_PATH"/grammar.cpp");

	outh << R"(#pragma once
#include "Parser/ParseTable.h"

constexpr size_t CreateTime = )";

	auto time = std::chrono::file_clock::now().time_since_epoch().count();
	outh << time << "u; \n\n";

	outh << R"(
int GetIndex(int state, int token);
extern ActionNode* ParseTable;
extern RuleTable_t RuleTable;
extern std::vector<RuleData> Data;
)";

	outcpp << R"(#include "grammar.h"
RuleTable_t RuleTable = {
)";

	for (auto& rule : RuleTable) {
		outcpp << "{ " << rule.index << ", ";
		outcpp << "Token::" << TokensToName[rule.nonTerminal] << ", ";
		outcpp << "{ ";
		for (auto& t : rule.development) {
			outcpp << "Token::" << TokensToName[t] << ", ";
		}
		outcpp << " }}, \n";
	}

	outcpp << R"(};

int Width = )";
	if (ParseTable.size() > 0) {
		outcpp << ParseTable[0].size();
	}
	else {
		outcpp << 0;
	}

	outcpp << R"(;
int Height = )";
	outcpp << ParseTable.size();

	outcpp << R"(;

int GetIndex(int state, int token)
{
	return state * Width + token;
}

ActionNode* ParseTable = new ActionNode[Width * Height] {
)";
	for (auto& row : ParseTable) {
		for (auto& action : row) {
			outcpp << "{ " << ActionToName(action.type) << ", " << action.shift << ", " << action.reduce << " }, ";
		}
		outcpp << "\n";
	}

outcpp << R"(};

std::vector<RuleData> Data = {
)";
	for (auto& row : Data) {
		outcpp << "{ ";
		outcpp << "Token::" << TokensToName[row.dataToken] << ", Token::" << TokensToName[row.nodeType] << ", Token::" << TokensToName[row.mergeToken];
		outcpp << " }, \n";
	}

	outcpp << "};\n";

}

#endif