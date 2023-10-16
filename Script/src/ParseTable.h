#pragma once
#include <vector>
#include "Lexer.h"

https://softwareengineering.stackexchange.com/questions/178187/how-to-add-precedence-to-lalr-parser-like-in-yacc

std::vector<std::vector<Token>> Rules = {
	{Start, Stmt, Semi},
	{Stmt, Expr, Assign, Expr},
	{Expr, Arithmetic},
	{Arithmetic, Expr, Add, Expr},
	{Expr, Value},
	{Value, Integer},
};

enum Action {
	ACCEPT, SHIFT, REDUCE, ERROR, CONFLICT
};

struct Rule
{
	int index;
	int lhs;
	std::vector<int> rhs;

	bool operator==(const Rule& rhs) const {
		return index == rhs.index && lhs == rhs.lhs;
	}
};

struct ActionNode
{
	Action type = ERROR;
	int state = -1;
	std::vector<ActionNode> actions;
	Rule rule;
};

struct CollectionItem
{
	Rule rule;
	int position;
	int lookahead;

	bool operator==(const CollectionItem& rhs) const {
		return rule == rhs.rule && position == rhs.position && lookahead == rhs.lookahead;
	}
};

typedef std::vector<CollectionItem> Collection;

std::vector<Token> _symbols;
std::vector<std::vector<Token>> rulesSymbols;
std::vector<Collection> _collection;
std::vector<Rule> _rules;
int _endMarker;
bool _parseTableHasConflict;
std::vector<std::vector<ActionNode>> _parseTable;
int _startSymbol = 0;
int _symbolsTerminalOffset;

bool isTerminal(int symbol) {
	return symbol >= _symbolsTerminalOffset && symbol < _symbols.size() - 1;
};

bool isTerminalOrEndMarker(int symbol) {
	return symbol >= _symbolsTerminalOffset && symbol < _symbols.size();
};

bool actionsEqual(const ActionNode& lhs, const ActionNode& rhs) {
	return lhs.type == rhs.type && lhs.state == rhs.state;
}

std::vector<int> first(int symbol) {
	if (isTerminalOrEndMarker(symbol)) {
		return { symbol };
	}
	std::vector<int> f = {};
	for (int i = 0; i < _rules.size(); ++i) {
		if (_rules[i].lhs == symbol && _rules[i].rhs[0] != symbol) {
			auto fn = first(_rules[i].rhs[0]);
			for (int j = 0; j < fn.size(); ++j) {
				f.push_back(fn[j]);
			}
		}
	}
	return f;
};

template <typename T>
Collection closure(const T& set) {
	Collection closureSet = set;
	bool newItemAdded = true;
	while (newItemAdded) {
		newItemAdded = false;
		for (int i = 0; i < closureSet.size(); ++i) {
			int symbol = closureSet[i].rule.rhs.size() > closureSet[i].position ? closureSet[i].rule.rhs[closureSet[i].position] : -1;
			for (int j = 0; j < _rules.size(); ++j) {
				if (_rules[j].lhs == symbol) {
					auto f = first(
						closureSet[i].position + 1 < closureSet[i].rule.rhs.size() ?
						closureSet[i].rule.rhs[closureSet[i].position + 1] :
						closureSet[i].lookahead);
					for (int k = 0; k < f.size(); ++k) {
						CollectionItem item = {
							_rules[j],
							0,
							f[k]
						};
						bool newItem = true;
						for (int m = 0; m < closureSet.size(); ++m) {
							if (closureSet[m] == item) {
								newItem = false;
								break;
							}
						}
						if (newItem) {
							closureSet.push_back(item);
							newItemAdded = true;
						}
					}
				}
			}
		}
	}
	return closureSet;
};

template <typename T>
Collection goTo(const T& set, int symbol) {
	T gotoSet = {};
	for (int i = 0; i < set.size(); ++i) {
		if (set[i].rule.rhs.size() > set[i].position && set[i].rule.rhs[set[i].position] == symbol) {
			gotoSet.push_back({
				set[i].rule,
				set[i].position + 1,
				set[i].lookahead
			});
		}
	}
	return closure(gotoSet);
};

