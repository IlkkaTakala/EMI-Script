#include "ParseHelper.h"

bool TypesMatch(Token lhs, Token rhs)
{
	switch (lhs)
	{
	case Token::TypeString:
		return rhs == Token::Literal;
	case Token::TypeNumber:
		return rhs == Token::Number;
	case Token::TypeBoolean:
		return rhs == Token::False || rhs == Token::True;
	case Token::TypeArray:
		return rhs == Token::Array;
	case Token::AnyType:
		return true;
	default:
		return lhs == rhs;
	};
}