#pragma once
#include "Function.h"
#include "Symbol.h"
#include "Objects/UserObject.h"
#include <ankerl/unordered_dense.h>

struct Namespace
{
	Namespace* Parent;
	TName Name;
	ankerl::unordered_dense::map<TName, Symbol*> Symbols;

	Symbol* FindSymbol(const TName& name) {
		if (auto it = Symbols.find(name); it == Symbols.end()) return nullptr;
		else return it->second;
	}

	inline void merge(Namespace& rhs) {
		Symbols.insert(rhs.Symbols.begin(), rhs.Symbols.end());
		rhs.Symbols.clear();
	}

	inline void remove(Namespace& rhs) {
		for (auto& [name, func] : rhs.Symbols) {
			Symbols.erase(name);
		}
	}

	~Namespace() {
		for (auto& [name, f] : Symbols) {
			delete f;
		}
	}
	// @todo: check these
	Namespace() = default;
	Namespace(Namespace&& rhs) noexcept {
		merge(rhs);
	};
	Namespace(Namespace& rhs) = delete;
	Namespace& operator=(Namespace& rhs) {
		merge(rhs);
		return *this;
	};
	Namespace& operator=(Namespace&& rhs) noexcept {
		merge(rhs);
		return *this;
	}
};

struct CompileUnit
{
	std::vector<std::pair<std::string, std::string>> Functions;
	std::vector<std::pair<std::string, std::string>> Objects;
	std::vector<std::pair<std::string, std::string>> Variables;
};