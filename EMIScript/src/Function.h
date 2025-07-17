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

struct ScopeType
{
	ankerl::unordered_dense::map<PathType, CompileSymbol*> symbols;
	ScopeType* parent = nullptr;
	std::list<ScopeType> children;

	CompileSymbol* FindSymbol(const PathType& name) {
		if (auto it = symbols.find(name); it != symbols.end()) {
			return it->second;
		}
		if (!parent) return nullptr;
		return parent->FindSymbol(name);
	}

	CompileSymbol* addSymbol(const PathType& name) {
		auto it = FindSymbol(name);
		if (!it) {
			auto s = symbols.emplace(name, new CompileSymbol{});
			return s.first->second;
		}
		return nullptr;
	}

	~ScopeType() {
		for (auto& [name, s] : symbols) {
			delete s;
		}
	}
};

struct Function
{
	PathType Name;

	std::vector<Variable> StringTable;
	ankerl::unordered_dense::set<double> NumberTable;

	std::vector<Function*> FunctionTable;
	std::vector<EMI::_internal_function*> ExternalTable;
	std::vector<IntrinsicPtr> IntrinsicTable;
	std::vector<Variable*> GlobalTable;
	std::vector<int32_t> PropertyTable;
	std::vector<VariableType> TypeTable;

	std::vector<PathTypeQuery> FunctionTableSymbols;
	std::vector<NameType> PropertyTableSymbols;
	std::vector<PathTypeQuery> TypeTableSymbols;
	std::vector<PathTypeQuery> GlobalTableSymbols;

	ankerl::unordered_dense::map<int, int> DebugLines;

	ScopeType* FunctionScope;

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