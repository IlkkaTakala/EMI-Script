#include "VM.h"
#include "Parser.h"
#include "NativeFuncs.h"

VM::VM()
{
	Parser::InitializeParser();
	auto counter = std::thread::hardware_concurrency();
	CompileRunning = true;

	for (uint i = 0; i < counter; i++) {
		ParserPool.emplace_back(Parser::ThreadedParse, this);
	}
}

VM::~VM()
{
	CompileRunning = false;
	QueueNotify.notify_all();
	for (auto& t : ParserPool) {
		t.join();
	}
	Parser::ReleaseParser();
}

void VM::ReinitializeGrammar(const char* grammar)
{
	Parser::InitializeGrammar(grammar);
}

ScriptHandle VM::Compile(const char* path, const Options& options)
{
	ScriptHandle handle;
	{
		std::lock_guard lock(HandleMutex);
		handle = (ScriptHandle)++HandleCounter;
	}

	CompileOptions fulloptions{};
	fulloptions.Handle = handle;
	fulloptions.Path = path;
	fulloptions.UserOptions = options;

	{
		std::lock_guard lock(CompileMutex);
		CompileQueue.push(fulloptions);
	}
	QueueNotify.notify_one();

	return handle;
}

std::vector<void(*)(const uint8*& ptr, const uint8* end)> OpCodeTable = {
	IntAdd
};

Runner::Runner(VM* vm) : Owner(vm)
{

}

Runner::~Runner()
{

}

#define TARGET(Op) case OpCodes::Op

void Runner::operator()(const Function& f)
{
	CallStack.emplace(&f, 0);

	//auto& target = CallStack.top();
	bool interrupt = true;
	const uint8* bytecodePtr = f.Bytecode.data();
	const uint8* codeEnd = bytecodePtr + f.Bytecode.size();

	while (interrupt && bytecodePtr < codeEnd) {
		switch ((OpCodes)*bytecodePtr)
		{
			TARGET(JumpForward) : {
				bytecodePtr += 0;
			} break;




		default:
			OpCodeTable[*bytecodePtr](++bytecodePtr, codeEnd);
			break;
		}
	}
}
