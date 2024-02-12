#include "VM.h"
#include "Parser/Parser.h"
#include "NativeFuncs.h"
#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"

VM::VM()
{
	Parser::InitializeParser();
	auto counter = std::thread::hardware_concurrency();
	CompileRunning = true;

	VMRunning = true;
	for (uint i = 0; i < counter / 2; i++) {
		ParserPool.emplace_back(Parser::ThreadedParse, this);
		RunnerPool.emplace_back(Runner(this));
	}

	GarbageCollector = new std::thread(&VM::GarbageCollect, this);
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
	GarbageCollector->join();
	delete GarbageCollector;
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

void* VM::GetFunctionID(const std::string& name)
{
	if (name.find('.') == std::string::npos) {
		if (auto it = NameToFunctionMap.find("Global." + name); it != NameToFunctionMap.end()) {
			return it->second;
		}
		return 0;
	}
	if (auto it = NameToFunctionMap.find(name); it != NameToFunctionMap.end()) {
		return it->second;
	}
	return 0;
}

size_t VM::CallFunction(FunctionHandle handle, const std::span<InternalValue>& args)
{
	Function* fn = (Function*)(void*)handle;
	auto it = ValidFunctions.find(fn);
	if (it == ValidFunctions.end()) {
		gWarn() << "Invalid function handle\n";
		return (size_t)-1;
	}

	if (!fn->IsPublic) {
		gWarn() << "Function is private\n";
		return (size_t)-1;
	}

	CallObject& call = CallQueue.emplace(fn);

	call.Arguments.reserve(args.size());
	for (int i = 0; i < call.FunctionPtr->ArgCount && i < args.size(); i++) {
		auto val = moveOwnershipToVM(args[i]);
		if (call.FunctionPtr->Types[i + 1] == val.getType()) {
			call.Arguments.push_back(val);
		}
	}

	size_t idx = 0;
	if (ReturnFreeList.empty()) {
		auto& promise = ReturnPromiseValues.emplace_back();
		ReturnValues.push_back(promise.get_future());
		idx = ReturnValues.size() - 1;
		call.PromiseIndex = idx;
	}
	else {
		idx = ReturnFreeList.back();
		ReturnFreeList.pop_back();
		call.PromiseIndex = idx;
		ReturnPromiseValues[idx] = std::promise<Variable>();
		ReturnValues[idx] = ReturnPromiseValues[idx].get_future();
	}

	std::unique_lock lk(CallMutex);
	CallQueueNotify.notify_one();

	return idx;
}

InternalValue VM::GetReturnValue(size_t index)
{
	if (ReturnValues.size() <= index || ReturnFreeList.end() != std::find(ReturnFreeList.begin(), ReturnFreeList.end(), index)) return {};
	Variable var = ReturnValues[index].get();
	ReturnFreeList.push_back(index);
	auto val = moveOwnershipToHost(var);
	return val;
}

Symbol* VM::FindSymbol(const std::string& name, const std::string& space, bool& isNamespace)
{
	if (auto it = Namespaces.find(name); it != Namespaces.end()) {
		isNamespace = true;
		return it->second.Sym;
	}
	isNamespace = false;
	if (auto it = Namespaces.find(space); it != Namespaces.end()) {
		auto res = it->second.findSymbol(name);
		if (res) return res;
	}
	auto res = Namespaces["Global"].findSymbol(name);
	if (res) return res;

	return nullptr;
}

void VM::AddNamespace(const std::string& path, ankerl::unordered_dense::map<std::string, Namespace>& space)
{
	std::unique_lock lk(MergeMutex);
	for (auto& [name, s] : space) {
		for (auto& [oname, obj] : s.objects) {
			auto type = Manager.AddType(name + '.' + oname, obj);
			s.objectTypes[oname] = type;
			Units[path].objects.push_back({ name, oname });
		}
		s.objects.clear();
		for (auto& [fname, func] : s.functions) {
			auto full = name + '.' + fname;
			if (auto it = NameToFunctionMap.find(full); it != NameToFunctionMap.end()) {
				gWarn() << "Multiple definitions for function " << full << '\n';
			}
			NameToFunctionMap[full] = func;
			ValidFunctions.emplace(func);
			Units[path].functions.push_back({ name, fname });
		}
		for (auto& [vname, var] : s.variables) {
			Units[path].variables.push_back({ name, vname });
			GlobalVariables.emplace(name + '.' + vname, var);
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

			n.symbols.erase(function);
			auto ptr = n.functions[function.c_str()];
			ValidFunctions.erase(ptr);
			delete ptr;
			n.functions.erase(function.c_str());
			auto full = name + '.' + function;
			NameToFunctionMap.erase(full);
		}
		for (auto& [name, obj] : u.objects) {
			Manager.RemoveType(name + '.' + obj);
			Namespace& n = Namespaces[name];
			n.objects.erase(obj);
			n.objectTypes.erase(obj);
			n.symbols.erase(obj);
		}
		for (auto& [name, var] : u.variables) {
			Namespace& n = Namespaces[name];
			n.variables.erase(var);
			n.symbols.erase(var);
			GlobalVariables.erase(var);
		}
		
		Units.erase(unit);
	}
}

void VM::GarbageCollect()
{
	while (VMRunning) {

		String::GetAllocator()->Free();
		Array::GetAllocator()->Free();

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

Runner::Runner(VM* vm) : Owner(vm)
{
	Registers.reserve(64);
	CallStack.reserve(128);
}

Runner::~Runner()
{

}

#define TARGET(Op) Op: 

void Runner::operator()()
{
	while (Owner->IsRunning()) {

		{
			std::unique_lock lk(Owner->CallMutex);
			Owner->CallQueueNotify.wait(lk, [&]() {return !Owner->CallQueue.empty() || !Owner->IsRunning(); });
			if (!Owner->IsRunning()) return;

			CallStack.push_back(Owner->CallQueue.front());
			Owner->CallQueue.pop();
		}

		bool interrupt = true;
		CallObject* current = &CallStack.back();
		current->Ptr = current->FunctionPtr->Bytecode.data();
		current->End = current->Ptr + current->FunctionPtr->Bytecode.size();
		current->StackOffset = 0;

		Registers.reserve(current->FunctionPtr->RegisterCount);
		Registers.to(0);

		for (int i = 0; i < current->FunctionPtr->ArgCount && i < current->Arguments.size(); i++) {
			Registers[i] = current->Arguments[i];
		}
		current->Arguments.clear();

#define NUMS current->FunctionPtr->numberTable.values()
#define STRS current->FunctionPtr->stringTable
#define JUMPS current->FunctionPtr->jumpTable.values()
		out:
		while (interrupt && Owner->IsRunning()) {

		start: // @todo: maybe something better so we can exit whenever?
			const Instruction& byte = *(Instruction*)current->Ptr++;

#define X(x) case OpCodes::x: goto x;
			switch (byte.code)
			{
			default: __assume(0);
#include "Opcodes.h"
			}
#undef X
				TARGET(Noop) goto start;
				TARGET(Break) {
					interrupt = false;
				} goto out;

				TARGET(JumpForward) {
					current->Ptr += byte.param;
				} goto start;	

				TARGET(JumpBackward) {
					current->Ptr -= byte.param;
				} goto start;

				TARGET(Jump) {
					current->Ptr = &current->FunctionPtr->Bytecode.data()[JUMPS[byte.param]];
				} goto start;

				TARGET(LoadNumber) {
					Registers[byte.target] = NUMS[byte.param];
				} goto start;

				TARGET(LoadString) {
					Registers[byte.target] = STRS[byte.param];
				} goto start;

				TARGET(LoadSymbol) {
					auto& var = current->FunctionPtr->globalTable[byte.param];
					if (var == nullptr) {
						auto& name = current->FunctionPtr->globalTableSymbols[byte.param];
						if (auto idx = Owner->GlobalVariables.find(name); idx != Owner->GlobalVariables.end()) {
							var = &idx->second;
						}
						else {
							std::string varname = current->FunctionPtr->Namespace + "." + name;
							if (idx = Owner->GlobalVariables.find(varname); idx != Owner->GlobalVariables.end()) {
								var = &idx->second;
							}
							else {
								gWarn() << "Variable does not exist: " << varname << "\n";
								goto start;
							}	
						}
					}

					Registers[byte.target] = *var;

				} goto start;

				TARGET(StoreSymbol) {
					auto& var = current->FunctionPtr->globalTable[byte.param];
					if (var == nullptr) {
						auto& name = current->FunctionPtr->globalTableSymbols[byte.param];
						if (auto idx = Owner->GlobalVariables.find(name); idx != Owner->GlobalVariables.end()) {
							var = &idx->second;
						}
						else {
							std::string varname = current->FunctionPtr->Namespace + "." + name;
							if (idx = Owner->GlobalVariables.find(varname); idx != Owner->GlobalVariables.end()) {
								var = &idx->second;
							}
							else {
								gWarn() << "Variable does not exist: " << varname << "\n";
								goto start;
							}
						}
					}

					*var = Registers[byte.target];
				} goto start;

				TARGET(Return) {
					Variable val;
					if (byte.in1 == 1) {
						val = Registers[byte.target];
					}
					Registers.destroy(current->FunctionPtr->RegisterCount);
					if (CallStack.size() > 1) {
						CallStack.pop_back();
						current = &CallStack.back();
						Registers.to(current->StackOffset);
						const Instruction& oldByte = *(Instruction*)(current->Ptr - 1);
						Registers[oldByte.target] = val;
					}
					else {
						Owner->ReturnPromiseValues[current->PromiseIndex].set_value(val);
						CallStack.pop_back();
						Registers.to(0);
						current = nullptr;
						interrupt = false;
						goto out;
					}
				} goto start;

				TARGET(CallFunction) {

					auto& fn = current->FunctionPtr->functionTable[byte.in1];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->functionTableSymbols[byte.in1];
						if (auto idx = Owner->NameToFunctionMap.find(name); idx != Owner->NameToFunctionMap.end()) {
							fn = idx->second;
						}
						else {
							std::string fnname = current->FunctionPtr->Namespace + "." + name;
							if (idx = Owner->NameToFunctionMap.find(fnname); idx != Owner->NameToFunctionMap.end()) {
								fn = idx->second;
							}
							else {
								gWarn() << "Function does not exist: " << fnname << "\n";
								goto start;
							}
						}
					}
					//else if (Owner->ValidFunctions.find(fn) == Owner->ValidFunctions.end()) { // @todo: check is maybe required but very slow
					//	current->FunctionPtr->functionTable[byte.in1] = nullptr;
					//	gWarn() << "Cannot call deleted function\n";
					//	goto start;
					//}

					if (!fn->IsPublic && current->FunctionPtr->NamespaceHash != fn->NamespaceHash) {
						gWarn() << "Cannot call private function " << fn->Name << "\n";
						goto start;
					}

					for (int i = 1; i < fn->Types.size(); i++) {
						if ((fn->Types[i] != VariableType::Undefined 
							&& fn->Types[i] != Registers[byte.in2 + (i - 1)].getType())
							&& Registers[byte.in2 + (i - 1)].getType() != VariableType::Undefined) { // @todo: Once conversions exist remove this

							gWarn() << "Invalid argument types\n";
							goto start;
						}
					}
					
					auto offset = current->StackOffset + byte.in2;
					auto& call = CallStack.emplace_back(fn); // @todo: do this better, too slow
					call.StackOffset = offset;
					Registers.reserve(call.StackOffset + fn->RegisterCount);
					Registers.to(call.StackOffset);
					current = &call;

				} goto start;

				TARGET(CallSymbol) {

				} goto start;

				TARGET(CallInternal) {

				} goto start;

				TARGET(PushUndefined) {
					Registers[byte.target].setUndefined();
				} goto start;
				TARGET(PushBoolean) {
					Registers[byte.target] = byte.in1 == 1 ? true : false;
				} goto start;
				TARGET(PushTypeDefault) {
					Registers[byte.target] = GetTypeDefault((VariableType)byte.param);
				} goto start;
				TARGET(PushArray) {
					Registers[byte.target] = Array::GetAllocator()->Make(byte.param);
				} goto start;

				TARGET(Copy) {
					Registers[byte.target] = Registers[byte.in1];
				} goto start;
				TARGET(PushIndex) {
					Registers[byte.target].as<Array>()->data().push_back(Registers[byte.in1]);
				} goto start;
				
				TARGET(StoreIndex) {
					if (Registers[byte.in1].getType() == VariableType::Array) {
						auto& arr = Registers[byte.in1].as<Array>()->data();
						size_t idx = static_cast<size_t>(toNumber(Registers[byte.in2]));
						if (arr.size() < idx) {
							arr.resize(idx + 1);
						}
						arr[idx] = Registers[byte.target];
					}
					else {
						gWarn() << "Indexing target is not array\n";
					}
				} goto start;

				TARGET(LoadIndex) {
					if (Registers[byte.in1].getType() == VariableType::Array) {
						Registers[byte.target] = Registers[byte.in1].as<Array>()->data()[static_cast<size_t>(toNumber(Registers[byte.in2]))];
					}
					else {
						gWarn() << "Indexing target is not array\n";
					}
				} goto start;

				TARGET(PreMod) {
					if (byte.in2 == 0) {
						Registers[byte.target] = Registers[byte.target].as<double>() + 1.0;
					}
					else {
						Registers[byte.target] = Registers[byte.target].as<double>() - 1.0;
					}
				} goto start;
				
				TARGET(PostMod) {
					if (byte.in2 == 0) {
						Registers[byte.in1] = Registers[byte.in1].as<double>() + 1.0;
						Registers[byte.target] = Registers[byte.in1];
					}
					else {
						Registers[byte.in1] = Registers[byte.in1].as<double>() - 1.0;
						Registers[byte.target] = Registers[byte.in1];
					}
				} goto start;

				TARGET(NumAdd) {
					Registers[byte.target] = Registers[byte.in1].as<double>() + toNumber(Registers[byte.in2]);
				} goto start;

				TARGET(StrAdd) {
					stradd(Registers[byte.target], Registers[byte.in1], Registers[byte.in2]);
				} goto start;

				TARGET(NumSub) {
					Registers[byte.target] = Registers[byte.in1].as<double>() - toNumber(Registers[byte.in2]);
				} goto start;

				TARGET(NumDiv) {
					Registers[byte.target] = Registers[byte.in1].as<double>() / toNumber(Registers[byte.in2]);
				} goto start;

				TARGET(NumMul) {
					Registers[byte.target] = Registers[byte.in1].as<double>() * toNumber(Registers[byte.in2]);
				} goto start;

				// @todo: these could be optimized if the arguments are always the same type
				TARGET(Add) {
					 add(Registers[byte.target], Registers[byte.in1], Registers[byte.in2]);
				} goto start;

				TARGET(Sub) {
					sub(Registers[byte.target], Registers[byte.in1], Registers[byte.in2]);
				} goto start;

				TARGET(Div) {
					div(Registers[byte.target], Registers[byte.in1], Registers[byte.in2]);
				} goto start;

				TARGET(Mul) {
					mul(Registers[byte.target], Registers[byte.in1], Registers[byte.in2]);
				} goto start;
				
				TARGET(Equal) {
					Registers[byte.target] = equal(Registers[byte.in1], Registers[byte.in2]); // @todo: fix this
				} goto start;

				TARGET(NotEqual) {
					Registers[byte.target] = !equal(Registers[byte.in1], Registers[byte.in2]); // @todo: fix this
				} goto start;

				TARGET(Not) {
					Registers[byte.target] = !isTruthy(Registers[byte.in1]); // @todo: fix this
				} goto start;

				TARGET(JumpEq) {
					if (isTruthy(Registers[byte.target])) {
						current->Ptr += byte.param;
					}
				} goto start;

				TARGET(JumpNeg) {
					if (!isTruthy(Registers[byte.target])) {
						current->Ptr += byte.param;
					}
				} goto start;

				TARGET(Less) {
					Registers[byte.target] = toNumber(Registers[byte.in1]) < toNumber(Registers[byte.in2]);
				} goto start;

				TARGET(LessEqual) {
					Registers[byte.target] = toNumber(Registers[byte.in1]) <= toNumber(Registers[byte.in2]);
				} goto start;

				TARGET(Greater) {
					Registers[byte.target] = toNumber(Registers[byte.in1]) > toNumber(Registers[byte.in2]);
				} goto start;

				TARGET(GreaterEqual) {
					Registers[byte.target] = toNumber(Registers[byte.in1]) >= toNumber(Registers[byte.in2]);
				} goto start;

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
