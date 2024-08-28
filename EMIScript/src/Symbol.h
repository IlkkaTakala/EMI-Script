#pragma once
#include "Defines.h"
#include "Variable.h"
#include <vector>
#include <Core.h>

enum class SymbolFlags
{
	None = 0,
	Assignable = 1,
	Typed = 2,
	Value = 4,
	Public = 8,
};

enum class SymbolType : uint8_t
{
	None,
	Namespace,
	Object,
	Function,
	Variable,
	Static,
	// ...
};

enum class FunctionType : uint8_t
{
	None,
	User,
	Host,
	Intrinsic,
};

inline SymbolFlags operator| (SymbolFlags a, SymbolFlags b) { return (SymbolFlags)((int)a | (int)b); }
inline SymbolFlags operator& (SymbolFlags a, SymbolFlags b) { return (SymbolFlags)((int)a & (int)b); }
inline SymbolFlags operator^ (SymbolFlags a, SymbolFlags b) { return (SymbolFlags)((int)a ^ (int)b); }

struct Symbol
{
	SymbolType Type = SymbolType::Variable;
	SymbolFlags Flags = SymbolFlags::None;
	VariableType VarType = VariableType::Undefined;
	void* Data = nullptr; // Namespace, UserDefinedObject, Variable or FunctionSymbol
	bool Builtin = false;

	void setType(SymbolType t);

	~Symbol();
};

struct CompileSymbol
{
	Symbol* Sym = nullptr;

	bool Global = false;
	bool Resolved = false;
	bool NeedsLoading = false;
	uint8_t Register = 0;
	size_t StartLife = 0;
	size_t EndLife = 0;

	~CompileSymbol() {
		if (!Global)
			delete Sym;
	}
};

struct FunctionSymbol
{
	FunctionType Type;
	void* DirectPtr;
	Variable Function;
	VariableType Return;
	std::vector<VariableType> Arguments;
};

inline bool isAssignable(const SymbolFlags& s) { return SymbolFlags::Assignable == (s & SymbolFlags::Assignable); }
inline bool isTyped(const SymbolFlags& s) { return SymbolFlags::Typed == (s & SymbolFlags::Typed); }