#pragma once
#include "Function.h"
#include "Object.h"
#include "Symbol.h"
#include <ankerl/unordered_dense.h>

struct Namespace
{
	std::string Name;

	ankerl::unordered_dense::map<std::string, Function> functions;
	ankerl::unordered_dense::map<std::string, UserDefined> objects;

	ankerl::unordered_dense::map<std::string, Symbol> symbols;

	Symbol* findSymbol(const std::string& name) {
		if (auto it = symbols.find(name); it == symbols.end()) return nullptr;
		else return &it->second;
	}
};

