#include "Lexer.h"
#include <unordered_map>

std::unordered_map<std::string_view, Token> TokenMap = {
	{"object", Token::Object },
	{"def", Token::Definition },
	{"public", Token::Public },
	{"return", Token::Return },
	{"if", Token::If },
	{"else", Token::Else },
	{"for", Token::For },
	{"while", Token::While },
	{"scope", Token::Scope },
	{"break", Token::Break },
	{"continue", Token::Continue },
	{"extend", Token::Extend },
	{"var", Token::Variable },
	{"set", Token::Set },

	{"string", Token::TypeString},
	{"int", Token::TypeInteger},
	{"float", Token::TypeFloat},

	{"true", Token::True},
	{"false", Token::False},

	{"or", Token::Or},
	{"is", Token::Equal},
	{"not", Token::Not},
	{"and", Token::And},
};

Lexer::Lexer(const std::string& file)
{
	Valid = false;
	std::fstream data(file, std::ios::in);

	if (data.is_open()) {
		Valid = true;

		data.seekg(0, std::ios::end);
		size_t size = data.tellg();
		FileData.resize(size + 1);
		data.seekg(0);
		data.read(&FileData[0], size);

		Current.Ptr = FileData.c_str();
		Current.Last = Current.Ptr + FileData.length();
	}
}

Lexer::~Lexer()
{
	Valid = false;
}

#define CASE(Token, Block) \
case Token: { \
	Block\
} break;

#define TOKEN(T) token = Token::T

inline bool is_whitespace(unsigned char c) {
	return (c >= 9 && c <= 13) || c == 32;
}

inline bool is_digit(unsigned char c) {
	return c >= 48 && c <= 57;
}

inline bool is_alpha(unsigned char c) {
	return (c >= 97 && c <= 122) || (c >= 65 && c <= 90);
}

inline bool is_alnum(unsigned char c) {
	return is_alpha(c) || is_digit(c);
}

inline bool ValidID(unsigned char c) {
	return is_alpha(c) || c == 95;
}

Token Lexer::GetNext(std::string_view& Data)
{
	Token token(Token::None);
	auto& ptr = Current.Ptr;
	if (!Current.Valid) return token;
	if (*ptr == '\0') return Token::None;

	while (is_whitespace(*ptr)) Current.Advance();
	auto start = ptr;
	size_t endOffset = 0;

	if (ValidID(*ptr)) {
		if ((*ptr == 'x' || *ptr == '.') && is_digit(*(ptr + 1))) {
			TOKEN(Integer);
			while (is_alnum(*ptr)) Current.Advance();
		}
		else {
			while (is_alnum(*ptr)) Current.Advance();

			std::string_view value(start, ptr - start);
			if (auto it = TokenMap.find(value); it != TokenMap.end()) {
				token = it->second;
			}
			else TOKEN(Id);
		}
	}
	else if (is_digit(*ptr)) {
		while (is_digit(*ptr)) Current.Advance();
		switch (*ptr) {
			CASE('.',
				Current.Advance();
			while (is_digit(*ptr)) Current.Advance();
			TOKEN(Float);
			)
				CASE('x',
					Current.Advance();
			while (is_alnum(*ptr)) Current.Advance();
			TOKEN(Integer);
			)
		default:
			TOKEN(Integer);
		}
	}
	else {
		switch (*ptr) {
			CASE(';', TOKEN(Semi);)
			CASE('(', TOKEN(Lparenthesis);)
			CASE(')', TOKEN(Rparenthesis);)
			CASE('{', TOKEN(Lcurly);)
			CASE('}', TOKEN(Rcurly);)
			CASE('[', TOKEN(Lbracket);)
			CASE(']', TOKEN(Rbracket);)
			CASE(',', TOKEN(Comma);)
			CASE('?', TOKEN(Opt);)
			CASE('.', TOKEN(Dot);)
			CASE(':',
				if (is_alpha(*(ptr + 1))) {
					TOKEN(ValueId);
					Current.Advance();
					while (is_alnum(*ptr)) Current.Advance();
					endOffset = 1;
				}
				else TOKEN(Colon);)

			CASE('+', TOKEN(Add);)
			CASE('-', TOKEN(Sub);)
			CASE('*', TOKEN(Mult);)
			CASE('/', TOKEN(Div);)

			CASE('"',
				while (*ptr != '"') Current.Advance();
				TOKEN(String);
				start++; endOffset = 1;
			)
			CASE('=',
				if (*(ptr + 1) == '=') {
					TOKEN(Equal);
					Current.Advance();
				}
				else TOKEN(Assign);
			)

			CASE('|',
				Current.Advance();
				switch (*ptr) {
					CASE('|', TOKEN(Or);)
					CASE('>', TOKEN(Router);)
				default:
					TOKEN(Error);
			})

		default:
			TOKEN(Skip);
		}

		Current.Advance();
	}

	Data = std::string_view(start, ptr - start - endOffset);
	Current.Previous = token;

	return token;
}
