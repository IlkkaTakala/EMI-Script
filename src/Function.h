#pragma once
#include "Defines.h"
#include <string>
#include <vector>
#include <list>
#include <ankerl/unordered_dense.h>
#include "Symbol.h"
#include "EMI/EMI.h"
#include "Intrinsic.h"
#include <map>

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
	// @todo: NameType might be enough?
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

struct FunctionSignature
{
	VariableType Return = VariableType::Undefined;
	std::vector<VariableType> Arguments;
	std::vector<NameType> ArgumentNames;
	bool HasReturn;
	bool AnyNumArgs;
};

struct FunctionSymbol
{
	FunctionSignature Signature;
	bool IsPublic = true;
	FunctionType Type = FunctionType::None;
	union {
		ScriptFunction* Local = nullptr;
		EMI::_internal_function* Host;
		IntrinsicPtr Intrinsic;
	};
	FunctionSymbol* Previous = nullptr;
	FunctionSymbol* Next = nullptr;

	~FunctionSymbol();
};

struct FunctionTable
{
	std::map<int, FunctionSymbol*> Functions;
	Variable FunctionVar;

	// @todo: Should probably account for types if arg count does not match
	FunctionSymbol* GetFirstFitting(int args);

	void AddFunction(int args, FunctionSymbol* symbol);

	~FunctionTable() {
		for (auto& [i, sym] : Functions) {
			delete sym;
		}
	}
};

struct ScriptFunction
{
	PathType Name;

	std::vector<Variable> StringTable;
	ankerl::unordered_dense::set<double> NumberTable;

	std::vector<FunctionSymbol*> FunctionTable;
	std::vector<Variable*> GlobalTable;
	std::vector<int32_t> PropertyTable;
	std::vector<VariableType> TypeTable;

	std::vector<PathTypeQuery> FunctionTableSymbols;
	std::vector<NameType> PropertyTableSymbols;
	std::vector<PathTypeQuery> TypeTableSymbols;
	std::vector<PathTypeQuery> GlobalTableSymbols;

	ankerl::unordered_dense::map<int, int> DebugLines;

	ScopeType* FunctionScope;

	uint8_t ArgCount;
	uint8_t RegisterCount;
	bool IsPublic;

	std::vector<uint32_t> Bytecode;

	ScriptFunction() : Name(nullptr) {
		FunctionScope = nullptr;
		ArgCount = 0;
		RegisterCount = 0;
		IsPublic = false;
	}
	~ScriptFunction() {
		delete FunctionScope;
	}

	void Append(ScriptFunction fn);
};

