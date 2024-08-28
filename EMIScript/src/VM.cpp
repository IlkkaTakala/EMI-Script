#include "VM.h"
#include "Parser/Parser.h"
#include "TypeConverters.h"
#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Objects/FunctionObject.h"
#include <math.h>
#include <filesystem>
#include <fstream>
#include "EMLibFormat.h"

VM::VM()
{
	Parser::InitializeParser();
	auto counter = std::thread::hardware_concurrency();
	CompileRunning = true;

	GlobalSymbols.Table.insert(IntrinsicFunctions.Table.begin(), IntrinsicFunctions.Table.end());
	GlobalSymbols.Table.insert(HostFunctions().Table.begin(), HostFunctions().Table.end());

	VMRunning = true;
	for (uint32_t i = 0; i < counter / 2; i++) {
		ParserPool.emplace_back(Parser::ThreadedParse, this);
		RunnerPool.emplace_back(new Runner(this));
	}

	GarbageCollector = new std::thread(&VM::GarbageCollect, this);
}

VM::~VM()
{
	while (!Units.empty()) {
		RemoveUnit(Units.begin()->first);
	}

	GlobalSymbols.Table.clear();

	CompileRunning = false;
	VMRunning = false;
	QueueNotify.notify_all();
	CallQueueNotify.notify_all();
	for (auto& t : ParserPool) {
		t.join();
	}
	for (auto& t : RunnerPool) {
		t->SetRunning(false);
	}
	CallQueueNotify.notify_all();
	for (auto& t : RunnerPool) {
		t->Join();
		delete t;
	}
	GarbageCollector->join();
	delete GarbageCollector;
	Parser::ReleaseParser();
}

void VM::ReinitializeGrammar(const char* grammar)
{
	Parser::InitializeGrammar(grammar);
}

void* VM::Compile(const char* path, const Options& options)
{
	void* handle = nullptr;

	CompileOptions fulloptions{};
	fulloptions.Path = path;
	fulloptions.UserOptions = options;

	{
		std::lock_guard lock(CompileMutex);
		auto& future = CompileRequests.emplace_back(fulloptions.CompileResult.get_future());
		handle = &future;
		CompileQueue.push(std::move(fulloptions));
	}
	QueueNotify.notify_one();

	return handle;
}

void VM::CompileTemporary(const char* data)
{
	CompileOptions fulloptions{};
	fulloptions.Data = data;
	{
		std::lock_guard lock(CompileMutex);
		CompileQueue.push(std::move(fulloptions));
	}
	QueueNotify.notify_one();
}

void VM::Interrupt()
{

}

bool VM::Export(const char* path, const ExportOptions& options)
{
	ExportOptions ex = options;
	std::filesystem::path fullpath(path);
	if (!std::filesystem::is_directory(fullpath)) {
		ex.CombineUnits = true;
	}

	if (ex.CombineUnits) {
		std::ofstream file(fullpath, std::ios::out | std::ios::binary);
		if (!file.is_open()) {
			gError() << "Cannot open file for writing: " << fullpath;
			return false;
		}

		if (!Library::Encode(GlobalSymbols, file)) {
			gError() << "Writing library file failed";
			return false;
		}
	}
	else {
		for (auto& [name, unit] : Units) {
			auto fp = fullpath;
			fp += std::filesystem::path(name).filename().replace_extension();
			std::ofstream file(fp, std::ios::out | std::ios::binary);

			if (!file.is_open()) {
				gError() << "Cannot open file for writing: " << fullpath;
				return false;
			}

			SymbolTable table;
			for (auto& id : unit.Symbols) {
				table.AddName(id, GlobalSymbols.Table[id]);
			}

			if (!Library::Encode(table, file)) {
				gError() << "Writing library file failed";
				return false;
			}
		}
	}

	return true;
}

void* VM::GetFunctionID(const std::string& id)
{
	TName name(id.c_str());
	auto [fullName, symbol] = GlobalSymbols.FindName(name);
	if (symbol && symbol->Type == SymbolType::Function) {
		auto fn = static_cast<FunctionSymbol*>(symbol->Data);
		if (fn->Type != FunctionType::User) {
			gWarn() << "Symbol is not a script function";
			return 0;
		}
		if (!static_cast<Function*>(fn->DirectPtr)->IsPublic) {
			gWarn() << "Function is private";
			return 0;
		}

		return fn->DirectPtr;
	}
	return 0;
}

