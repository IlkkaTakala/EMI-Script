#include "Lexer.h"
#include <ankerl/unordered_dense.h>
#include "Defines.h"
#include <filesystem>

#ifdef DEBUG
#define X(name) {Token::name, #name},
extern ankerl::unordered_dense::map<Token, const char*> TokensToName = {
#include "Lexemes.h"
};
#undef X
#endif // DEBUG

ankerl::unordered_dense::map<std::string_view, Token> TokenMap = {
	{"object", Token::Object },
	{"def", Token::Definition },
	{"public", Token::Public },
	{"return", Token::Return },
	{"if", Token::If },
	{"else", Token::Else },
	{"for", Token::For },
	{"while", Token::While },
	{"break", Token::Break },
	{"continue", Token::Continue },
	{"extend", Token::Extend },
	{"var", Token::Var },
	{"set", Token::Set },
	{"static", Token::Static },
	{"const", Token::Const },

	{"string", Token::TypeString},
	{"number", Token::TypeNumber},
	{"array", Token::TypeArray},
	{"function", Token::TypeFunction},

	{"true", Token::True},
	{"false", Token::False},

	{"or", Token::Or},
	{"is", Token::Equal},
	{"not", Token::Not},
	{"and", Token::And},
};

Lexer::Lexer(const char* data, size_t size)
{
	Valid = data && size > 0;
	FileData = data;
	Size = size;
	Reset();
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

void Lexer::Reset()
{
	Current.Ptr = FileData;
	Current.Last = FileData + Size;
	Current.Column = 1;
	Current.Row = 1;
	Current.Valid = true;
}

Token Lexer::GetNext(std::string_view& Data)
{
	Token token = Token::None;
	do {
		token = Analyse(Data);
	} while (token == Token::Skip);

	return token;
}

Token Lexer::Analyse(std::string_view& Data)
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
			TOKEN(Number);
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
			TOKEN(Number);
			)
				CASE('x',
					Current.Advance();
			while (is_alnum(*ptr)) Current.Advance();
			TOKEN(Number);
			)
		default:
			TOKEN(Number);
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
			CASE('<', TOKEN(Less);)
			CASE('>', TOKEN(Larger);)
			CASE(',', TOKEN(Comma);)
			CASE('?', TOKEN(Opt);)
			CASE('.', TOKEN(Dot);)
			CASE(':',
				if (is_alpha(*(ptr + 1))) {
					TOKEN(ValueId);
					Current.Advance();
					while (ValidID(*(ptr + 1))) Current.Advance();
				}
				else TOKEN(Colon);)

			CASE('+', TOKEN(Add);)
			CASE('-', TOKEN(Sub);)
			CASE('*', TOKEN(Mult);)
			CASE('/', 
				if (*(ptr + 1) == '*') {
					Current.Advance();
					while (!(*ptr == '*' && *(ptr + 1) == '/')) Current.Advance();
					Current.Advance();
					TOKEN(Skip);
				} else
					TOKEN(Div);
				)

			CASE('"',
				Current.Advance();
				while (*ptr != '"') Current.Advance();
				TOKEN(Literal);
				start++; endOffset = 1;
				)

			CASE('=',
				if (*(ptr + 1) == '=') {
					TOKEN(Equal);
					Current.Advance();
				}
				else TOKEN(Assign);
				)
					
			CASE('!',
				if (*(ptr + 1) == '=') {
					TOKEN(NotEqual);
					Current.Advance();
				}
				else TOKEN(Not);
				)

			CASE('|',
				Current.Advance();
				switch (*ptr) {
					CASE('|', TOKEN(Or);)
						CASE('>', TOKEN(Router);)
				default:
					TOKEN(Error);
				})

			CASE('#',
				bool res = true;
				while (*ptr != '\n' && res) res = Current.Advance();
				TOKEN(Skip);
			)

		default:
			TOKEN(Skip);
		}

		Current.Advance();
	}

	Data = std::string_view(start, ptr - start - endOffset);
	Current.Previous = token;

	return token;
}
