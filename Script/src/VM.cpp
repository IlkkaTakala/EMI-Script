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

void VM::AddNamespace(const std::string& path, ankerl::unordered_dense::map<std::string, Namespace>& space)
{
	std::unique_lock lk(MergeMutex);
	for (auto& [name, s] : space) {
		for (auto& [oname, obj] : s.objects) {
			auto type = Manager.AddType(oname, obj);
			s.objectTypes[oname] = type;
			Units[path].objects.push_back({ name, oname });
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
			Units[path].functions.push_back({ name, fname });
		}

		if (auto it = Namespaces.find(name); it != Namespaces.end()) {
			it->second.merge(s);
		}
		else {
			Namespaces[name] = s;
		}
	}
}

void VM::RemoveUnit(const std::string& unit)
{
	std::unique_lock lk(MergeMutex);
	if (auto it = Units.find(unit); it != Units.end()) {
		auto& u = it->second;
		for (auto& [name, function] : u.functions) {
			Namespace& n = Namespaces[name];

			n.symbols.erase(name);
			delete n.functions[function.c_str()];
			n.functions.erase(function.c_str());
			auto full = name + '.' + function;
			if (auto named = NameToFunctionMap.find(full); named != NameToFunctionMap.end()) {
				FunctionMap.erase(named->second);
				NameToFunctionMap.erase(full);
			}
		}
		for (auto& [name, obj] : u.objects) {
			Manager.RemoveType(name);
			Namespace& n = Namespaces[name];
			n.objects.erase(name);
			n.objectTypes.erase(name);
			n.symbols.erase(name);
		}
		
		Units.erase(unit);
	}
}

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

		bool interrupt = true;
		CallObject* current = CallStack.top();
		current->Ptr = current->FunctionPtr->Bytecode.data();
		current->End = current->Ptr + current->FunctionPtr->Bytecode.size();

		Registers.reserve(current->FunctionPtr->RegisterCount);

		auto& nums = f->FunctionPtr->numberTable.values();


		while (interrupt && current->Ptr < current->End && Owner->IsRunning()) {
			const Instruction& byte = *(Instruction*)current->Ptr++;
			switch (byte.code)
			{
				TARGET(JumpForward) : {
					current->Ptr += byte.param;
				} break;

				TARGET(JumpBackward) : {
					current->Ptr -= byte.param;
				} break;

				TARGET(Jump) : {
					current->Ptr = &current->FunctionPtr->Bytecode.data()[byte.param];
				} break;

				TARGET(LoadNumber) : {
					Registers[byte.target] = nums[byte.param];
				} break;

				TARGET(Return) : {
					if (byte.target == 1) {
						auto& val = Registers[byte.in1];
						if (current->StackOffset == 0) {
							current->Return.set_value(val);
							Registers.to(0);
						}
						else {
							Registers.to(current->StackOffset);
							// @todo: return value
						}
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
					Registers[byte.target].setUndefined();
				}
				TARGET(PushBoolean) : {
					Registers[byte.target] = byte.in1 == 1 ? true : false;
				}

				TARGET(NumAdd) : {
					Registers[byte.target] = Registers[byte.in1].as<double>() + Registers[byte.in2].as<double>();
				} break;

				TARGET(NumSub) : {
					Registers[byte.target] = Registers[byte.in1].as<double>() - Registers[byte.in2].as<double>();
				} break;

				TARGET(NumDiv) : {
					Registers[byte.target] = Registers[byte.in1].as<double>() / Registers[byte.in2].as<double>();
				} break;

				TARGET(NumMul) : {
					Registers[byte.target] = Registers[byte.in1].as<double>() * Registers[byte.in2].as<double>();
				} break;

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
