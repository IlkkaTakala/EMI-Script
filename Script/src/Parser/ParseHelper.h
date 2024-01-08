#pragma once
#include <sstream>
#include <string_view>
#include "Lexer.h"

struct RuleData
{
	Token dataToken = None;
	Token nodeType = None;
	Token mergeToken = None;
};

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

struct Node;
struct TokenHolder
{
	Token token = None;
	Node* ptr = nullptr;
	std::string_view data;
};

inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string& s) {
	rtrim(s);
	ltrim(s);
}

template <typename Out>
void splits(const std::string& s, char delim, Out result) {
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim)) {
		trim(item);
		*result++ = item;
	}
}

inline std::vector<std::string> splits(const std::string& s, char delim) {
	std::vector<std::string> elems;
	splits(s, delim, std::back_inserter(elems));
	return elems;
}

bool TypesMatch(Token lhs, Token rhs);