#include "Parser.h"
#include <functional>
#include <set>
#include <stack>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <charconv>
#include <filesystem>

#include "Lexer.h"
#include "ParseTable.h"
#include "AST.h"

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

static RuleType Rules = {
	{Start, Program},
	{Program, RProgram, Program},
	{Program, None},

	{RProgram, ObjectDef},
	{RProgram, FunctionDef},
	{RProgram, NamespaceDef},

	{Typename, TypeString},
	{Typename, TypeInteger},
	{Typename, TypeFloat},

	{ObjectDef, Object, Id, Lcurly, MObjectVar, Rcurly},
	{MObjectVar, ObjectVar, MObjectVar},
	{MObjectVar, None},
	{ObjectVar, Id, OTypename, OExpr, Semi},
	{OTypename, Colon, Typename},
	{OTypename, None},
	{OExpr, Assign, Expr},
	{OExpr, None},

	{FunctionDef, Definition, Id, Lparenthesis, Rparenthesis, Scope},
	{NamespaceDef, Extend, Id, Colon},

	{Scope, Lcurly, MStmt, Rcurly},
	{MStmt, Stmt, MStmt},
	{MStmt, Scope, MStmt},
	{MStmt, None},
	{Stmt, Expr, Semi},

	{Expr, Expr, Add, Expr},
	{Expr, Expr, Sub, Expr},
	{Expr, Expr, Div, Expr},
	{Expr, Expr, Mult, Expr},

	{Expr, Conditional},
	{Conditional, Expr, Opt, Expr, Colon, Expr },

	{Expr, Value},
	{Value, Integer},
	{Value, String},
	{Value, Float},
	{Value, Identifier},
	{Identifier, Id},
	{Identifier, Identifier, Dot, Id},

	{Expr, Expr, Assign, Expr},
	{Expr, VarDeclare},
	{Expr, FuncionCall},

	{VarDeclare, Variable, Id, OVarDeclare},
	{OVarDeclare, Assign, Expr},
	{OVarDeclare, None},

	{FuncionCall, Expr, Lparenthesis, CallParams, Rparenthesis},
	{CallParams, Expr},
	{CallParams, Expr, Comma, CallParams},
	{CallParams, None},

};

static std::vector<RuleData> Data = {
	{},
	{None, None, Program},
	{},

	{None, None, ObjectDef},
	{None, None, FunctionDef},
	{None, None, NamespaceDef},

	{None, TypeString },
	{None, TypeInteger },
	{None, TypeFloat },

	{Id, None, MObjectVar},
	{None, None, MObjectVar},
	{None, ObjectDef },
	{Id},
	{None, None, Typename},
	{None, AnyType },
	{None, None, Expr },
	{ },

	{Id},
	{Id},

	{None, Scope, MStmt },
	{None, Scope, MStmt },
	{None, Scope, MStmt },
	{None, Scope },
	{ },

	{Add, Add },
	{Sub, Sub },
	{Mult, Mult },
	{Div, Div },

	{None, None, Conditional },
	{ },

	{None, None, Value },
	{Integer, Integer },
	{String, String },
	{Float, Float },
	{None, None, Identifier },
	{Id, Id },
	{Id, Id },

	{Assign, Assign},
	{None, None, VarDeclare},
	{None, None, FuncionCall},

	{Id, None, OVarDeclare},
	{None, VarDeclare},
	{None, VarDeclare},

	{ },
	{None, None},
	{None, None, CallParams},
	{ },
};

void Parser::InitializeParser()
{
	CreateParser(G, Rules);

	TokenParseData[Add] = { 50, Association::Left };
	TokenParseData[Sub] = { 50, Association::Left };
	TokenParseData[Mult] = { 60, Association::Left };
	TokenParseData[Div] = { 60, Association::Left };
	TokenParseData[Assign] = { 5, Association::Right };
	TokenParseData[Equal] = { 19, Association::Left };
	TokenParseData[Not] = { 20, Association::Left };
	TokenParseData[And] = { 18, Association::Left };
	TokenParseData[Or] = { 17, Association::Left };


	if (Rules.size() != Data.size()) {
		throw std::range_error("Rule data does not match rules");
	}
}

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

std::vector<std::string> splits(const std::string& s, char delim) {
	std::vector<std::string> elems;
	splits(s, delim, std::back_inserter(elems));
	return elems;
}

