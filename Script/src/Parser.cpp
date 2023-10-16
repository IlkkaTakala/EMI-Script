#include "Parser.h"
#include <functional>
#include <set>
#include <stack>
#include <iostream>
#include <unordered_map>

#include "ParseTable.h"

static const auto ParseTable = CreateParser();

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

void Parser::Parse(VM* vm, CompileOptions& options)
{
	Lexer lex(options.Path);

	int count = 0;
	auto token = Token::None;
	std::string_view view;
	std::stack<Token> ss;

	std::stack<int> stateStack;
	std::stack<Token> symbolStack;
	std::stack<Token> parseStack;
	stateStack.push(0);

	//int inputIndex = 0;
	Token currentToken = lex.GetNext(view);

	while (true) {
		int currentState = stateStack.top();

		int tokenIndex = 0; // = static_cast<int>(currentToken);
		if (auto it = std::find(_symbols.begin(), _symbols.end(), currentToken); it != _symbols.end()) {
			tokenIndex = int(it - _symbols.begin());
		}

		ActionNode action = ParseTable[currentState][tokenIndex];

		switch (action.type)
		{
		case Action::SHIFT: {
			stateStack.push(action.state);
			symbolStack.push(currentToken);
			currentToken = lex.GetNext(view);
		} break;

		case Action::REDUCE: {
			Rule& rule = action.rule;
			for (int i = 0; i < rule.rhs.size(); ++i) symbolStack.pop();

			symbolStack.push(_symbols[rule.lhs]);

			stateStack.pop();
			int i = 0;
			while (i < 1) {
				i = ParseTable[stateStack.top()][rule.lhs].state;
				if (i > 0)
					stateStack.push(i);
				else stateStack.pop();
			}
			
			// Pop symbols from the stack and state based on the rule
			// Push the non-terminal and update the state based on the goto table
		} break;

		case Action::ACCEPT: {
			std::cout << "Accepted\n";
		} break;

		default: {
			std::cout << "Error\n";
			break;
		}
		}
	}

	//std::vector<Data> TokenList;
	//TokenList.reserve(100);

	//do {
	//	token = lex.GetNext(view);
	//	TokenList.emplace_back(token, view);
	//} while (token != Token::None);

	//Iterator iter(TokenList.begin());
	//Node* start;
	//Rules[0].Try(start, iter, Rules, { (int)Token::None });
}
