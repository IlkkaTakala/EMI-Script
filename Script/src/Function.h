#pragma once
#include "Defines.h"
#include <string>
#include <vector>
#include <ankerl/unordered_dense.h>

#include "Symbol.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4201)
#pragma pack(push,1)
#define PACKED
#else
#define PACKED __attribute__ ((__packed__))
#endif

union Instruction
{
	struct {
		OpCodes code : 8;
		uint8 target : 8;
		union {
			struct {
				uint8 in1 : 8;
				uint8 in2 : 8;
			};
			uint16 param : 16;
		};
	}PACKED;
	uint32 data : 32;

	Instruction() { data = 0; }
};

#ifdef _MSC_VER
#pragma warning(pop)
#pragma pack(pop)
#endif

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
	uint8 RegisterCount;

	std::vector<uint32> Bytecode;

	Function() {
		scope = nullptr;
		ArgCount = 0;
		RegisterCount = 0;
	}
	~Function() {
		delete scope;
	}
};