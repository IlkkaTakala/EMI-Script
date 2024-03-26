#pragma once
#include "Function.h"
#include "Symbol.h"
#include "Objects/UserObject.h"
#include <ankerl/unordered_dense.h>

struct Namespace
{
	std::string Name;
	Symbol* Sym = nullptr;
	ankerl::unordered_dense::map<std::string, Function*> Functions;
	ankerl::unordered_dense::map<std::string, UserDefinedType> Objects;
	ankerl::unordered_dense::map<std::string, VariableType> ObjectTypes;
	ankerl::unordered_dense::map<std::string, Symbol*> Symbols;
	std::unordered_map<std::string, Variable> Variables;

	Symbol* FindSymbol(const std::string& name) {
		if (auto it = Symbols.find(name); it == Symbols.end()) return nullptr;
		else return it->second;
	}

	inline void merge(Namespace& rhs) {
		Functions.insert(rhs.Functions.begin(), rhs.Functions.end());
		ObjectTypes.insert(rhs.ObjectTypes.begin(), rhs.ObjectTypes.end());
		Objects.insert(rhs.Objects.begin(), rhs.Objects.end());
		Symbols.insert(rhs.Symbols.begin(), rhs.Symbols.end());
		Variables.insert(rhs.Variables.begin(), rhs.Variables.end());

		rhs.Functions.clear();
		rhs.Objects.clear();
		rhs.ObjectTypes.clear();
		rhs.Symbols.clear();
		rhs.Variables.clear();

		delete Sym;
		Sym = rhs.Sym;
		rhs.Sym = nullptr;
	}

	inline void remove(Namespace& rhs) {
		for (auto& [name, func] : rhs.Functions) {
			Functions.erase(name);
		}
		for (auto& [name, func] : rhs.Objects) {
			Objects.erase(name);
		}
		for (auto& [name, func] : rhs.ObjectTypes) {
			ObjectTypes.erase(name);
		}
		for (auto& [name, func] : rhs.Symbols) {
			Symbols.erase(name);
		}
		for (auto& [name, func] : rhs.Variables) {
			Variables.erase(name);
		}
	}

	~Namespace() {
		for (auto& [name, f] : Functions) {
			delete f;
		}
		for (auto& [name, f] : Symbols) {
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
	std::vector<std::pair<std::string, std::string>> Functions;
	std::vector<std::pair<std::string, std::string>> Objects;
	std::vector<std::pair<std::string, std::string>> Variables;
};