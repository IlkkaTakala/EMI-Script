#include "Symbol.h"
#include "Namespace.h"
#include "Function.h"
#include "Objects/UserObject.h"

void Symbol::setType(SymbolType t)
{
	Type = t;
	Flags = SymbolFlags::None;

	switch (t)
	{
	case SymbolType::Static:
		Flags = SymbolFlags::Value;
		break;
	case SymbolType::Object:
		break;
	case SymbolType::Function:
		break;
	case SymbolType::Namespace:
		break;
	case SymbolType::Variable:
		Flags = SymbolFlags::Assignable;
		break;
	default:
		break;
	}
}

Symbol::~Symbol()
{
	switch (Type)
	{
	case SymbolType::Namespace: {
		delete static_cast<Namespace*>(Data);
	} break;
	case SymbolType::Function: {
		auto sym = static_cast<FunctionSymbol*>(Data);
		if (sym->Type == FunctionType::User)
			delete static_cast<Function*>(sym->DirectPtr);
		delete sym;
	} break;
	case SymbolType::Object: {
		delete static_cast<UserDefinedType*>(Data);
	} break;
	case SymbolType::Variable: {
		delete static_cast<Variable*>(Data);
	} break;
	default:
		break;
	}
	Data = nullptr;
}
