#pragma once
#include <string>
#include <string_view>
#include <fstream>
#include <unordered_map>

enum Token
{
	// Terminals
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
	AnyType,

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
	Less,
	Larger,
	LessEqual,
	LargerEqual,

	Add,
	Sub,
	Mult,
	Div,
	Router,
	Opt,

	Increment,
	Decrement,

	Comma,
	Dot,
	Semi,
	Colon,

	ValueId,
	Id,
	Ws,

	Hex,

	// Non-terminals
	Start,
	Program,
	RProgram,
	ObjectDef,
	PublicFunctionDef,
	FunctionDef,
	NamespaceDef,
	Scope,
	MStmt,
	Stmt,
	Expr,
	Value,
	Identifier,
	Arithmetic,
	Priority,

	ObjectVar,
	MObjectVar,
	OExpr,

	Typename,
	OTypename,

	VarDeclare,
	OVarDeclare,

	FuncionCall,
	CallParams,

	Conditional,

	Array,
	Indexer,

	Pipe,
	MPipe,

	Control,
	Flow,
	IfFlow,
	ForFlow,
	WhileFlow,
	ElseFlow,
	FlowBlock,

	Setter,

	Last
};

extern std::unordered_map<Token, const char*> TokensToName;

struct Context
{
	const char* Ptr;
	const char* Last;
	size_t Column{ 0 };
	size_t Row{ 1 };
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
	bool IsValid() const { return Valid; }
	Token GetNext(std::string_view& Data);
	const Context& GetContext() { return Current; }

private:

	Token Analyse(std::string_view& Data);

	std::string FileData;

	Context Current;

	bool Valid;
};