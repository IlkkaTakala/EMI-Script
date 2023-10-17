#include "Parser.h"
#include <functional>
#include <set>
#include <stack>
#include <iostream>
#include <unordered_map>

#include "ParseTable.h"

static Grammar G;

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

static std::vector<TokenData> TokenParseData(Last, TokenData{});

struct RuleData
{
	Token dataToken = None;
	Token nodeType = None;
	Token mergeToken = None;
};
//
//static const RuleType Rules = {
//	{Start, Program},
//	{Program, RProgram, Program},
//	{Program, None},
//
//	{RProgram, ObjectDef},
//	{RProgram, FunctionDef},
//	{RProgram, NamespaceDef},
//
//	{ObjectDef, Object, Id},
//	{FunctionDef, Definition, Id, Lparenthesis, Rparenthesis, Scope},
//
//	{Scope, Lcurly, MStmt, Rcurly},
//	{MStmt, Stmt, MStmt},
//	{MStmt, Scope, MStmt},
//	{MStmt, None},
//	{Stmt, Expr, Semi},
//	{Expr, Expr, Add, Expr},
//	{Expr, Integer},
//
//	{NamespaceDef, Extend, Id, Colon},
//};

static const RuleType Rules = {
	{Start, FunctionDef},
	{FunctionDef, Definition, Id, Scope},
	{Scope, Lcurly, MStmt, Rcurly},
	{MStmt, Stmt, MStmt},
	{MStmt, Scope, MStmt},
	{MStmt, None},
	{Stmt, Integer, Semi},
};
/*
static const std::vector<RuleData> Data = {
	{},
	{None, None, Program},
	{},

	{None, None, ObjectDef},
	{None, None, FunctionDef},
	{None, None, NamespaceDef},

	{Id},
	{Id},

	{None, Scope },
	{None, None, MStmt },
	{None, None, MStmt },
	{ },
	{ },
	{Add, Add },
	{Integer },

	{Id},
};
*/

static const std::vector<RuleData> Data = {
	{},
	{},
	{ },
	{ },
	{ },
	{ },
	{ },
};


void Parser::InitializeParser()
{
	G = CreateParser(Rules);

	TokenParseData[Add] = { 10, Association::Left };
	TokenParseData[Mult] = { 20, Association::Left };
	TokenParseData[Lcurly] = { 2, Association::Right };
	TokenParseData[Rcurly] = { 1, Association::Left };

	if (Rules.size() != Data.size()) {
		throw std::range_error("Rule data does not match rules");
	}
}

void Parser::ThreadedParse(VM* vm)
{
	while (vm->CompileRunning) {

		std::unique_lock lk(vm->CompileMutex);
		vm->QueueNotify.wait(lk, [vm] {return !vm->CompileQueue.empty() || !vm->CompileRunning; });
		if (!vm->CompileRunning) return;

		auto options = vm->CompileQueue.front();
		vm->CompileQueue.pop();
		lk.unlock();

		Parse(vm, options);
	}
}

struct Node
{
	Token type;
	std::string_view data;

	std::vector<Node*> parent;
	std::list<Node*> children;
};

struct TokenHolder
{
	Token token = None;
	Node* ptr = nullptr;
	std::string_view data;
};

void Parser::Parse(VM* vm, CompileOptions& options)
{
	Lexer lex(options.Path);

	int count = 0;
	auto token = Token::None;
	std::stack<Token> ss;

	std::stack<int> stateStack;
	std::stack<TokenHolder> symbolStack;
	std::stack<Token> operatorStack;
	stateStack.push(0);

	//int inputIndex = 0;
	TokenHolder holder;
	holder.token = lex.GetNext(holder.data);

	while (true) {
		int currentState = stateStack.top();

		ActionNode action = G.ParseTable[currentState][holder.token];

		switch (action.type)
		{
		SHIFTCASE:
		case Action::SHIFT: {
			stateStack.push(action.shift);
			symbolStack.push(holder);
			if (TokenParseData[holder.token].Precedence != 0) operatorStack.push(holder.token);
			holder.token = lex.GetNext(holder.data);
			holder.ptr = nullptr;
		} break;
		
		REDUCECASE:
		case Action::REDUCE: {
			const Rule& rule = G.RuleTable[action.reduce];
			const RuleData data = Data[action.reduce];

			TokenHolder next;
			next.token = rule.nonTerminal;
			if (data.mergeToken == None) {
				next.ptr = new Node();
				next.ptr->type = data.nodeType == None ? next.token : data.nodeType;
			}

			for (int i = 0; i < rule.development.size(); ++i) {
				if (rule.development[i] == None) break;
				if (!operatorStack.empty() && symbolStack.top().token == operatorStack.top()) operatorStack.pop();

				auto& old = symbolStack.top();
				if (data.mergeToken == old.token) {
					next.ptr = old.ptr;
				}
				else if (old.ptr && next.ptr)
					next.ptr->children.push_front(old.ptr);
				else if (old.token == data.dataToken){
					next.ptr->data = old.data;
				}

				symbolStack.pop();
				stateStack.pop();
			}

			symbolStack.push(next);
			if (TokenParseData[rule.nonTerminal].Precedence != 0) operatorStack.push(rule.nonTerminal);

			stateStack.push(G.ParseTable[stateStack.top()][rule.nonTerminal].shift);
		} break;

		case DECIDE: {
			auto& next = TokenParseData[holder.token];
			if (next.Precedence == 0) 
				goto SHIFTCASE;

			auto& prev = TokenParseData[operatorStack.top()];
			if (prev.Precedence != 0) {
				if (prev.Precedence == next.Precedence) {
					if (prev.Associativity == next.Associativity) {
						switch (prev.Associativity)
						{
						case Association::None: goto ERRORCASE;
						case Association::Left: goto REDUCECASE;
						case Association::Right: goto SHIFTCASE;
						default:
							break;
						}
					}
					goto ERRORCASE;
				}
				else if (prev.Precedence > next.Precedence) 
					goto REDUCECASE;
				else 
					goto SHIFTCASE;
			}
			goto SHIFTCASE;
		} break;

		case Action::ACCEPT: {
			std::cout << "Accepted\n";
		} break;

		ERRORCASE:
		default: {
			std::cout << "Error\n";
			break;
		}
		}
	}
}
