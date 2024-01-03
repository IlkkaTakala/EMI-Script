#include "VM.h"
#include "Parser.h"
#include "NativeFuncs.h"

VM::VM()
{
	Parser::InitializeParser();
	auto counter = std::thread::hardware_concurrency();
	CompileRunning = true;

	VMRunning = true;
	for (uint i = 0; i < counter; i++) {
		ParserPool.emplace_back(Parser::ThreadedParse, this);
		RunnerPool.emplace_back(Runner(this));
	}
}

VM::~VM()
{
	CompileRunning = false;
	VMRunning = false;
	QueueNotify.notify_all();
	CallQueueNotify.notify_all();
	for (auto& t : ParserPool) {
		t.join();
	}
	for (auto& t : RunnerPool) {
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

VariableHandle VM::CallFunction(FunctionHandle handle, const std::vector<Variable>& args)
{
	auto it = FunctionMap.find((uint16)handle);
	if (it == FunctionMap.end()) return {0};

	CallObject* call = new CallObject(&it->second);
	call->Arguments = args;

	std::unique_lock lk(CallMutex);
	CallQueue.push(call);
	CallQueueNotify.notify_one();

	//call->Return.get_future();
	return { 0 };
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

void Runner::operator()()
{
	while (Owner->IsRunning()) {

		CallObject * f;
		{
			std::unique_lock lk(Owner->CallMutex);
			Owner->CallQueueNotify.wait(lk, [&]() {return !Owner->CallQueue.empty() || !Owner->IsRunning(); });
			if (!Owner->IsRunning()) return;

			f = Owner->CallQueue.front();
			Owner->CallQueue.pop();
		}
		CallStack.emplace(f);

		//auto& target = CallStack.top();
		bool interrupt = true;
		const uint8* bytecodePtr = f->FunctionPtr->Bytecode.data();
		const uint8* codeEnd = bytecodePtr + f->FunctionPtr->Bytecode.size();

		while (interrupt && bytecodePtr < codeEnd && Owner->IsRunning()) {
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
}

CallObject::CallObject(Function* function)
{
	FunctionPtr = function;
	StackOffset = 0;
	Location = 0;
	
}
