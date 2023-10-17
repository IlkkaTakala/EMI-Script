#include "ParseTable.h"

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

bool CollectDevelopmentFirsts(Grammar& g, const std::vector<Token>& rhs, std::vector<Token>& nonTerminalFirsts) {
	bool result = false;
	bool epsilonInSymbolFirsts = true;

	for (auto& s : rhs) {
		epsilonInSymbolFirsts = false;

		if (IsTerminal(s)) {
			result |= AddUnique(nonTerminalFirsts, s);
			break;
		}

		for (auto& k : g.Firsts[s]) {
			epsilonInSymbolFirsts |= k == None;
			result |= AddUnique(nonTerminalFirsts, k);
		}

		if (!epsilonInSymbolFirsts) {
			break;
		}
	}

	if (epsilonInSymbolFirsts) {
		result |= AddUnique(nonTerminalFirsts, None);
	}

	return result;
}

std::vector<Item> Item::TokensAfterDot(const Grammar& g) const
{
	std::vector<Item> result = {};
	const Rule& actualRule = g.RuleTable[rule];

	if (dotIndex < actualRule.development.size()) {
		Token nonTerminal = actualRule.development[dotIndex];
		for (const auto& r : g.RuleTable) {
			if (r.nonTerminal != nonTerminal) continue;

			AddUnique(result, Item{ r.index, 0 });
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
		for (auto& i : lookaheads) {
			AddUnique(newLookAheads, i);
		}
	}

	for (auto& i : result) {
		i.lookaheads = newLookAheads;
	}

	return result;
}

bool Item::AddToClosure(std::vector<Item>& closure) const
{
	bool result = false;
	for (auto& item : closure) {
		if (*this == item) {
			for (auto& l : lookaheads) {
				result |= AddUnique(item.lookaheads, l);
			}
			return result;
		}
	}
	closure.push_back(*this);

	return false;
}

bool Item::NextItemAfterShift(Item& result, const Rule& r) const
{
	if (dotIndex < r.development.size() && r.development[dotIndex] != None) {
		result = { rule, dotIndex + 1 };
		result.lookaheads = lookaheads;

		return true;
	}

	return false;
}

std::vector<Token> Grammar::GetSequenceFirsts(const std::vector<Token>& sequence, int start) const
{
	std::vector<Token> result = {};
	result.reserve(sequence.size() - start);
	bool epsilonInSymbolFirsts = true;

	for (int i = start; i < sequence.size(); ++i) {
		auto& symbol = sequence[i];
		epsilonInSymbolFirsts = false;

		if (IsTerminal(symbol)) {
			AddUnique(result, symbol);
			break;
		}

		for (auto& first : Firsts.at(symbol)) {
			epsilonInSymbolFirsts |= first == None;
			AddUnique(result, first);
		}

		epsilonInSymbolFirsts |= (Firsts.find(symbol) == Firsts.end() || Firsts.at(symbol).size() == 0);

		if (!epsilonInSymbolFirsts) {
			break;
		}
	}

	if (epsilonInSymbolFirsts) {
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
		auto nextItems = k.closure[i].TokensAfterDot(g);

		for (const auto& n : nextItems) {
			n.AddToClosure(k.closure);
		}
	}
}

bool AddGotos(Grammar& g, int k) {
	bool result = false;
	std::unordered_map<Token, std::vector<Item>> newKernels;
	auto& kernels = g.ClosureKernels;

	for (auto& item : kernels[k].closure) {
		Item next{ 0, 0 };
		
		Rule& rule = g.RuleTable[item.rule];
		if (item.NextItemAfterShift(next, rule)) {
			Token symbolAfterDot = rule.development[item.dotIndex];

			AddUnique(kernels[k].keys, symbolAfterDot);
			next.AddToClosure(newKernels[symbolAfterDot]);
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
				result |= item.AddToClosure(kernels[targetKernelIndex].items);
			}
		}

		kernels[k].gotos[key] = targetKernelIndex;
	}

	return result;
}

void CreateClosureTables(Grammar& g) {

	g.ClosureKernels.push_back({ 0, {Item{0, 0}} });

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

		for (const auto& item : kernel.closure) {
			Rule& rule = g.RuleTable[item.rule];
			if (item.dotIndex == rule.development.size() || rule.development[0] == None) {
				for (const auto& k : item.lookaheads) {
					switch (state[k].type)
					{
					case REDUCE:
						printf("Reduce-reduce error in state %d, token %d\n", i, k);
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

Grammar CreateParser(const RuleType& rules)
{
	Grammar g;
	TransformGrammar(g, rules);
	CreateClosureTables(g);
	CreateParseTable(g);

	return g;
};