void Parser::InitializeGrammar(const char* grammar)
{
	gLogger() << LogLevel::Debug << "Initializing grammar...\n";

	std::fstream in(grammar, std::ios::in);
	if (!in.is_open()) {
		gLogger() << LogLevel::Debug << "No grammar found\n";
		return;
	}

	Rules.clear();
	Data.clear();

	bool rules = true;
	std::string line;
	while (std::getline(in, line)) {
		trim(line);
		if (line.empty()) continue;
		switch (*line.c_str())
		{
		case '#': {
			if (line == "#rules") rules = true;
			else if (line == "#data") rules = false;
		} break;

		case '.': 
			if (!rules) Data.push_back({});
			break;
		case '\n': break;

		default:
			const auto items = splits(line, ' ');
			if (rules) {
				Rules.push_back({});
				auto& target = Rules.back();
				for (const auto& i : items) {
					if (auto it = std::find_if(TokensToName.begin(), TokensToName.end(), [&i](const auto& data) { return i == data.second; }); it != TokensToName.end()) {
							target.push_back(it->first);
					}
				}
			}
			else {
				Data.push_back({});
				auto& target = Data.back();
				std::vector<Token> toks(3, {});
				int idx = 0;
				for (const auto& i : items) {
					if (auto it = std::find_if(TokensToName.begin(), TokensToName.end(), [&i](const auto& data) { return i == data.second; }); it != TokensToName.end()) {
						toks[idx++] = (it->first);
					}
				}
				target = { toks[0], toks[1], toks[2] };
			}
			break;
		}
	}

	if (Rules.size() != Data.size()) {
		gLogger() << LogLevel::Debug << "Invalid grammar\n";
		return;
	}

	ReleaseGrammar(G);
	CreateParser(G, Rules);
	gLogger()<< LogLevel::Debug << "Grammar compiled\n";
}

void Parser::ReleaseParser()
{
	ReleaseGrammar(G);
}

void Parser::ThreadedParse(VM* vm)
{
	while (vm->CompileRunning) {
		CompileOptions options;
		{
			std::unique_lock lk(vm->CompileMutex);
			vm->QueueNotify.wait(lk, [vm] {return !vm->CompileQueue.empty() || !vm->CompileRunning; });
			if (!vm->CompileRunning) return;

			options = vm->CompileQueue.front();
			vm->CompileQueue.pop();
		}
		if (options.Path != "")
			Parse(vm, options);
	}
}

struct TokenHolder
{
	Token token = None;
	Node* ptr = nullptr;
	std::string_view data;
};

void TypeConverter(Node* n, const TokenHolder& h) {
	switch (h.token)
	{
	case Integer:
		n->data = 0;
		std::from_chars(h.data.data(), h.data.data() + h.data.size(), std::get<int>(n->data));
		break;
	case Float:
		n->data = 0.f;
		std::from_chars(h.data.data(), h.data.data() + h.data.size(), std::get<int>(n->data));
		break;
	case True:
		n->data = true;
		break;
	case False:
		n->data = false;
		break;
	default:
		std::get<std::string>(n->data) = h.data;
		break;
	}
}

void Parser::Parse(VM*, const CompileOptions& options)
{
	if (!std::filesystem::exists(options.Path)) {
		gLogger() << LogLevel::Warning << MakePath(options.Path) << ": File not found\n";
		return;
	}

	Lexer lex(options.Path);

	if (!lex.IsValid()) return;

	bool useSimplify = options.UserOptions.Simplify;

	std::stack<Token> ss;

	std::stack<int> stateStack;
	std::stack<TokenHolder> symbolStack;
	std::stack<Token> operatorStack;
	stateStack.push(0);

	//int inputIndex = 0;
	TokenHolder holder;
	holder.token = lex.GetNext(holder.data);

	Node* ASTRoot = nullptr;

	bool notDone = true;
	while (notDone) {
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
			if (data.mergeToken == None && useSimplify) {
				next.ptr = new Node();
				next.ptr->type = data.nodeType == None ? next.token : data.nodeType;
			}
			else {
				next.ptr = new Node();
				next.ptr->type = next.token;
			}

			for (int i = 0; i < rule.development.size(); ++i) {
				if (rule.development[i] == None) break;
				if (!operatorStack.empty() && symbolStack.top().token == operatorStack.top()) operatorStack.pop();

				auto& old = symbolStack.top();
				if (useSimplify) {
					if (data.mergeToken == old.token) {
						next.ptr = old.ptr;
						if (data.nodeType != None) next.ptr->type = data.nodeType;
					}
					else if (old.ptr && next.ptr)
						next.ptr->children.push_front(old.ptr);
					else if (old.token == data.dataToken) {
						TypeConverter(next.ptr, old);
					}
				}
				else if(old.ptr && next.ptr)
					next.ptr->children.push_front(old.ptr);

				symbolStack.pop();
				stateStack.pop();
			}

			Optimize(next.ptr);

			symbolStack.push(next);
			if (TokenParseData[rule.nonTerminal].Precedence != 0) operatorStack.push(rule.nonTerminal);

			stateStack.push(G.ParseTable[stateStack.top()][rule.nonTerminal].shift);
		} break;

		case DECIDE: {
			auto& next = TokenParseData[holder.token];
			if (next.Precedence == 0) 
				goto SHIFTCASE;
			if (operatorStack.empty()) goto SHIFTCASE;

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
			gLogger() << LogLevel::Info << "Parse successful\n";
			notDone = false;
			ASTRoot = symbolStack.top().ptr;
		} break;

		ERRORCASE:
		default: {
			const auto& c = lex.GetContext();
			gLogger() << LogLevel::Error << MakePath(options.Path) 
				<< std::format(" (%lld, %lld)", c.Row, c.Column) 
				<< ": Critical error found '" << holder.data << "\n";
			notDone = false;
			break;
		}
		}
	}

	// @todo: remove debug code
	if (ASTRoot) {
		gLogger() << "AST\n";
		ASTRoot->print("", true);
		gLogger() << "\n\n";
	}


}
