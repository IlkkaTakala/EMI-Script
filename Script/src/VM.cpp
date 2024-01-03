#include "VM.h"
#include "Parser.h"
#include "NativeFuncs.h"
#include "Helpers.h"

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

size_t VM::GetFunctionID(const std::string& name)
{
	if (auto it = NameToFunctionMap.find(name); it != NameToFunctionMap.end()) {
		return it->second;
	}
	return 0;
}

size_t VM::CallFunction(FunctionHandle handle, const std::span<Variable>& args)
{
	auto it = FunctionMap.find((uint16)handle);
	if (it == FunctionMap.end()) return {0};

	CallObject* call = new CallObject(&it->second);

	call->Arguments.reserve(args.size());
	for (auto& v : args) {
		moveOwnershipToVM(v);
		call->Arguments.push_back(v);
	}

	size_t idx = 0;
	if (ReturnFreeList.empty()) {
		ReturnValues.push_back(call->Return.get_future());
		idx = ReturnValues.size();
	}
	else {
		idx = ReturnFreeList.top();
		ReturnFreeList.pop();
		ReturnValues[idx] = call->Return.get_future();
	}

	std::unique_lock lk(CallMutex);
	CallQueue.push(call);
	CallQueueNotify.notify_one();

	return idx;
}

Variable VM::GetReturnValue(size_t index)
{
	if (ReturnValues.size() <= index) return {};
	return ReturnValues[index].get();
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
