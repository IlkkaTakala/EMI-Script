#pragma once
#include "Namespace.h"

namespace Library 
{

	bool Decode(std::istream& instream, SymbolTable& table, ScriptFunction*& init);

	bool Encode(const SymbolTable& table, std::ostream& outstream, ScriptFunction* init);

}