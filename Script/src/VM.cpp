#include "VM.h"
#include "Parser/Parser.h"
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

void VM::CompileTemporary(const char* data)
{
	CompileOptions fulloptions{};
	fulloptions.Handle = 0;
	fulloptions.Data = data;
	fulloptions.UserOptions.Simplify = true;
	{
		std::lock_guard lock(CompileMutex);
		CompileQueue.push(fulloptions);
	}
	QueueNotify.notify_one();
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
	if (it == FunctionMap.end()) return (size_t)-1;

	CallObject* call = new CallObject(it->second);

	call->Arguments.reserve(args.size());
	for (auto& v : args) {
		moveOwnershipToVM(v);
		call->Arguments.push_back(v);
	}

	size_t idx = 0;
	if (ReturnFreeList.empty()) {
		ReturnValues.push_back(call->Return.get_future());
		idx = ReturnValues.size() - 1;
	}
	else {
		idx = ReturnFreeList.back();
		ReturnFreeList.pop_back();
		ReturnValues[idx] = call->Return.get_future();
	}

	std::unique_lock lk(CallMutex);
	CallQueue.push(call);
	CallQueueNotify.notify_one();

	return idx;
}

Variable VM::GetReturnValue(size_t index)
{
	if (ReturnValues.size() <= index || ReturnFreeList.end() != std::find(ReturnFreeList.begin(), ReturnFreeList.end(), index)) return {};
	Variable var = ReturnValues[index].get();
	ReturnFreeList.push_back(index);
	moveOwnershipToHost(var);
	return var;
}

void VM::AddNamespace(ankerl::unordered_dense::map<std::string, Namespace>& space)
{
	std::unique_lock lk(MergeMutex);
	for (auto& [name, s] : space) {
		for (auto& [oname, obj] : s.objects) {
			auto type = Manager.AddType(oname, obj);
			s.objectTypes[oname] = type;
		}
		s.objects.clear();
		for (auto& [fname, func] : s.functions) {
			auto full = name + '.' + fname;
			if (auto it = NameToFunctionMap.find(full); it != NameToFunctionMap.end()) {
				gWarn() << "Multiple definitions for function " << full << '\n';
			}
			auto idx = ++FunctionHandleCounter;
			NameToFunctionMap[full] = idx;
			FunctionMap[idx] = func;
		}

		if (auto it = Namespaces.find(name); it != Namespaces.end()) {
			it->second.merge(s);
		}
		else {
			Namespaces[name] = s;
		}
	}
}

Runner::Runner(VM* vm) : Owner(vm)
{

}

Runner::~Runner()
{

}

#define TARGET(Op) case OpCodes::Op
#define Read16() *(uint16*)current->Ptr
#define Read8() *(uint8*)current->Ptr

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

		bool interrupt = true;
		CallObject* current = CallStack.top();
		current->Ptr = current->FunctionPtr->Bytecode.data();
		current->End = current->Ptr + current->FunctionPtr->Bytecode.size();

		auto nums = &f->FunctionPtr->numberTable.values();


		while (interrupt && current->Ptr < current->End && Owner->IsRunning()) {
			switch ((OpCodes)*current->Ptr++)
			{
				TARGET(JumpForward) : {
					current->Ptr += Read16();
				} break;

				TARGET(JumpBackward) : {
					current->Ptr -= Read16();
				} break;

				TARGET(LoadNumber) : {
					Stack.push((*nums)[Read16()]);
					current->Ptr += 2;
				} break;

				TARGET(Return) : {
					auto val = Stack.pop();
					if (current->StackOffset == 0) {
						current->Return.set_value(val);
						Stack.to(0);
					}
					else {
						Stack.to(current->StackOffset);
						Stack.push(val);
					}
					CallStack.pop();
					if (!CallStack.empty()) {
						current = CallStack.top();
					}
					else {
						current = nullptr;
						interrupt = false;
					}
				} break;

				TARGET(PushUndefined) : {
					Variable var;
					var.setUndefined();
					Stack.push(var);
				}

			default:
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
	Ptr = nullptr;
	End = nullptr;
}
