#include "Parser.h"
#include <functional>
#include <stack>
#include <iostream>
#include <ankerl/unordered_dense.h>
#include <filesystem>

#include "AST.h"
#include "Lexer.h"
#include "ParseHelper.h"

#ifdef EMI_PARSE_GRAMMAR
#include "ParseTable.h"
#endif

#include "grammar.h"

#define TOKEN(t) (int)Token::t

static std::vector<TokenData> TokenParseData(TOKEN(Last), TokenData{});
static RuleType Rules = {};

void Parser::InitializeParser()
{
	TokenParseData[TOKEN(Add)] = { 50, Association::Left };
	TokenParseData[TOKEN(Sub)] = { 50, Association::Left };
	TokenParseData[TOKEN(Mult)] = { 60, Association::Left };
	TokenParseData[TOKEN(Div)] = { 60, Association::Left };
	TokenParseData[TOKEN(Assign)] = { 5, Association::Right };
	TokenParseData[TOKEN(Equal)] = { 19, Association::Left };
	TokenParseData[TOKEN(NotEqual)] = { 19, Association::Left };
	TokenParseData[TOKEN(Less)] = { 19, Association::Left };
	TokenParseData[TOKEN(LessEqual)] = { 19, Association::Left };
	TokenParseData[TOKEN(Larger)] = { 19, Association::Left };
	TokenParseData[TOKEN(LargerEqual)] = { 19, Association::Left };
	TokenParseData[TOKEN(Conditional)] = { 15, Association::Left };
	TokenParseData[TOKEN(Opt)] = { 15, Association::Left };
	TokenParseData[TOKEN(Not)] = { 50, Association::Left };
	TokenParseData[TOKEN(And)] = { 17, Association::Left };
	TokenParseData[TOKEN(Or)] = { 17, Association::Left };
	TokenParseData[TOKEN(Literal)] = { 20, Association::Left };
	TokenParseData[TOKEN(StrDelimiterL)] = { 20, Association::Left };
	TokenParseData[TOKEN(StrDelimiterR)] = { 20, Association::Left };
	TokenParseData[TOKEN(OFormatArg)] = { 29, Association::Left };
}

void Parser::InitializeGrammar([[maybe_unused]] const char* grammar)
{
#ifdef EMI_PARSE_GRAMMAR
	if (!std::filesystem::exists(grammar)) {
		gError() << "Grammar not found";
		return;
	}
	auto lastTime = std::filesystem::last_write_time(grammar);
	size_t count = lastTime.time_since_epoch().count();
	
	gDebug() << "Initializing grammar...";

	if (count > CreateTime) {

		std::fstream in(grammar, std::ios::in);
		if (!in.is_open()) {
			gDebug() << "No grammar found";
			return;
		}

		Rules.clear();
		Data.clear();

		bool rules = false;
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
			gDebug() << "Invalid grammar";
			return;
		}

		ParseTable.clear();
		RuleTable.clear();
		CreateParser(ParseTable, RuleTable, Data, Rules);
		Rules.clear();
	}

	gDebug() << "Grammar compiled";
#endif
}

void Parser::ReleaseParser()
{
}

void Parser::ThreadedParse(VM* vm)
{
	while (vm->CompileRunning) {
		CompileOptions options;
		{
			std::unique_lock lk(vm->CompileMutex);
			vm->QueueNotify.wait(lk, [vm] {return !vm->CompileQueue.empty() || !vm->CompileRunning; });
			if (!vm->CompileRunning) return;

			options = std::move(vm->CompileQueue.front());
			vm->CompileQueue.pop();
		}
		if (options.Path != "")
			Parse(vm, options);
	}
}

void Parser::Parse(VM* vm, CompileOptions& options)
{
	auto fullPath = MakePath(options.Path);
	if (options.Data.size() == 0) {
		gDebug() << "Parsing file " << fullPath;
		if (!std::filesystem::exists(options.Path)) {
			gWarn() << fullPath << ": File not found";
			options.CompileResult.set_value(false);
			return;
		}
		vm->RemoveUnit(fullPath);
	}
	else {
		if (options.Data.size() == 0) {
			gError() << "No data given";
			options.CompileResult.set_value(false);
			return;
		}
	}

	gDebug() << "Constructing AST";
	auto root = ConstructAST(options);
	if (!root) {
		gError() << fullPath << ": Parse failed";
		options.CompileResult.set_value(false);
		return;
	}

	Desugar(root);

#ifdef DEBUG
	root->print("");
#endif // DEBUG
	gDebug() << "Walking AST";
	ASTWalker ast(vm, root);
	ast.Run();

	if (!ast.HasError) {
		vm->AddNamespace(fullPath, ast.Global);
		ast.Global.Table.clear();
		options.CompileResult.set_value(true);
	}
	else {
		gError() << "Errors present, compile failed: " << fullPath;
		options.CompileResult.set_value(false);
	}
}

