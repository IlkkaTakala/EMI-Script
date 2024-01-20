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
	if (it == FunctionMap.end()) {
		gWarn() << "Invalid function handle\n";
		return (size_t)-1;
	}
	if (!it->second->IsPublic) {
		gWarn() << "Function is private\n";
		return (size_t)-1;
	}

	CallObject* call = new CallObject(it->second);

	call->Arguments.reserve(args.size());
	for (int i = 0; i < call->FunctionPtr->ArgCount && i < args.size(); i++) {
		if (call->FunctionPtr->Types[i + 1] == args[i].getType()) {
			moveOwnershipToVM(args[i]);
			call->Arguments.push_back(args[i]);
		}
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
		current->StackOffset = 0;

		Registers.reserve(current->FunctionPtr->RegisterCount);
		Registers.to(0);

		for (int i = 0; i < current->FunctionPtr->ArgCount && i < f->Arguments.size(); i++) {
			Registers[i] = f->Arguments[i];
		}

#define NUMS current->FunctionPtr->numberTable.values()
#define JUMPS current->FunctionPtr->jumpTable.values()

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
					current->Ptr = &current->FunctionPtr->Bytecode.data()[JUMPS[byte.param]];
				} break;

				TARGET(LoadNumber) : {
					Registers[byte.target] = NUMS[byte.param];
				} break;

				TARGET(Return) : {
					Variable val;
					if (byte.in1 == 1) {
						val = Registers[byte.target];
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
						Registers.to(current->StackOffset);
						const Instruction& oldByte = *(Instruction*)(current->Ptr - 1);
						Registers[oldByte.target] = val;
					}
					else {
						current = nullptr;
						interrupt = false;
					}
				} break;

				TARGET(CallFunction) : {

					auto& fn = current->FunctionPtr->functionTable[byte.in1];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->functionTableSymbols[byte.in1];
						if (auto idx = Owner->NameToFunctionMap.find(name); idx != Owner->NameToFunctionMap.end()) {
							if (auto it = Owner->FunctionMap.find(idx->second); it != Owner->FunctionMap.end()) {
								fn = it->second;
							}
						}
						else {
							name = current->FunctionPtr->Namespace + "." + name;
							if (idx = Owner->NameToFunctionMap.find(name); idx != Owner->NameToFunctionMap.end()) {
								if (auto it = Owner->FunctionMap.find(idx->second); it != Owner->FunctionMap.end()) {
									fn = it->second;
								}
							}
							else {
								gWarn() << "Function does not exist: " << name << "\n";
								break;
							}
						}
					}
					if (!fn->IsPublic && current->FunctionPtr->NamespaceHash != fn->NamespaceHash) {
						gWarn() << "Cannot call private function " << fn->Name << "\n";
						break;
					}
					
					CallObject* call = new CallObject(fn); // @todo: do this better, too slow
					call->StackOffset = current->StackOffset + byte.in2;
					Registers.to(call->StackOffset);
					Registers.reserve(fn->RegisterCount);

					CallStack.push(call);
					current = call;
				} break;

				TARGET(PushUndefined) : {
					Registers[byte.target].setUndefined();
				} break;
				TARGET(PushBoolean) : {
					Registers[byte.target] = byte.in1 == 1 ? true : false;
				} break;
				TARGET(PushTypeDefault) : {
					Registers[byte.target] = GetTypeDefault((VariableType)byte.param);
				} break;

				TARGET(Copy) : {
					Registers[byte.target] = Registers[byte.in1];
				} break;

				TARGET(PreMod) : {
					if (byte.in2 == 0) {
						Registers[byte.target] = Registers[byte.target].as<double>() + 1.0;
					}
					else {
						Registers[byte.target] = Registers[byte.target].as<double>() - 1.0;
					}
				} break;
				
				TARGET(PostMod) : {
					if (byte.in2 == 0) {
						Registers[byte.in1] = Registers[byte.in1].as<double>() + 1.0;
						Registers[byte.target] = Registers[byte.in1];
					}
					else {
						Registers[byte.in1] = Registers[byte.in1].as<double>() - 1.0;
						Registers[byte.target] = Registers[byte.in1];
					}
				} break;

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
				
				TARGET(Equal) : {
					Registers[byte.target] = Registers[byte.in1] == Registers[byte.in2]; // @todo: fix this
				} break;

				TARGET(Not) : {
					Registers[byte.target] = !isTruthy(Registers[byte.in1]); // @todo: fix this
				} break;

				TARGET(JumpEq) : {
					if (isTruthy(Registers[byte.target])) {
						current->Ptr += byte.param;
					}
				} break;

				TARGET(JumpNeg) : {
					if (!isTruthy(Registers[byte.target])) {
						current->Ptr += byte.param;
					}
				} break;

				TARGET(Less) : {
					Registers[byte.target] = Registers[byte.in1].as<double>() < Registers[byte.in2].as<double>();
				} break;

			default:
				gError() << "Unknown bytecode detected\n";
				break;
			}
		}
	}
}

CallObject::CallObject(Function* function)
{
	FunctionPtr = function;
	StackOffset = 0;
	Ptr = function->Bytecode.data();
	End = function->Bytecode.data() + function->Bytecode.size();
}
