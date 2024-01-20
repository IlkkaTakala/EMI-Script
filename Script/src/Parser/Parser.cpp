#include "Parser.h"
#include <functional>
#include <stack>
#include <iostream>
#include <ankerl/unordered_dense.h>
#include <filesystem>

#include "Lexer.h"
#include "ParseHelper.h"
#include "ParseTable.h"
#include "AST.h"

static Grammar G;
static std::vector<TokenData> TokenParseData(Last, TokenData{});
static RuleType Rules = {};
static std::vector<RuleData> Data = {};

void Parser::InitializeParser()
{
	TokenParseData[Add] = { 50, Association::Left };
	TokenParseData[Sub] = { 50, Association::Left };
	TokenParseData[Mult] = { 60, Association::Left };
	TokenParseData[Div] = { 60, Association::Left };
	TokenParseData[Assign] = { 5, Association::Right };
	TokenParseData[Equal] = { 19, Association::Left };
	TokenParseData[Not] = { 50, Association::Left };
	TokenParseData[And] = { 18, Association::Left };
	TokenParseData[Or] = { 17, Association::Left };


	if (Rules.size() != Data.size()) {
		throw std::range_error("Rule data does not match rules");
	}
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
		if (options.Path != "" || options.Handle == 0)
			Parse(vm, options);
	}
}

void Parser::Parse(VM* vm, CompileOptions& options)
{
	auto fullPath = MakePath(options.Path);
	if (options.Handle != 0) {
		gDebug() << "Parsing file " << fullPath << '\n';
		if (!std::filesystem::exists(options.Path)) {
			gLogger() << LogLevel::Warning << fullPath << ": File not found\n";
			return;
		}
		vm->RemoveUnit(fullPath);
	}
	else {
		if (Data.size() == 00) {
			gError() << "No data given\n";
			return;
		}
	}

	gDebug() << "Constructing AST\n";
	auto root = ConstructAST(options);
	if (!root) {
		gLogger() << LogLevel::Error << fullPath << ": Parse failed\n";
		return;
	}

	root->print("");
	gDebug() << "Walking AST\n";
	ASTWalker ast(vm, root);
	ast.Run();

	if (!ast.HasError) {
		vm->AddNamespace(fullPath, ast.namespaces);
	}
}

Node* Parser::ConstructAST(CompileOptions& options)
{
	if (options.Handle != 0) {
		std::fstream data(options.Path, std::ios::in);

		if (data.is_open()) {

			data.seekg(0, std::ios::end);
			size_t size = data.tellg();
			options.Data.resize(size + 1);
			data.seekg(0);
			data.read(&options.Data[0], size);

		}
		else {
			gLogger() << LogLevel::Warning << MakePath(options.Path) << ": Cannot open file\n";
		}
	}

	Lexer lex(options.Data.c_str(), options.Data.size());

	if (!lex.IsValid()) return nullptr;

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
			if (useSimplify) {
				next.ptr = new Node();
				if (data.mergeToken == None) {
					next.ptr->type = data.nodeType == None ? next.token : data.nodeType;
				}
				else {
					next.ptr->type = next.token;
				}
				next.ptr->depth = 0;
				next.ptr->line = lex.GetContext().Row;
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
						if (next.ptr) delete next.ptr;
						next.ptr = old.ptr;
						if (data.nodeType != None) next.ptr->type = data.nodeType;
					}
					else if (old.ptr && next.ptr) {
						next.ptr->children.push_front(old.ptr);
						next.ptr->depth = std::max(next.ptr->depth, old.ptr->depth + 1);
					}
					else if (old.token == data.dataToken) {
						TypeConverter(next.ptr, old);
					}
				}
				else if(old.ptr && next.ptr)
					next.ptr->children.push_front(old.ptr);

				symbolStack.pop();
				stateStack.pop();
			}
			if (next.ptr) {
				Optimize(next.ptr);
			}

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
			notDone = false;
			ASTRoot = symbolStack.top().ptr;
		} break;

		ERRORCASE:
		default: {
			const auto& c = lex.GetContext();
			gError() << MakePath(options.Path) 
				<< std::format(" ({}, {})", c.Row, c.Column) 
				<< ": Critical error found '" << holder.data << "'. Expected one of: ";
			int idx = 0;
			for (auto& state : G.ParseTable[currentState]) {
				if (state.type != ERROR) {
					gLogger() << TokensToName[(Token)idx] << ", ";
				}
				idx++;
			}
			gLogger() << '\n';
			notDone = false;
			break;
		}
		}
	}

	return ASTRoot;
}