#include "Symbol.h"

void Symbol::setType(SymbolType t)
{
	type = t;
	flags = SymbolFlags::None;

	switch (t)
	{
	case SymbolType::Static:
		flags = SymbolFlags::Value;
		break;
	case SymbolType::Object:
		break;
	case SymbolType::Function:
		break;
	case SymbolType::Namespace:
		break;
	case SymbolType::Variable:
		flags = SymbolFlags::Assignable;
		break;
	default:
		break;
	}
}
