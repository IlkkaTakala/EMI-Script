#pragma once
#include "Defines.h"
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>

#include "Symbol.h"

struct Scoped
{
	ankerl::unordered_dense::map<std::string, Symbol> symbols;
	Scoped* parent = nullptr;
	std::list<Scoped> children;

	Symbol* findSymbol(const std::string& name) {
		if (auto it = symbols.find(name); it != symbols.end()) {
			return &it->second;
		}
		if (!parent) return nullptr;
		return parent->findSymbol(name);
	}

	Symbol* addSymbol(const std::string& name) {
		auto it = findSymbol(name);
		if (!it) {
			auto s = symbols.emplace(name, Symbol{});
			return &s.first->second;
		}
		return nullptr;
	}
};

struct Function
{
	std::string Name;
	std::string Path;

	ankerl::unordered_dense::set<std::string> stringTable;
	ankerl::unordered_dense::set<double> numberTable;

	Scoped* scope;


	uint8 ArgCount;

	std::vector<uint8> Bytecode;

	Function() {
		scope = nullptr;
		ArgCount = 0;
	}
	~Function() {
		delete scope;
	}
};