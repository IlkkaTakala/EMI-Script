#pragma once
#include "Function.h"
#include "Symbol.h"
#include "Objects/UserObject.h"
#include <ankerl/unordered_dense.h>

struct Namespace
{
	std::string Name;
	Symbol* Sym = nullptr;
	ankerl::unordered_dense::map<std::string, Function*> functions;
	ankerl::unordered_dense::map<std::string, UserDefined> objects;
	ankerl::unordered_dense::map<std::string, VariableType> objectTypes;
	ankerl::unordered_dense::map<std::string, Symbol*> symbols;
	std::unordered_map<std::string, Variable> variables;

	Symbol* findSymbol(const std::string& name) {
		if (auto it = symbols.find(name); it == symbols.end()) return nullptr;
		else return it->second;
	}

	inline void merge(Namespace& rhs) {
		functions.insert(rhs.functions.begin(), rhs.functions.end());
		objectTypes.insert(rhs.objectTypes.begin(), rhs.objectTypes.end());
		symbols.insert(rhs.symbols.begin(), rhs.symbols.end());
		variables.insert(rhs.variables.begin(), rhs.variables.end());

		rhs.functions.clear();
		rhs.objects.clear();
		rhs.symbols.clear();
		rhs.variables.clear();

		delete Sym;
		Sym = rhs.Sym;
		rhs.Sym = nullptr;
	}

	inline void remove(Namespace& rhs) {
		for (auto& [name, func] : rhs.functions) {
			functions.erase(name);
		}
		for (auto& [name, func] : rhs.objects) {
			objects.erase(name);
		}
		for (auto& [name, func] : rhs.objectTypes) {
			objectTypes.erase(name);
		}
		for (auto& [name, func] : rhs.symbols) {
			symbols.erase(name);
		}
		for (auto& [name, func] : rhs.variables) {
			variables.erase(name);
		}
	}

	~Namespace() {
		for (auto& [name, f] : functions) {
			delete f;
		}
		for (auto& [name, f] : symbols) {
			delete f;
		}
		delete Sym;
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
	std::vector<std::pair<std::string, std::string>> functions;
	std::vector<std::pair<std::string, std::string>> objects;
	std::vector<std::pair<std::string, std::string>> variables;
};