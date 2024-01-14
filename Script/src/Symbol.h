#pragma once
#include "Defines.h"
#include "Eril/Variable.h"

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
	SymbolType type = SymbolType::Variable;
	SymbolFlags flags = SymbolFlags::None;
	VariableType varType = VariableType::Undefined;
	bool resolved = false;
	int startLife;
	int endLife;

	void setType(SymbolType t);

	inline bool isAssignable() const { return SymbolFlags::Assignable == (flags & SymbolFlags::Assignable); }
};