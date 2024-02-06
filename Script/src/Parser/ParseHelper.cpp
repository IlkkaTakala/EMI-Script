#include "ParseHelper.h"

bool TypesMatch(Token lhs, Token rhs)
{
	switch (lhs)
	{
	case TypeString:
		return rhs == Literal;
	case TypeNumber:
		return rhs == Number;
	case AnyType:
		return true;
	default:
		return lhs == rhs;
	};
}