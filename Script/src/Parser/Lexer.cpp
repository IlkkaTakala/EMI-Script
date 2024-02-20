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
	{"bool", Token::TypeBoolean},
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

inline bool ValidIDFirst(unsigned char c) {
	return is_alpha(c) || c == 95;
}

inline bool ValidID(unsigned char c) {
	return is_alnum(c) || c == 95;
}

void Lexer::Reset()
{
	Current.Ptr = FileData;
	Current.Last = FileData + Size;
	Current.Column = 1;
	Current.Row = 1;
	Current.Valid = true;
	InString = false;
	InQuote = false;
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
	size_t endOffset = 0;
	auto start = ptr;

	if (InString && InQuote) {
		while (InString && Current.Valid) {
			switch (*(ptr))
			{
			case '}': {
				if (*(ptr + 1) == '´') {
					InString = false;
				}
				else Current.Advance();
			} break;

			case '´': {
				if (*(ptr + 1) == '{') {
					InString = false;
				}
				else Current.Advance();
			} break;

			case '"': {
				InString = false;
			} break;

			default:
				Current.Advance();
				break;
			}
		}

		token = Token::Literal;
		//if (*(ptr + 1) == '´' && *(ptr + 2) == '{') {
		//	endOffset = 0;
		//}
		//else {
		//	endOffset = 1;
		//}
		InString = false;

		Data = std::string_view(start, ptr - start - endOffset);
		Current.Previous = token;

		return token;
	}

	while (is_whitespace(*ptr)) Current.Advance();
	start = ptr;

	if (ValidIDFirst(*ptr)) {
		if ((*ptr == 'x' || *ptr == '.') && is_digit(*(ptr + 1))) {
			token = Token::Number;
			while (is_alnum(*ptr)) Current.Advance();
		}
		else {
			while (ValidID(*ptr)) Current.Advance();

			std::string_view value(start, ptr - start);
			if (auto it = TokenMap.find(value); it != TokenMap.end()) {
				token = it->second;
			}
			else token = Token::Id;
		}
	}
	else if (is_digit(*ptr)) {
		while (is_digit(*ptr)) Current.Advance();
		switch (*ptr) {
		case '.': { Current.Advance(); while (is_digit(*ptr)) Current.Advance(); token = Token::Number; } break;
		case 'x': { Current.Advance(); while (is_alnum(*ptr)) Current.Advance(); token = Token::Number; } break;
		default:
			token = Token::Number;
		}
	}
	else {
		switch (*ptr) {
		case ';': { token = Token::Semi; } break;
		case '(': { token = Token::Lparenthesis; } break;
		case ')': { token = Token::Rparenthesis; } break;
		case '{': { token = Token::Lcurly; } break;
		case '}': { 
			if (*(ptr + 1) == '´') {
				token = Token::StrDelimiterR; 
				Current.Advance(); 
				InString = true;
			}
			else token = Token::Rcurly; 
		} break;
		case '´': {
			if (*(ptr + 1) == '{') {
				token = Token::StrDelimiterL;
				Current.Advance();
			}
			else token = Token::Error;
		} break;
		case '[': { token = Token::Lbracket; } break;
		case ']': { token = Token::Rbracket; } break;
		case '<': { token = Token::Less; } break;
		case '>': { token = Token::Larger; } break;
		case ',': { token = Token::Comma; } break;
		case '?': { token = Token::Opt; } break;
		case '.': { token = Token::Dot; } break;
		case ':': { 
			if (is_alpha(*(ptr + 1))) {
				token = Token::ValueId; 
				Current.Advance(); 
				while (ValidID(*(ptr + 1))) 
					Current.Advance();
			}
			else token = Token::Colon; 
		} break;
		case '+': { token = Token::Add; } break;
		case '-': { token = Token::Sub; } break;
		case '*': { token = Token::Mult; } break;
		case '/': { 
			if (*(ptr + 1) == '*') {
				Current.Advance(); 
				while (!(*ptr == '*' && *(ptr + 1) == '/')) 
					Current.Advance(); 
				Current.Advance(); 
				token = Token::Skip;
			}
			else token = Token::Div; 
		} break;

		case '"': { 
			token = Token::Quote;
			InString = !InQuote;
			InQuote = InString;
		} break;

		case '=': { 
			if (*(ptr + 1) == '=') {
				token = Token::Equal; 
				Current.Advance();
			}
			else token = Token::Assign; 
		} break;

		case '!': { 
			if (*(ptr + 1) == '=') {
				token = Token::NotEqual; 
				Current.Advance();
			}
			else token = Token::Not; 
		} break;

		case '|': { 
			Current.Advance(); 
			switch (*ptr) {
				case '|': { token = Token::Or; } break; 
				case '>': { token = Token::Router; } break; 
				default: token = Token::Error;
			}
		} break;

		case '&': { 
			Current.Advance(); 
			if (*ptr == '&') {
				token = Token::And;
			}
			else token = Token::Error; 
		} break;

		case '#': { 
			bool res = true; 
			while (*ptr != '\n' && res) 
				res = Current.Advance(); 
			token = Token::Skip; 
		} break;

		default:
			token = Token::Skip;
		}

		Current.Advance();
	}

	Data = std::string_view(start, ptr - start - endOffset);
	Current.Previous = token;

	return token;
}
