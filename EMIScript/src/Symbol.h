#pragma once
#include "Defines.h"
#include "Variable.h"

enum class SymbolFlags
{
	None = 0,
	Assignable = 1,
	Typed = 2,
	Value = 4,
	Public = 8,
};

enum class SymbolType
{
	Namespace,
	Object,
	Function,
	Variable,
	Static,
	// ...
};

inline SymbolFlags operator| (SymbolFlags a, SymbolFlags b) { return (SymbolFlags)((int)a | (int)b); }
inline SymbolFlags operator& (SymbolFlags a, SymbolFlags b) { return (SymbolFlags)((int)a & (int)b); }
inline SymbolFlags operator^ (SymbolFlags a, SymbolFlags b) { return (SymbolFlags)((int)a ^ (int)b); }

struct Symbol
{
	SymbolType Type = SymbolType::Variable;
	SymbolFlags Flags = SymbolFlags::None;
	VariableType VarType = VariableType::Undefined;
	bool Resolved = false;
	uint8_t Register = 0;
	bool NeedsLoading = false;
	size_t StartLife = 0;
	size_t EndLife = 0;

	void setType(SymbolType t);
};

inline bool isAssignable(const SymbolFlags& s) { return SymbolFlags::Assignable == (s & SymbolFlags::Assignable); }
inline bool isTyped(const SymbolFlags& s) { return SymbolFlags::Typed == (s & SymbolFlags::Typed); }