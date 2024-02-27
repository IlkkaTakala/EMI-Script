#include "VM.h"
#include "Parser/Parser.h"
#include "NativeFuncs.h"
#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Objects/FunctionObject.h"

VM::VM()
{
	Parser::InitializeParser();
	auto counter = std::thread::hardware_concurrency();
	CompileRunning = true;

	for (auto& space : DefaultNamespaces) {
		auto [it, success] = Namespaces.emplace(space, Namespace{});
		auto sym = new Symbol();
		sym->setType(SymbolType::Namespace);
		sym->resolved = true;
		it->second.Sym = sym;
	}

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
	{
		std::lock_guard lock(CompileMutex);
		CompileQueue.push(fulloptions);
	}
	QueueNotify.notify_one();
}

void VM::Interrupt()
{

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
			std::string fullobjname;
			if (name != "Global") {
				fullobjname = name + '.' + oname;
			}
			else {
				fullobjname = oname;
			}
			auto type = GetManager().AddType(fullobjname, obj);
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
			GlobalVariables.erase(full);
		}
		for (auto& [name, obj] : u.objects) {
			GetManager().RemoveType(name + '.' + obj);
			Namespace& n = Namespaces[name];
			n.objects.erase(obj);
			n.objectTypes.erase(obj);
			n.symbols.erase(obj);
		}
		for (auto& [name, var] : u.variables) {
			Namespace& n = Namespaces[name];
			n.variables.erase(var);
			n.symbols.erase(var);
			GlobalVariables.erase(name + '.' + var);
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

		bool hasReturn = false;
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

#define NUMS current->FunctionPtr->NumberTable.values()
#define STRS current->FunctionPtr->StringTable
#define JUMPS current->FunctionPtr->JumpTable.values()
		out:
		while (interrupt && Owner->IsRunning()) {

		start: // @todo: maybe something better so we can exit whenever?
			if (!Owner->IsRunning()) goto out;
			const Instruction& byte = *(Instruction*)current->Ptr++;

#define X(x) case OpCodes::x: goto x;
			switch (byte.code)
			{
#include "Opcodes.h"
			default: __assume(0);
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
					current->Ptr = &current->FunctionPtr->Bytecode.data()[byte.param];
				} goto start;

				TARGET(RangeFor) {

					auto& index = Registers[byte.in1];
					auto& cmp = Registers[byte.in2];

					if (index.getType() != VariableType::Number) {
						gError() << "Index type does not match the expression\n";
						goto start;
					}
					index = index.as<double>() + 1.0;

					bool result = false;
					switch (cmp.getType())
					{
					case VariableType::Number: {
						result = index.as<double>() < cmp.as<double>();
					} break;

					case VariableType::Array: {
						result = index.as<double>() < cmp.as<Array>()->size();
					} break;

					default:
						break;
					}

					Registers[byte.target] = result;
				} goto start;

				TARGET(RangeForVar) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					auto& var = Registers[data.in1];
					auto& index = Registers[byte.in1];
					auto& cmp = Registers[byte.in2];

					if (index.getType() != VariableType::Number) {
						gError() << "Index type does not match the expression\n";
						goto start;
					}
					index = index.as<double>() + 1.0;

					bool result = false;
					switch (cmp.getType())
					{
					case VariableType::Number: {
						result = index.as<double>() < cmp.as<double>();
						if (result) {
							var = index;
						}
					} break;

					case VariableType::Array: {
						result = index.as<double>() < cmp.as<Array>()->size();
						if (result) {
							var = cmp.as<Array>()->data()[static_cast<size_t>(index.as<double>())];
						}
					} break;

					default:
						break;
					}

					Registers[byte.target] = result;
				} goto start;

				TARGET(LoadNumber) {
					Registers[byte.target] = NUMS[byte.param];
				} goto start;

				TARGET(LoadImmediate) {
					Registers[byte.target] = (int8)byte.param;
				} goto start;

				TARGET(LoadString) {
					Registers[byte.target] = STRS[byte.param];
				} goto start;

				TARGET(LoadSymbol) {
					auto& var = current->FunctionPtr->GlobalTable[byte.param];
					if (var == nullptr) {
						auto& name = current->FunctionPtr->GlobalTableSymbols[byte.param];
						if (auto idx = Owner->GlobalVariables.find(name); idx != Owner->GlobalVariables.end()) {
							var = &idx->second;
						}
						else if (auto intrfunc = IntrinsicFunctions.find(name); intrfunc != IntrinsicFunctions.end()) {
							auto func = FunctionObject::GetAllocator()->Make(FunctionType::Intrinsic, name);
							auto& v = Owner->GlobalVariables[name] = func;
							func->Callee = intrfunc->second;
							var = &v;
						}
						else if (auto hostfunc = HostFunctions().find(name); hostfunc != HostFunctions().end()) {
							auto func = FunctionObject::GetAllocator()->Make(FunctionType::Host, name);
							auto& v = Owner->GlobalVariables[name] = func;
							func->Callee = hostfunc->second;
							var = &v;
						}
						else if (auto userfunc = Owner->NameToFunctionMap.find(name); userfunc != Owner->NameToFunctionMap.end()) {
							auto func = FunctionObject::GetAllocator()->Make(FunctionType::User, name);
							auto& v = Owner->GlobalVariables[name] = func;
							func->Callee = userfunc->second;
							var = &v;
						}
						else {
							std::string varname = current->FunctionPtr->Namespace + "." + name;
							if (idx = Owner->GlobalVariables.find(varname); idx != Owner->GlobalVariables.end()) {
								var = &idx->second;
							}
							else if (auto localintrfunc = IntrinsicFunctions.find(varname); localintrfunc != IntrinsicFunctions.end()) {
								auto func = FunctionObject::GetAllocator()->Make(FunctionType::Intrinsic, varname);
								auto& v = Owner->GlobalVariables[varname] = func;
								func->Callee = localintrfunc->second;
								var = &v;
							}
							else if (auto localhostfunc = HostFunctions().find(varname); localhostfunc != HostFunctions().end()) {
								auto func = FunctionObject::GetAllocator()->Make(FunctionType::Host, varname);
								auto& v = Owner->GlobalVariables[varname] = func;
								func->Callee = localhostfunc->second;
								var = &v;
							}
							else if (auto localuserfunc = Owner->NameToFunctionMap.find(varname); localuserfunc != Owner->NameToFunctionMap.end()) {
								auto func = FunctionObject::GetAllocator()->Make(FunctionType::User, varname);
								auto& v = Owner->GlobalVariables[varname] = func;
								func->Callee = localuserfunc->second;
								var = &v;
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
					auto& var = current->FunctionPtr->GlobalTable[byte.param];
					if (var == nullptr) {
						auto& name = current->FunctionPtr->GlobalTableSymbols[byte.param];
						if (auto idx = Owner->GlobalVariables.find(name); idx != Owner->GlobalVariables.end()) {
							var = &idx->second;
						}
						else {
							std::string varname = current->FunctionPtr->Namespace + "." + name;
							if (idx = Owner->GlobalVariables.find(varname); idx != Owner->GlobalVariables.end()) {
								var = &idx->second;
							}
							else {
								gWarn() << "Symbol does not exist: " << varname << "\n";
								goto start;
							}
						}
					}

					*var = Registers[byte.target];
				} goto start;

				TARGET(LoadProperty) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					Variable& prop = Registers[byte.in1];
					int32& propertyIdx = current->FunctionPtr->PropertyTable[data.param];
					if (propertyIdx == -1) {
						auto& name = current->FunctionPtr->PropertyTableSymbols[data.param];

						uint16 idx;
						if (!GetManager().GetPropertyIndex(idx, name, prop.getType())) {
							propertyIdx = -1;
							gError() << "Property not found: " << name << ", in function " << current->FunctionPtr->Name << '\n';
							Registers[byte.target].setUndefined();
							goto start;
						}
						propertyIdx = idx;
					}

					if (prop.getType() > VariableType::Object) {
						UserObject* ptr = prop.as<UserObject>();
						if (ptr->size() <= propertyIdx) {
							gError() << "Invalid property " << current->FunctionPtr->PropertyTableSymbols[data.param] << '\n';
							Registers[byte.target].setUndefined();
							goto start;
						}
						Registers[byte.target] = (*ptr)[static_cast<uint16>(propertyIdx)];
					}
				} goto start;

				TARGET(StoreProperty) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					Variable& prop = Registers[byte.in1];
					int32& propertyIdx = current->FunctionPtr->PropertyTable[data.param];
					if (propertyIdx == -1) {
						auto& name = current->FunctionPtr->PropertyTableSymbols[data.param];

						uint16 idx;
						if (!GetManager().GetPropertyIndex(idx, name, prop.getType())) {
							propertyIdx = -1;
							gError() << "Property not found: " << name << ", in function " << current->FunctionPtr->Name << '\n';
							goto start;
						}
						propertyIdx = idx;
					}

					if (prop.getType() > VariableType::Object) {
						UserObject* ptr = prop.as<UserObject>();
						if (ptr->size() <= propertyIdx) {
							gError() << "Invalid property " << current->FunctionPtr->PropertyTableSymbols[data.param] << '\n';
							goto start;
						}
						(*ptr)[static_cast<uint16>(propertyIdx)] = Registers[byte.target];
					}
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
						const Instruction& oldByte = *(Instruction*)(current->Ptr - 2);
						Registers[oldByte.target] = val;
					}
					else {
						Owner->ReturnPromiseValues[current->PromiseIndex].set_value(val);
						CallStack.pop_back();
						Registers.to(0);
						current = nullptr;
						interrupt = false;
						hasReturn = true;
						goto end;
					}
				} goto start;

				TARGET(CallFunction) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					auto& fn = current->FunctionPtr->FunctionTable[data.data];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->FunctionTableSymbols[data.data];
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
					//	current->FunctionPtr->FunctionTable[data.data] = nullptr;
					//	gWarn() << "Cannot call deleted function\n";
					//	goto start;
					//}

					if (!fn->IsPublic && current->FunctionPtr->NamespaceHash != fn->NamespaceHash) {
						gWarn() << "Cannot call private function " << fn->Name << "\n";
						goto start;
					}

					for (int i = 1; i < fn->Types.size() && i < byte.in2; i++) {
						if (fn->Types[i] != VariableType::Undefined 
							&& Registers[byte.in1 + (i - 1)].getType() != VariableType::Undefined) { // @todo: Once conversions exist remove this
							VariableType real = fn->Types[i];

							if (real >= VariableType::Object) {
								size_t typeidx = static_cast<uint16>(real) - static_cast<uint16>(VariableType::Object);
								if (typeidx < fn->TypeTable.size()) {
									real = fn->TypeTable[typeidx];
									if (real == VariableType::Undefined) {
										auto& name = fn->TypeTableSymbols[typeidx];
										UserDefinedType* usertype = nullptr;
										if (GetManager().GetType(usertype, name)) {
											fn->TypeTable[typeidx] = real = usertype->Type;
										}
									}
								}
								else {
									gError() << "Invalid type while calling " << fn->Name << "\n";
								}
							}

							if (real != Registers[byte.in1 + (i - 1)].getType()) {
								gWarn() << "Invalid argument types when calling " << fn->Name << " from " << current->FunctionPtr->Name << "\n";
								goto start;
							}
						}
					}
					
					auto offset = current->StackOffset + byte.in1;
					auto& call = CallStack.emplace_back(fn); // @todo: do this better, too slow
					call.StackOffset = offset;
					Registers.reserve(call.StackOffset + fn->RegisterCount);
					Registers.to(call.StackOffset);
					current = &call;

				} goto start;

				TARGET(CallSymbol) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					if (Registers[data.target].getType() != VariableType::Function) {
						gWarn() << "Variable does not contain a function\n";
						goto start;
					}

					FunctionObject* f = Registers[data.target].as<FunctionObject>();

					switch (f->InternalType)
					{
					case FunctionType::User: {
						if (auto ptr = std::get<Function*>(f->Callee)) { // @todo: Add finding function
							if (!ptr->IsPublic && current->FunctionPtr->NamespaceHash != ptr->NamespaceHash) {
								gWarn() << "Cannot call private function " << ptr->Name << "\n";
								goto start;
							}

							for (int i = 1; i < ptr->Types.size() && i < byte.in2; i++) {
								if ((ptr->Types[i] != VariableType::Undefined
									&& ptr->Types[i] != Registers[byte.in1 + (i - 1)].getType())
									&& Registers[byte.in1 + (i - 1)].getType() != VariableType::Undefined) { // @todo: Once conversions exist remove this

									gWarn() << "Invalid argument types when calling " << ptr->Name << " from " << current->FunctionPtr->Name << "\n";
									goto start;
								}
							}

							auto offset = current->StackOffset + byte.in1;
							auto& call = CallStack.emplace_back(ptr); // @todo: do this better, too slow
							call.StackOffset = offset;
							Registers.reserve(call.StackOffset + ptr->RegisterCount);
							Registers.to(call.StackOffset);
							current = &call;
						}
					} break;

					case FunctionType::Host: {
						if (auto ptr = std::get<EMI::__internal_function*>(f->Callee)) {

							thread_local static std::vector<InternalValue> args;
							args.resize(byte.in2);
							for (int i = 0; i < byte.in2; ++i) {
								args[i] = makeHostArg(Registers[byte.in1 + i]);
							}
							InternalValue ret = (*ptr)(byte.in2, args.data());
							Registers[byte.target] = moveOwnershipToVM(ret);
						}
					} break;

					case FunctionType::Intrinsic: {
						if (auto ptr = std::get<IntrinsicPtr>(f->Callee)) {
							ptr(Registers[byte.target], &Registers[byte.in1], byte.in2);
						}
					} break;

					case FunctionType::None: {
						goto start;
					} break;

					default: __assume(0);
					}
				} goto start;

				TARGET(CallInternal) {
					const Instruction& data = *(Instruction*)current->Ptr++;
					auto& fn = current->FunctionPtr->IntrinsicTable[data.data];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->FunctionTableSymbols[data.data];
						if (auto idx = IntrinsicFunctions.find(name); idx != IntrinsicFunctions.end()) {
							fn = idx->second;
						}
						else {
							std::string fnname = current->FunctionPtr->Namespace + "." + name;
							if (idx = IntrinsicFunctions.find(fnname); idx != IntrinsicFunctions.end()) {
								fn = idx->second;
							}
							else {
								gWarn() << "Function does not exist: " << fnname << "\n";
								goto start;
								break;
							}
						}
					}

					fn(Registers[byte.target], &Registers[byte.in1], byte.in2);

				} goto start;

				TARGET(CallExternal) {
					const Instruction& data = *(Instruction*)current->Ptr++;
					auto& fn = current->FunctionPtr->ExternalTable[data.data];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->FunctionTableSymbols[data.data];
						if (auto idx = HostFunctions().find(name); idx != HostFunctions().end()) {
							fn = idx->second;
						}
						else {
							std::string fnname = current->FunctionPtr->Namespace + "." + name;
							if (idx = HostFunctions().find(fnname); idx != HostFunctions().end()) {
								fn = idx->second;
							}
							else {
								gWarn() << "Function does not exist: " << fnname << "\n";
								goto start;
								break;
							}
						}
					}

					thread_local static std::vector<InternalValue> args;
					args.resize(byte.in2);
					for (int i = 0; i < byte.in2; ++i) {
						args[i] = makeHostArg(Registers[byte.in1 + i]);
					}
					InternalValue ret = (*fn)(byte.in2, args.data());
					Registers[byte.target] = moveOwnershipToVM(ret);

				} goto start;

				TARGET(Call) {
					gError() << "Unknown functions not implemented\n";
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
				TARGET(PushObjectDefault) {
					auto& type = current->FunctionPtr->TypeTable[byte.param];
					if (type == VariableType::Undefined) {
						auto& name = current->FunctionPtr->TypeTableSymbols[byte.param];
						UserDefinedType* usertype = nullptr;
						if (GetManager().GetType(usertype, name)) {
							type = usertype->Type;
						}
						else {
							gError() << "Type not defined: " << name << "\n";
							goto start;
						}
					}

					Registers[byte.target] = GetManager().Make(type);
				} goto start;

				TARGET(InitObject) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					Registers[byte.in1];
					Registers[byte.in2];

					auto& type = current->FunctionPtr->TypeTable[data.param];
					if (type == VariableType::Undefined) {
						auto& name = current->FunctionPtr->TypeTableSymbols[data.param];
						UserDefinedType* usertype = nullptr;
						if (GetManager().GetType(usertype, name)) {
							type = usertype->Type;
						}
						else {
							gError() << "Type not defined: " << name << "\n";
							goto start;
						}
					}
					auto obj = GetManager().Make(type);

					for (uint16 i = 0; i < byte.in2 && i < obj.as<UserObject>()->size(); ++i) {
						(*obj.as<UserObject>())[i] = Registers[byte.in1 + i];
					}

					Registers[byte.target] = obj;

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
						if (arr.size() <= idx) {
							gError() << "Array out of bounds: Size " << arr.size() << ", tried to access index " << idx << "\n";
							goto start;
						}
						arr[idx] = Registers[byte.target];
					}
					else {
						gWarn() << "Indexing target is not array\n";
					}
				} goto start;

				TARGET(LoadIndex) {
					if (Registers[byte.in1].getType() == VariableType::Array) {
						size_t idx = static_cast<size_t>(toNumber(Registers[byte.in2]));
						Array* arr = Registers[byte.in1].as<Array>();
						if (idx < arr->size()) {
							Registers[byte.target] = arr->data()[idx];
						}
						else {
							gError() << "Array out of bounds: Size " << arr->size() << ", tried to access index " << idx << "\n";
							goto start;
						}
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
						Registers[byte.target] = Registers[byte.in1];
						Registers[byte.in1] = Registers[byte.in1].as<double>() + 1.0;
					}
					else {
						Registers[byte.target] = Registers[byte.in1];
						Registers[byte.in1] = Registers[byte.in1].as<double>() - 1.0;
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
				
				TARGET(And) {
					Registers[byte.target] = isTruthy(Registers[byte.in1]) && isTruthy(Registers[byte.in2]);
				} goto start;
				
				TARGET(Or) {
					Registers[byte.target] = isTruthy(Registers[byte.in1]) || isTruthy(Registers[byte.in2]);
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
	end:
		if (hasReturn) {

		}

	}
}

CallObject::CallObject(Function* function)
{
	PromiseIndex = 0;
	FunctionPtr = function;
	StackOffset = 0;
	Ptr = function->Bytecode.data();
	End = function->Bytecode.data() + function->Bytecode.size();
}
