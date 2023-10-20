#include "ParseTable.h"
#include <stack>

bool IsTerminal(Token token) {
	return token < Start && token > None;
}

bool IsNonTerminal(Token token) {
	return token >= Start && token < Last;
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
		Items(h).lookaheads.push_back(None);

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
			noneInFirsts |= k == None;
			result |= AddUnique(nonTerminalFirsts, k);
		}

		if (!noneInFirsts) {
			break;
		}
	}

	if (noneInFirsts) {
		result |= AddUnique(nonTerminalFirsts, None);
	}

	return result;
}

std::vector<ItemHandle> TokensAfterDot(const Grammar& g, ItemHandle h)
{
	std::vector<ItemHandle> result = {};
	const Rule& actualRule = g.RuleTable[Items(h).rule];
	int dotIndex = Items(h).dotIndex;

	if (dotIndex < actualRule.development.size()) {
		Token nonTerminal = actualRule.development[dotIndex];
		for (const auto& r : g.RuleTable) {
			if (r.nonTerminal != nonTerminal) continue;

			auto item = NewItem(r.index, 0);
			if (!AddUnique(result, item))
				ReleaseItem(item);
		}
	}

	if (result.size() == 0) {
		return result;
	}

	std::vector<Token> newLookAheads = {};
	bool epsilonPresent = false;
	auto firstsAfterSymbolAfterDot = g.GetSequenceFirsts(actualRule.development, dotIndex + 1);

	for (const auto& first : firstsAfterSymbolAfterDot) {
		if (None == first) {
			epsilonPresent = true;
		}
		else {
			AddUnique(newLookAheads, first);
		}
	}

	if (epsilonPresent) {
		for (auto& i : Items(h).lookaheads) {
			AddUnique(newLookAheads, i);
		}
	}

	for (auto& i : result) {
		Items(i).lookaheads = newLookAheads;
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
	int dotIndex = Items(h).dotIndex;
	if (dotIndex < r.development.size() && r.development[dotIndex] != None) {
		result.rule = Items(h).rule;
		result.dotIndex = dotIndex + 1;
		result.lookaheads = Items(h).lookaheads;

		return true;
	}

	return false;
}

std::vector<Token> Grammar::GetSequenceFirsts(const std::vector<Token>& sequence, int start) const
{
	std::vector<Token> result = {};
	result.reserve(sequence.size() - start);
	bool noneInFirsts = true;

	for (int i = start; i < sequence.size(); ++i) {
		auto& symbol = sequence[i];
		noneInFirsts = false;

		if (IsTerminal(symbol)) {
			AddUnique(result, symbol);
			break;
		}

		for (auto& first : Firsts.at(symbol)) {
			noneInFirsts |= first == None;
			AddUnique(result, first);
		}

		noneInFirsts |= (Firsts.find(symbol) == Firsts.end() || Firsts.at(symbol).size() == 0);

		if (!noneInFirsts) {
			break;
		}
	}

	if (noneInFirsts) {
		AddUnique(result, None);
	}

	return result;
}

void TransformGrammar(Grammar& grammar, const RuleType& rules) {

	grammar.RuleTable.reserve(rules.size());
	for (int i = 0; i < rules.size(); ++i) {
		grammar.RuleTable.push_back({ i, rules[i][0], std::vector<Token>{rules[i].begin() + 1, rules[i].end()} });
	}

	// Firsts
	bool notDone = true;
	do {
		notDone = false;

		for (auto& r : grammar.RuleTable) {

			std::vector<Token>& nonTerminalFirsts = grammar.Firsts[r.nonTerminal];

			if (r.development.size() == 1 && r.development[0] == None) {
				notDone |= AddUnique(nonTerminalFirsts, None);
			}
			else {
				notDone |= CollectDevelopmentFirsts(grammar, r.development, nonTerminalFirsts);
			}
		}
	} while (notDone);


	// Follows
	do {
		notDone = false;

		for (int i = 0; i < grammar.RuleTable.size(); ++i) {
			auto& rule = grammar.RuleTable[i];

			if (i == 0) {
				auto& nonTerminalFollows = grammar.Follows[rule.nonTerminal];
				notDone |= AddUnique(nonTerminalFollows, None);
			}

			for (int j = 0; j < rule.development.size(); ++j) {
				auto& symbol = rule.development[j];

				if (IsNonTerminal(symbol)) {
					auto& symbolFollows = grammar.Follows[symbol];
					auto afterSymbolFirsts = grammar.GetSequenceFirsts(rule.development, j + 1);

					for (auto& first : afterSymbolFirsts) {

						if (first == None) {
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

void UpdateClosure(const Grammar& g, Kernel& k) {
	for (int i = 0; i < k.closure.size(); i++) {
		auto nextItems = TokensAfterDot(g, k.closure[i]);

		for (auto& n : nextItems) {
			AddToClosure(k.closure, n);
		}
	}
}

bool AddGotos(Grammar& g, int k) {
	bool result = false;
	std::unordered_map<Token, std::vector<ItemHandle>> newKernels;
	auto& kernels = g.ClosureKernels;

	for (auto& i : kernels[k].closure) {
		Item temp{ };
		
		Rule& rule = g.RuleTable[Items(i).rule];
		if (NextItemAfterShift(temp, rule, i)) {
			ItemHandle next = NewItem(temp.rule, temp.dotIndex);
			Items(next).lookaheads = temp.lookaheads;
			Token symbolAfterDot = rule.development[Items(i).dotIndex];

			AddUnique(kernels[k].keys, symbolAfterDot);
			AddToClosure(newKernels[symbolAfterDot], next);
		}
	}

	for (int i = 0; i < kernels[k].keys.size(); ++i) {
		auto key = kernels[k].keys[i];
		Kernel newKernel{ (int)kernels.size(), newKernels[key] };
		int targetKernelIndex = -1;
		if (auto it = std::find(kernels.begin(), kernels.end(), newKernel); it != kernels.end()) {
			targetKernelIndex = int(it - kernels.begin());
		}

		if (targetKernelIndex < 0) {
			kernels.push_back(newKernel);
			targetKernelIndex = newKernel.index;
		}
		else {
			for (auto& item : newKernel.items) {
				result |= AddToClosure(kernels[targetKernelIndex].items, item);
			}
		}

		kernels[k].gotos[key] = targetKernelIndex;
	}

	return result;
}

void CreateClosureTables(Grammar& g) {

	g.ClosureKernels.push_back({ 0, { NewItem() } });

	size_t i = 0;
	while (i < g.ClosureKernels.size()) {
		auto& kernel = g.ClosureKernels[i];

		UpdateClosure(g, kernel);

		if (AddGotos(g, (int)i)) {
			i = 0;
		}
		else {
			++i;
		}
	}
}

void CreateParseTable(Grammar& g) {
	g.ParseTable.resize(g.ClosureKernels.size());

	for (int i = 0; i < g.ClosureKernels.size(); ++i) {
		const auto& kernel = g.ClosureKernels[i];
		auto& state = g.ParseTable[i];
		state.resize(Last);

		for (const auto& key : kernel.keys) {
			int nextState = kernel.gotos.at(key);

			if (state[key].type != ERROR && state[key].type != ACCEPT)
				state[key].type = DECIDE;
			else
				state[key].type = SHIFT;

			state[key].shift = nextState;
		}

		for (const auto& it : kernel.closure) {
			auto& item = Items(it);
			Rule& rule = g.RuleTable[item.rule];
			if (item.dotIndex == rule.development.size() || rule.development[0] == None) {
				for (const auto& k : item.lookaheads) {
					switch (state[k].type)
					{
					case REDUCE:
						printf("Reduce-reduce error in state %d, token %d. Rules %d and %d\n", i, (int)k, state[k].reduce, item.rule);
						if (state[k].reduce > item.rule)
							state[k].reduce = item.rule;
						break;

					case SHIFT:
						state[k].type = DECIDE;
						state[k].reduce = item.rule;
						break;
					case ACCEPT:
					case ERROR:
						state[k].type = item.rule == 0 ? ACCEPT : REDUCE;
						state[k].reduce = item.rule;
						break;
					default:
						break;
					}

				}
			}
		}
	}
}

Grammar& CreateParser(Grammar& g, const RuleType& rules)
{
	TransformGrammar(g, rules);
	Items().reserve((size_t)pow(g.RuleTable.size(), 1.75f));
	CreateClosureTables(g);
	CreateParseTable(g);

	return g;
}

void ReleaseGrammar(Grammar& g)
{
	Items().clear();
	while (!ItemFreeIdx.empty()) ItemFreeIdx.pop();
	LastItem = 0;

	g.ClosureKernels.clear();
	g.Firsts.clear();
	g.Follows.clear();
	g.RuleTable.clear();
	g.ParseTable.clear();
};