#include "ParseHelper.h"

bool TypesMatch(Token lhs, Token rhs)
{
	switch (lhs)
	{
	case Token::TypeString:
		return rhs == Token::Literal;
	case Token::TypeNumber:
		return rhs == Token::Number;
	case Token::AnyType:
		return true;
	default:
		return lhs == rhs;
	};
}