Node* Parser::ConstructAST(CompileOptions& options)
{
	if (options.Data.size() == 0) {
		std::fstream data(options.Path, std::ios::in);

		if (data.is_open()) {

			data.seekg(0, std::ios::end);
			size_t size = data.tellg();
			options.Data.resize(size + 1);
			data.seekg(0);
			data.read(&options.Data[0], size);

		}
		else {
			gLogger() << LogLevel::Warning << MakePath(options.Path) << ": Cannot open file";
		}
	}

	Lexer lex(options.Data.c_str(), options.Data.size());

	if (!lex.IsValid()) return nullptr;

	std::stack<Token> ss;

	std::stack<int, std::vector<int>> stateStack;
	std::stack<TokenHolder, std::vector<TokenHolder>> symbolStack;
	std::stack<Token, std::vector<Token>> operatorStack;
	stateStack.push(0);

	//int inputIndex = 0;
	TokenHolder holder;
	holder.token = lex.GetNext(holder.data);

	Node* ASTRoot = nullptr;

	bool notDone = true;
	while (notDone)
	{
		int currentState = stateStack.top();

		ActionNode action = ParseTable[currentState][(int)holder.token];

		switch (action.type)
		{
		SHIFTCASE:
		case Action::SHIFT: {
			stateStack.push(action.shift);
			symbolStack.push(holder);
			if (TokenParseData[(int)holder.token].Precedence != 0) 
				operatorStack.push(holder.token);
			holder.token = lex.GetNext(holder.data);
			holder.ptr = nullptr;
		} break;
		
		REDUCECASE:
		case Action::REDUCE: {
			const Rule& rule = RuleTable[action.reduce];
			const RuleData data = Data[action.reduce];

			TokenHolder next;
			next.token = rule.nonTerminal;


			if (data.mergeToken == Token::None) {
				next.ptr = new Node();
				next.ptr->type = data.nodeType == Token::None ? next.token : data.nodeType;
				next.ptr->depth = 0;
				next.ptr->line = lex.GetContext().Row;
			}
			else {
				//next.ptr->type = next.token;
			}

			thread_local static std::vector<Node*> childrenList;
			childrenList.clear();
			size_t maxDepth = 0;
			NodeDataType nodeData;
			bool hasData = false;

			for (size_t i = 0; i < rule.development.size(); ++i) {
				if (rule.development[i] == Token::None) break;
				if (!operatorStack.empty() && symbolStack.top().token == operatorStack.top()) operatorStack.pop();

				auto& old = symbolStack.top();

				if (data.mergeToken == old.token) {
					if (next.ptr) delete next.ptr;
					next.ptr = old.ptr;
					if (data.nodeType != Token::None) next.ptr->type = data.nodeType;
				}
				else {
					if (old.ptr) {
						childrenList.push_back(old.ptr);
						maxDepth = std::max(maxDepth, old.ptr->depth + 1);
					}
					else if (old.token == data.dataToken) {
						TypeConverter(nodeData, old);
						hasData = true;
					}
				}

				symbolStack.pop();
				stateStack.pop();
			}
			if (next.ptr) {
				if (hasData) next.ptr->data = nodeData;
				next.ptr->depth = std::max(maxDepth, next.ptr->depth);
				next.ptr->children.reserve(next.ptr->children.size() + childrenList.size());
				for (auto it = childrenList.begin(); it != childrenList.end(); it++) {
					next.ptr->children.insert(next.ptr->children.begin(), *it);
				}
				Optimize(next.ptr);
			}

			symbolStack.push(next);
			if (TokenParseData[(int)rule.nonTerminal].Precedence != 0) operatorStack.push(rule.nonTerminal);

			stateStack.push(ParseTable[stateStack.top()][(int)rule.nonTerminal].shift);
		} break;

		case DECIDE: {
			auto& next = TokenParseData[(int)holder.token];
			if (next.Precedence == 0) 
				goto SHIFTCASE;
			if (operatorStack.empty()) goto SHIFTCASE;

			auto& prev = TokenParseData[(int)operatorStack.top()];
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
				<< " (" << c.Row << ", " << c.Column << ")"
				<< ": Critical error found '" << holder.data << "'. ";
#ifdef DEBUG
			gLogger() << "Expected one of: ";
			int idx = 0;
			for (auto& state : ParseTable[currentState]) {
				if (state.type != ERROR) {
					gLogger() << TokensToName[(Token)idx] << ", ";
				}
				idx++;
			}
#endif // DEBUG
			notDone = false;
			break;
		}
		}
	}

	return ASTRoot;
}