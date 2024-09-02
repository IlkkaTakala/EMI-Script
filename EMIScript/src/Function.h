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
				uint8_t target : 8;
				union {
					struct {
						uint8_t in1 : 8;
						uint8_t in2 : 8;
					} PACKED;
					uint16_t param : 16;
				};
			} PACKED;
		};
	}PACKED;
	uint32_t data : 32;

	Instruction() { data = 0; }
};

#ifdef _MSC_VER
#pragma warning(pop)
#pragma pack(pop)
#endif

struct Scoped
{
	ankerl::unordered_dense::map<TName, CompileSymbol*> symbols;
	Scoped* parent = nullptr;
	std::list<Scoped> children;

	CompileSymbol* FindSymbol(const TName& name) {
		if (auto it = symbols.find(name); it != symbols.end()) {
			return it->second;
		}
		if (!parent) return nullptr;
		return parent->FindSymbol(name);
	}

	CompileSymbol* addSymbol(const TName& name) {
		auto it = FindSymbol(name);
		if (!it) {
			auto s = symbols.emplace(name, new CompileSymbol{});
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
	TName Name;

	std::vector<Variable> StringTable;
	ankerl::unordered_dense::set<double> NumberTable;

	std::vector<Function*> FunctionTable;
	std::vector<EMI::_internal_function*> ExternalTable;
	std::vector<IntrinsicPtr> IntrinsicTable;
	std::vector<Variable*> GlobalTable;
	std::vector<int32_t> PropertyTable;
	std::vector<VariableType> TypeTable;

	std::vector<TNameQuery> FunctionTableSymbols;
	std::vector<TName> PropertyTableSymbols;
	std::vector<TNameQuery> TypeTableSymbols;
	std::vector<TNameQuery> GlobalTableSymbols;

	ankerl::unordered_dense::map<int, int> DebugLines;

	Scoped* FunctionScope;

	std::vector<VariableType> Types;

	uint8_t ArgCount;
	uint8_t RegisterCount;
	bool IsPublic;

	std::vector<uint32_t> Bytecode;

	Function() : Name(nullptr) {
		FunctionScope = nullptr;
		ArgCount = 0;
		RegisterCount = 0;
		IsPublic = false;
	}
	~Function() {
		delete FunctionScope;
	}

	void Append(Function fn);
};