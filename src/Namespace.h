#pragma once
#include "Symbol.h"
#include <ankerl/unordered_dense.h>

struct SymbolTable
{
	ankerl::unordered_dense::map<PathType, Symbol*> Table;

	std::pair<PathType, Symbol*> FindName(const PathTypeQuery& name) const {
		PathType searchName;
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

	Symbol* FindNameFromObject(const PathType& name, const PathType& path) const {
		if (auto it = Table.find(name.Append(path)); it != Table.end()) {
			return it->second;
		}
		return nullptr;
	}

	bool AddName(PathType name, Symbol* symbol) {
		if (auto it = Table.find(name); it == Table.end()) {
			Table.emplace(name, symbol);
			return true;
		}
		return false;
	}
};

struct Namespace
{
	PathType Name;
};

struct ScriptFunction;
struct CompileUnit
{
	// @todo: we need to track function overloads too
	std::vector<PathType> Symbols;
	ScriptFunction* InitFunction;
};