void createCollection() {
	_collection = { closure(Collection{{
		_rules[0],
		0,
		_endMarker
	} }) };
	bool newSetAdded = true;
	while (newSetAdded) {
		newSetAdded = false;
		for (int i = 0; i < _collection.size(); ++i) {
			for (int symbol = 1; symbol < _symbols.size() - 1; ++symbol) {
				auto set = goTo(_collection[i], symbol);
				if (set.size() != 0) {
					bool newSet = true;
					for (int j = 0; j < _collection.size(); ++j) {
						if (_collection[j] == set) {
							newSet = false;
							break;
						}
					}
					if (newSet) {
						_collection.push_back(set);
						newSetAdded = true;
					}
				}
			}
		}
	}
};

void addActionToParseTable(int state, int symbol, ActionNode action) {
	if (_parseTable[state][symbol].state == -1) {
		_parseTable[state][symbol] = action;
	}
	else if (_parseTable[state][symbol].type == CONFLICT) {
		bool newAction = true;
		for (int i = 0; i < _parseTable[state][symbol].actions.size(); ++i) {
			if (actionsEqual(_parseTable[state][symbol].actions[i], action)) {
				newAction = false;
				break;
			}
		}
		if (newAction) {
			_parseTable[state][symbol].actions.push_back(action);
		}
	}
	else if (!actionsEqual(_parseTable[state][symbol], action)) {
		_parseTable[state][symbol] = {
			CONFLICT,
			-1,
			{_parseTable[state][symbol], action}
		};
		_parseTableHasConflict = true;
	}
};

void createParseTable() {
	_parseTable.resize(_collection.size());
	_parseTableHasConflict = false;
	for (int i = 0; i < _collection.size(); ++i) {
		_parseTable[i].resize(_symbols.size());
		for (int j = 0; j < _collection[i].size(); ++j) {
			int symbol = _collection[i][j].rule.rhs.size() > _collection[i][j].position ? _collection[i][j].rule.rhs[_collection[i][j].position] : -1;
			if (isTerminal(symbol)) {
				auto gotoSet = goTo(_collection[i], symbol);
				for (int k = 0; k < _collection.size(); ++k) {
					if (_collection[k] == gotoSet) {
						addActionToParseTable(i, symbol, {
							SHIFT,
							k
						});
					}
				}
			}
			if (_collection[i][j].position == _collection[i][j].rule.rhs.size()) {
				if (_collection[i][j].rule.lhs != _startSymbol) {
					addActionToParseTable(i, _collection[i][j].lookahead, {
						REDUCE,
						-1,
						{},
						_collection[i][j].rule
					});
				}
				else if (_collection[i][j].lookahead == _endMarker) {
					_parseTable[i][_endMarker].type = ACCEPT;
				}
			}
		}
		for (int symbol = 1; symbol < _symbolsTerminalOffset; ++symbol) {
			auto gotoSet = goTo(_collection[i], symbol);
			for (int k = 0; k < _collection.size(); ++k) {
				if (_collection[k] == gotoSet) {
					_parseTable[i][symbol].state = k;
				}
			}
		}
	}
};

template <typename T>
int find_index(const std::vector<T>& vector, const T& val) {
	if (auto it = std::find(vector.begin(), vector.end(), val); it != vector.end())
		return int(it - vector.begin());
	return 0;
}

auto CreateParser() {
	const auto& lines = Rules;
	rulesSymbols.resize(lines.size());
	for (int i = 0; i < lines.size(); ++i) {
		rulesSymbols[i] = lines[i];
		if (std::find(_symbols.begin(), _symbols.end(), rulesSymbols[i][0]) == _symbols.end()) {
			_symbols.push_back(rulesSymbols[i][0]);
		}
	}
	int _symbolsTerminalOffset = (int)_symbols.size();
	for (int i = 0; i < rulesSymbols.size(); ++i) {
		for (int j = 0; j < rulesSymbols[i].size(); ++j) {
			if (std::find(_symbols.begin(), _symbols.end(), rulesSymbols[i][j]) == _symbols.end()) {
				_symbols.push_back(rulesSymbols[i][j]);
			}
		}
	}
	_endMarker = (int)_symbols.size();
	_symbols.push_back(None);
	_rules.resize(rulesSymbols.size());
	for (int i = 0; i < rulesSymbols.size(); ++i) {
		_rules[i].index = i;
		for (int j = 0; j < rulesSymbols[i].size(); ++j) {
			if (j == 0) {
				_rules[i].lhs = find_index(_symbols, rulesSymbols[i][j]);
			}
			else {
				_rules[i].rhs.push_back(find_index(_symbols, rulesSymbols[i][j]));
			}
		}
	}
	createCollection();
	createParseTable();

	return _parseTable;
};