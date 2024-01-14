#pragma once
#include "Function.h"
#include "Object.h"
#include "Symbol.h"
#include <ankerl/unordered_dense.h>

struct Namespace
{
	std::string Name;
	ankerl::unordered_dense::map<const char*, Function*> functions;
	ankerl::unordered_dense::map<std::string, UserDefined> objects;
	ankerl::unordered_dense::map<std::string, ObjectType> objectTypes;
	ankerl::unordered_dense::map<std::string, Symbol> symbols;

	Symbol* findSymbol(const std::string& name) {
		if (auto it = symbols.find(name); it == symbols.end()) return nullptr;
		else return &it->second;
	}

	inline void merge(Namespace& rhs) {
		functions.insert(rhs.functions.begin(), rhs.functions.end());
		objectTypes.insert(rhs.objectTypes.begin(), rhs.objectTypes.end());
		symbols.insert(rhs.symbols.begin(), rhs.symbols.end());

		rhs.functions.clear();
		rhs.objects.clear();
		rhs.symbols.clear();
	}

	~Namespace() {
		for (auto& [name, f] : functions) {
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
