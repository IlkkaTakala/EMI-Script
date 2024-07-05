#pragma once
#include "Function.h"
#include "Symbol.h"
#include "Objects/UserObject.h"
#include <ankerl/unordered_dense.h>

struct SymbolTable
{
	ankerl::unordered_dense::map<TName, Symbol*> Table;

	std::pair<TName, Symbol*> FindName(const TNameQuery& name) const {
		TName searchName;
		for (auto& p : name.GetPaths()) {
			for (char i = 0; i < p.Length(); i++) {
				searchName = name.GetTarget().Append(p, i);
				if (auto it = Table.find(searchName); it != Table.end()) {
					return { searchName, it->second };
				}
			}
		}
		if (auto it = Table.find(name.GetTarget()); it != Table.end()) {
			return { name.GetTarget(), it->second };
		}
		return { {}, nullptr };
	}

	Symbol* FindNameFromObject(const TName& name, const TName& path) const {
		if (auto it = Table.find(name.Append(path)); it != Table.end()) {
			return it->second;
		}
		return nullptr;
	}

	bool AddName(TName name, Symbol* symbol) {
		if (auto it = Table.find(name); it == Table.end()) {
			Table.emplace(name, symbol);
			return true;
		}
		return false;
	}
};

struct Namespace
{
	TName Name;
};

struct CompileUnit
{
	std::vector<TName> Symbols;
};