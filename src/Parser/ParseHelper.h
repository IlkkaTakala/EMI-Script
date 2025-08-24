#pragma once
#include <string_view>
#include "Lexer.h"

enum class Association {
	None,
	Left,
	Right,
};

struct TokenData
{
	int Precedence = 0;
	Association Associativity = Association::None;
};

class Node;
struct TokenHolder
{
	Token token = Token::None;
	Node* ptr = nullptr;
	std::string_view data;
};

bool TypesMatch(Token lhs, Token rhs);
