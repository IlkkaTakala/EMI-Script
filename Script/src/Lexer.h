#pragma once
#include <string>
#include <string_view>
#include <fstream>

enum class Token
{
	None,
	Skip,
	Error,
	Object,
	Definition,
	Public,
	Return,
	If,
	Else,
	For,
	While,
	Scope,
	Break,
	Continue,
	Extend,
	Variable,
	Assign,
	Set,

	String,
	Integer,
	Float,
	True,
	False,

	TypeString,
	TypeInteger,
	TypeFloat,

	Rcurly,
	Lcurly,
	Rbracket,
	Lbracket,
	Rparenthesis,
	Lparenthesis,

	Equal,
	Not,
	And,
	Or,

	Add,
	Sub,
	Mult,
	Div,
	Router,
	Opt,

	Comma,
	Dot,
	Semi,
	Colon,

	ValueId,
	Id,
	Ws,

	Hex,
};

struct Context
{
	const char* Ptr;
	const char* Last;
	size_t Column{ 0 };
	size_t Row{ 1 };
	std::string Function;
	bool Valid;

	Token Previous;

	bool Advance() {
		switch (*Ptr) {
		case '\n':
			Row++;
			Column = 0;
			break;
		default:
			Column++;
			break;
		}
		Ptr++;

		Valid = Ptr < Last;
		return Valid;
	}
};

class Lexer
{
public:
	Lexer(const std::string& file);
	~Lexer();

	void Reset();
	Token GetNext(std::string_view& Data);

private:

	std::string FileData;

	Context Current;

	bool Valid;
};