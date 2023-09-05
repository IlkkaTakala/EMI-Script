#include "Parser.h"

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

	auto token = Token::None;
	std::string_view view;
	do {
		token = lex.GetNext(view);
	} while (token != Token::None);
}
