#include "Symbol.h"

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