size_t VM::CallFunction(FunctionHandle handle, const std::span<InternalValue>& args)
{
	Function* fn = (Function*)(void*)handle;
	if (!fn) {
		gWarn() << "Invalid function handle";
		return (size_t)-1;
	}

	CallObject& call = CallQueue.emplace(fn);

	call.Arguments.reserve(args.size());
	for (size_t i = 0; i < call.FunctionPtr->ArgCount && i < args.size(); i++) {
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

bool VM::WaitForResult(void* ptr)
{
	std::unique_lock lk(CompileMutex);
	auto it = std::find_if(CompileRequests.begin(), CompileRequests.end(), [ptr](const std::future<bool>& item) { return ptr == &item; });
	if (it != CompileRequests.end()) {
		lk.unlock();
		auto res = it->get();
		std::unique_lock outlk(CompileMutex);
		CompileRequests.remove_if([ptr](const std::future<bool>& item) { return ptr == &item; });
		return res;
	}
	return false;
}

std::pair<TName, Symbol*> VM::FindSymbol(const TNameQuery& name)
{
	return GlobalSymbols.FindName(name);
}

void VM::AddNamespace(const std::string& path, const SymbolTable& space)
{
	std::unique_lock lk(MergeMutex);
	Units[path].Symbols.reserve(space.Table.size());
	for (auto& [name, s] : space.Table) {
		Units[path].Symbols.push_back(name);
		if (!s) continue;

		switch (s->Type)
		{
		case SymbolType::Object: {
			auto type = GetManager().AddType(name, *static_cast<UserDefinedType*>(s->Data));
			s->VarType = type;
			static_cast<UserDefinedType*>(s->Data)->Type = type;
		} break;

		default:
			break;
		}
	}
	GlobalSymbols.Table.insert(space.Table.begin(), space.Table.end());
}

void VM::RemoveUnit(const std::string& unit)
{
	std::unique_lock lk(MergeMutex);
	if (auto it = Units.find(unit); it != Units.end()) {
		auto& u = it->second;

		for (auto& name : u.Symbols) {
			Symbol* node = GlobalSymbols.Table[name];
			if (!node) continue;
			switch (node->Type)
			{
			case SymbolType::Object: {
				GetManager().RemoveType(name);
			} break;
			default:
				break;
			}
			delete node;
			GlobalSymbols.Table.erase(name);
		}

		Units.erase(unit);
	}
}

void VM::GarbageCollect()
{
	while (VMRunning) {

		String::GetAllocator()->Free();
		Array::GetAllocator()->Free();
		FunctionObject::GetAllocator()->Free();
		UserObject::GetAllocator()->Free();

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

Runner::Runner(VM* vm) : Owner(vm)
{
	Registers.reserve(64);
	CallStack.reserve(128);

	RunThread = std::thread{ &Runner::Run, this };
}

Runner::~Runner()
{

}

void Runner::Join()
{
	RunThread.join();
}

#define TARGET(Op) Op: 
#define Error() gError() << current->FunctionPtr->Name << " (" << current->FunctionPtr->DebugLines[int(current->Ptr - current->FunctionPtr->Bytecode.data())] << "):  "
#define Warn() gWarn() << current->FunctionPtr->Name << " (" << current->FunctionPtr->DebugLines[int(current->Ptr - current->FunctionPtr->Bytecode.data())] << "):  "

void Runner::Run()
{
	Running = true;
	while (Running) {

		{
			std::unique_lock lk(Owner->CallMutex);
			Owner->CallQueueNotify.wait(lk, [&]() {return !Owner->CallQueue.empty() || !Running; });
			if (!Running) return;

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

		for (size_t i = 0; i < current->FunctionPtr->ArgCount && i < current->Arguments.size(); i++) {
			Registers[i] = current->Arguments[i];
		}
		current->Arguments.clear();

#define NUMS current->FunctionPtr->NumberTable.values()
#define STRS current->FunctionPtr->StringTable
#define JUMPS current->FunctionPtr->JumpTable.values()
		out:
		while (interrupt && Running) {

		start: // @todo: maybe something better so we can exit whenever?
			if (!Running) goto out;
			const Instruction& byte = *(Instruction*)current->Ptr++;

#define X(x) case OpCodes::x: goto x;
			switch (byte.code)
			{
#include "Opcodes.h"
			default: 
			#ifdef _MSC_VER
			__assume(0);
			#endif
			break;
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
						Error() << "Index type does not match the expression";
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
						Error() << "Index type does not match the expression";
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
					Registers[byte.target] = (int8_t)byte.param;
				} goto start;

				TARGET(LoadString) {
					Registers[byte.target] = STRS[byte.param];
				} goto start;

				TARGET(LoadSymbol) {
					auto& var = current->FunctionPtr->GlobalTable[byte.param];
					if (var == nullptr) {
						auto& name = current->FunctionPtr->GlobalTableSymbols[byte.param];
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.second) {
							if (res.second->Type == SymbolType::Variable || res.second->Type == SymbolType::Static) {
								var = static_cast<Variable*>(res.second->Data);
							}
							else if (res.second->Type == SymbolType::Function) {
								auto f = static_cast<FunctionSymbol*>(res.second->Data);
								auto func = FunctionObject::GetAllocator()->Make(f->Type, res.first);
								switch (f->Type) {
								case FunctionType::Intrinsic: func->Callee = reinterpret_cast<IntrinsicPtr>(f->DirectPtr); break;
								case FunctionType::User: func->Callee = static_cast<Function*>(f->DirectPtr); break;
								case FunctionType::Host: func->Callee = static_cast<EMI::_internal_function*>(f->DirectPtr); break;
								default: break;
								}
								f->Function = func;
								var = &f->Function;
							}
						}
						else {
							Warn() << "Variable does not exist: " << name;
							goto start;
						}	
					}

					if (var)
					Registers[byte.target] = *var;

				} goto start;

				TARGET(StoreSymbol) {
					auto& var = current->FunctionPtr->GlobalTable[byte.param];
					if (var == nullptr) {
						auto& name = current->FunctionPtr->GlobalTableSymbols[byte.param];
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.second && res.second->Type == SymbolType::Variable) {
							var = static_cast<Variable*>(res.second->Data);
						}
						else {
							Warn() << "Symbol does not exist: " << name;
							goto start;
						}
					}

					*var = Registers[byte.target];
				} goto start;

				TARGET(LoadProperty) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					Variable& prop = Registers[byte.in1];
					int32_t& propertyIdx = current->FunctionPtr->PropertyTable[data.param];
					if (propertyIdx == -1) {
						auto& name = current->FunctionPtr->PropertyTableSymbols[data.param];

						uint16_t idx;
						if (!GetManager().GetPropertyIndex(idx, name, prop.getType())) {
							propertyIdx = -1;
							Error() << "Property not found: " << name;
							Registers[byte.target].setUndefined();
							goto start;
						}
						propertyIdx = idx;
					}

					if (prop.getType() > VariableType::Object) {
						UserObject* ptr = prop.as<UserObject>();
						if (ptr->size() <= propertyIdx) {
							Error() << "Invalid property " << current->FunctionPtr->PropertyTableSymbols[data.param];
							Registers[byte.target].setUndefined();
							goto start;
						}
						Registers[byte.target] = (*ptr)[static_cast<uint16_t>(propertyIdx)];
					}
				} goto start;

				TARGET(StoreProperty) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					Variable& prop = Registers[byte.in1];
					int32_t& propertyIdx = current->FunctionPtr->PropertyTable[data.param];
					if (propertyIdx == -1) {
						auto& name = current->FunctionPtr->PropertyTableSymbols[data.param];

						uint16_t idx;
						if (!GetManager().GetPropertyIndex(idx, name, prop.getType())) {
							propertyIdx = -1;
							Error() << "Property not found: " << name;
							goto start;
						}
						propertyIdx = idx;
					}

					if (prop.getType() > VariableType::Object) {
						UserObject* ptr = prop.as<UserObject>();
						if (ptr->size() <= propertyIdx) {
							Error() << "Invalid property " << current->FunctionPtr->PropertyTableSymbols[data.param];
							goto start;
						}
						(*ptr)[static_cast<uint16_t>(propertyIdx)] = Registers[byte.target];
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
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.first && res.second->Type == SymbolType::Function) {
							auto f = static_cast<FunctionSymbol*>(res.second->Data);
							if (f->Type == FunctionType::User) {
								fn = static_cast<Function*>(f->DirectPtr);
							}
							else {
								Warn() << "Function has unexpected location: " << name;
								goto start;
							}
						}
						else {
							Warn() << "Function does not exist: " << name;
							goto start;
						}
					}

					if (!fn->IsPublic && fn->Name.IsChildOf(current->FunctionPtr->Name.Get(1))) {
						Warn() << "Cannot call private function " << fn->Name;
						goto start;
					}

					for (size_t i = 1; i < fn->Types.size() && i < byte.in2; i++) {
						if (fn->Types[i] != VariableType::Undefined 
							&& Registers[byte.in1 + (i - 1)].getType() != VariableType::Undefined) { // @todo: Once conversions exist remove this
							VariableType real = fn->Types[i];

							if (real >= VariableType::Object) {
								size_t typeidx = static_cast<uint16_t>(real) - static_cast<uint16_t>(VariableType::Object);
								if (typeidx < fn->TypeTable.size()) {
									real = fn->TypeTable[typeidx];
									if (real == VariableType::Undefined) {
										auto& name = fn->TypeTableSymbols[typeidx];
										UserDefinedType* usertype = nullptr;
										auto res = Owner->GlobalSymbols.FindName(name);
										if (res.second && res.second->Type == SymbolType::Object) {
											usertype = static_cast<UserDefinedType*>(res.second->Data);
											fn->TypeTable[typeidx] = real = usertype->Type;
										}
									}
								}
								else {
									Error() << "Invalid type while calling " << fn->Name;
								}
							}

							if (real != Registers[byte.in1 + (i - 1)].getType()) {
								Warn() << "Invalid argument types when calling " << fn->Name;
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
						Warn() << "Variable does not contain a function";
						goto start;
					}

					FunctionObject* f = Registers[data.target].as<FunctionObject>();

					switch (f->InternalType)
					{
					case FunctionType::User: {
						if (auto ptr = std::get<Function*>(f->Callee)) { // @todo: Add finding function
							if (!ptr->IsPublic && ptr->Name.IsChildOf(current->FunctionPtr->Name.Get(1))) {
								Warn() << "Cannot call private function " << ptr->Name;
								goto start;
							}


							for (size_t i = 1; i < ptr->Types.size() && i < byte.in2; i++) {
								if (ptr->Types[i] != VariableType::Undefined
									&& Registers[byte.in1 + (i - 1)].getType() != VariableType::Undefined) { // @todo: Once conversions exist remove this
									VariableType real = ptr->Types[i];

									if (real >= VariableType::Object) {
										size_t typeidx = static_cast<uint16_t>(real) - static_cast<uint16_t>(VariableType::Object);
										if (typeidx < ptr->TypeTable.size()) {
											real = ptr->TypeTable[typeidx];
											if (real == VariableType::Undefined) {
												auto& name = ptr->TypeTableSymbols[typeidx];
												auto res = Owner->GlobalSymbols.FindName(name);
												if (res.second && res.second->Type == SymbolType::Object) {
													UserDefinedType* usertype = static_cast<UserDefinedType*>(res.second->Data);
													ptr->TypeTable[typeidx] = real = usertype->Type;
												}
											}
										}
										else {
											Error() << "Invalid type while calling " << ptr->Name;
										}
									}

									if (real != Registers[byte.in1 + (i - 1)].getType()) {
										Warn() << "Invalid argument types when calling " << ptr->Name;
										goto start;
									}
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
						if (auto ptr = std::get<EMI::_internal_function*>(f->Callee)) {

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

					default: 
					#ifdef _MSC_VER
					__assume(0);
					#endif
					break;
					}
				} goto start;

				TARGET(CallInternal) {
					const Instruction& data = *(Instruction*)current->Ptr++;
					auto& fn = current->FunctionPtr->IntrinsicTable[data.data];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->FunctionTableSymbols[data.data];
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.second && res.second->Type == SymbolType::Function) {
							auto f = static_cast<FunctionSymbol*>(res.second->Data);
							if (f->Type == FunctionType::Intrinsic) {
								fn = reinterpret_cast<IntrinsicPtr>(f->DirectPtr);
							}
						}
						else {
							Warn() << "Function does not exist: " << name;
							goto start;
							break;
						}
					}

					fn(Registers[byte.target], &Registers[byte.in1], byte.in2);

				} goto start;

				TARGET(CallExternal) {
					const Instruction& data = *(Instruction*)current->Ptr++;
					auto& fn = current->FunctionPtr->ExternalTable[data.data];
					if (fn == nullptr) {
						auto& name = current->FunctionPtr->FunctionTableSymbols[data.data];
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.second && res.second->Type == SymbolType::Function) {
							auto f = static_cast<FunctionSymbol*>(res.second->Data);
							if (f->Type == FunctionType::Host) {
								fn = static_cast<EMI::_internal_function*>(f->DirectPtr);
							}
						}
						else {
							Warn() << "Function does not exist: " << name;
							goto start;
							break;
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
					Error() << "Unknown functions not implemented";
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
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.second && res.second->Type == SymbolType::Object) {
							UserDefinedType* usertype = static_cast<UserDefinedType*>(res.second->Data);
							type = usertype->Type;
						}
						else {
							Error() << "Type not defined: " << name;
							goto start;
						}
					}

					Registers[byte.target] = GetManager().Make(type);
				} goto start;

				TARGET(InitObject) {
					const Instruction& data = *(Instruction*)current->Ptr++;

					auto& type = current->FunctionPtr->TypeTable[data.param];
					if (type == VariableType::Undefined) {
						auto& name = current->FunctionPtr->TypeTableSymbols[data.param];
						auto res = Owner->GlobalSymbols.FindName(name);
						if (res.second && res.second->Type == SymbolType::Object) {
							UserDefinedType* usertype = static_cast<UserDefinedType*>(res.second->Data);
							type = usertype->Type;
						}
						else {
							Error() << "Type not defined: " << name;
							goto start;
						}
					}
					auto obj = GetManager().Make(type);

					for (uint16_t i = 0; i < byte.in2 && i < obj.as<UserObject>()->size(); ++i) {
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
							Error() << "Array out of bounds: Size " << arr.size() << ", tried to access index " << idx;
							goto start;
						}
						arr[idx] = Registers[byte.target];
					}
					else {
						Warn() << "Indexing target is not array";
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
							Error() << "Array out of bounds: Size " << arr->size() << ", tried to access index " << idx;
							goto start;
						}
					}
					else {
						Warn() << "Indexing target is not array";
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
					auto val = Registers[byte.in1].as<double>() / toNumber(Registers[byte.in2]);
					if (!isnan(val)) {
						Registers[byte.target] = val;
					}
					else {
						Registers[byte.target].setUndefined();
					}
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
					auto& lhs = Registers[byte.in1];
					auto& rhs = Registers[byte.in2];
					if (lhs.getType() != rhs.getType()) { Registers[byte.target] = false; goto start; }
					switch (lhs.getType())
					{
					case VariableType::String: Registers[byte.target] = strcmp(lhs.as<String>()->data(), rhs.as<String>()->data()) == 0; goto start;
					case VariableType::Number: Registers[byte.target] = fabs(lhs.as<double>() - rhs.as<double>()) < 0.00001; goto start;
					case VariableType::Boolean: Registers[byte.target] = lhs.as<bool>() == rhs.as<bool>(); goto start;
					case VariableType::Undefined: Registers[byte.target] = false; goto start;
					default:
						Registers[byte.target] = lhs.operator==(rhs);
						goto start;
					}
				} goto start;

				TARGET(NotEqual) {
					auto& lhs = Registers[byte.in1];
					auto& rhs = Registers[byte.in2];
					if (lhs.getType() != rhs.getType()) { Registers[byte.target] = true; goto start; }
					switch (lhs.getType())
					{
					case VariableType::String: Registers[byte.target] = strcmp(lhs.as<String>()->data(), rhs.as<String>()->data()) != 0; goto start;
					case VariableType::Number: Registers[byte.target] = fabs(lhs.as<double>() - rhs.as<double>()) > 0.00001; goto start;
					case VariableType::Boolean: Registers[byte.target] = lhs.as<bool>() != rhs.as<bool>(); goto start;
					case VariableType::Undefined: Registers[byte.target] = false; goto start;
					default:
						Registers[byte.target] = !lhs.operator==(rhs);
						goto start;
					}
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
