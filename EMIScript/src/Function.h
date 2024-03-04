#pragma once
#include "Defines.h"
#include <string>
#include <vector>
#include <list>
#include <ankerl/unordered_dense.h>
#include "Symbol.h"
#include "EMI/EMI.h"
#include "Intrinsic.h"

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
		union {
			struct {
				uint8 target : 8;
				union {
					struct {
						uint8 in1 : 8;
						uint8 in2 : 8;
					} PACKED;
					uint16 param : 16;
				};
			} PACKED;
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
	ankerl::unordered_dense::map<std::string, Symbol*> symbols;
	Scoped* parent = nullptr;
	std::list<Scoped> children;

	Symbol* findSymbol(const std::string& name) {
		if (auto it = symbols.find(name); it != symbols.end()) {
			return it->second;
		}
		if (!parent) return nullptr;
		return parent->findSymbol(name);
	}

	Symbol* addSymbol(const std::string& name) {
		auto it = findSymbol(name);
		if (!it) {
			auto s = symbols.emplace(name, new Symbol{});
			return s.first->second;
		}
		return nullptr;
	}

	~Scoped() {
		for (auto& [name, s] : symbols) {
			delete s;
		}
	}
};

struct Function
{
	std::string Name;
	std::string Namespace;
	size_t NamespaceHash;

	std::vector<Variable> StringTable;
	ankerl::unordered_dense::set<double> NumberTable;
	ankerl::unordered_dense::set<size_t> JumpTable;

	std::vector<Function*> FunctionTable;
	std::vector<EMI::__internal_function*> ExternalTable;
	std::vector<IntrinsicPtr> IntrinsicTable;
	std::vector<Variable*> GlobalTable;
	std::vector<int32> PropertyTable;
	std::vector<VariableType> TypeTable;

	std::vector<std::string> FunctionTableSymbols;
	std::vector<std::string> PropertyTableSymbols;
	std::vector<std::string> TypeTableSymbols;
	std::vector<std::string> GlobalTableSymbols;

	ankerl::unordered_dense::map<int, int> DebugLines;

	Scoped* FunctionScope;

	std::vector<VariableType> Types;

	uint8 ArgCount;
	uint8 RegisterCount;
	bool IsPublic;

	std::vector<uint32> Bytecode;

	Function() {
		FunctionScope = nullptr;
		ArgCount = 0;
		RegisterCount = 0;
		IsPublic = false;
		NamespaceHash = 0;
	}
	~Function() {
		delete FunctionScope;
	}
};