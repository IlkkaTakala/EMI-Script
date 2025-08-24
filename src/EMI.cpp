#include "EMI/EMI.h"
#include "Core.h"
#include "VM.h"
#include "Helpers.h"

using namespace EMI;

bool EMI::_internal_register(_internal_function* func)
{
	std::string name;
	name = func->name;

	// @todo: Should find the existing symbol
	auto sym = new Symbol();
	sym->Type = SymbolType::Function;
	sym->VarType = VariableType::Function;
	sym->Builtin = true;
	std::vector<VariableType> types;
	types.resize(func->arg_count);
	for (int i = 0; i < types.size(); i++) {
		types[i] = TypeFromValue(func->arg_types[i]);
	}

	auto fn = new FunctionSymbol{  };
	fn->Signature.Return = TypeFromValue(func->return_type);
	fn->Type = FunctionType::Host;
	fn->Host = func;
	fn->IsPublic = true;
	fn->Signature.Arguments.resize(func->arg_count);
	for (int i = 0; i < func->arg_count; i++) {
		fn->Signature.Arguments[i] = TypeFromValue(func->arg_types[i]);
	}
	sym->Function = new FunctionTable();

	sym->Function->AddFunction((int)func->arg_count, fn);

	PathType fnname = toPath(func->name);

	PathType space = fnname.Pop();
	while (space.Length() > 0) {
		auto res = HostFunctions().FindName(space);
		if (!res.second && space.toString() != "Global") {
			auto spaceSym = new Symbol();
			spaceSym->setType(SymbolType::Namespace);
			spaceSym->Space = new Namespace{space};
			spaceSym->Builtin = true;
			HostFunctions().AddName(space, spaceSym);
		}
		space = space.Pop();
	}

	bool success = HostFunctions().AddName(fnname, sym);
	return success;
}

bool EMI::_internal_unregister(const char* name)
{
	auto& f = HostFunctions();
	std::string strname = name;
	auto parts = splits(strname, '.');
	PathType out;
	for (auto p = parts.rbegin(); p != parts.rend(); p++) {
		out = out.Append(p->c_str());
	}
	if (auto it = f.FindName(out); it.first) {
		delete it.second;
		f.Table.erase(it.first);
	}
	return false;
}

CORE_API void EMI::UnregisterAllExternals()
{
	for (auto& f : HostFunctions().Table) {
		delete f.second;
	}
	HostFunctions().Table.clear();
}

VMHandle EMI::CreateEnvironment()
{
	uint32_t idx = CreateVM();
	return VMHandle(idx, GetVM(idx));
}

CORE_API void EMI::SetCompileLogLevel(LogLevel level)
{
	gCompileLogger().SetLogLevel(level);
}

CORE_API void EMI::SetRuntimeLogLevel(LogLevel level)
{
	gRuntimeLogger().SetLogLevel(level);
}

CORE_API void EMI::SetScriptLogLevel(LogLevel level)
{
	gScriptLogger().SetLogLevel(level);
}

CORE_API void EMI::SetLogLevel(LogLevel level)
{
	gCompileLogger().SetLogLevel(level);
	gRuntimeLogger().SetLogLevel(level);
	gScriptLogger().SetLogLevel(level);
}

CORE_API void EMI::SetCompileLog(Logger* log)
{
	gCompileLogger().SetLogger(log);
}

CORE_API void EMI::SetRuntimeLog(Logger* log)
{
	gRuntimeLogger().SetLogger(log);
}

CORE_API void EMI::SetScriptLog(Logger* log)
{
	gScriptLogger().SetLogger(log);
}

EMI::VMHandle::VMHandle(unsigned int value, void* ptr) : Index(value), Vm(ptr)
{

}

ScriptHandle EMI::VMHandle::CompileScript(const char* file, const Options& options)
{
	auto vm = GetVM(Index);
	return { vm->Compile(file, options), this };
}

void EMI::VMHandle::CompileTemporary(const char* data)
{
	((VM*)Vm)->CompileTemporary(data);
}

FunctionHandle EMI::VMHandle::GetFunctionHandle(const char* name)
{
	auto id = ((VM*)Vm)->GetFunctionID(name);
	return FunctionHandle{id, this};
}

ValueHandle EMI::VMHandle::_internal_call(FunctionHandle handle, size_t count, InternalValue* args)
{
	const std::span<InternalValue> s(args, count);
	size_t out = ((VM*)Vm)->CallFunction(handle, s);
	return ValueHandle{ out, this };
}

bool EMI::VMHandle::_internal_wait(void* ptr)
{
	return ((VM*)Vm)->WaitForResult(ptr);
}

InternalValue EMI::VMHandle::GetReturn(ValueHandle handle)
{
	return ((VM*)Vm)->GetReturnValue(handle);
}

bool EMI::VMHandle::ExportVM(const char* path, const ExportOptions& options)
{
	return ((VM*)Vm)->Export(path, options);
}

void EMI::VMHandle::Interrupt()
{
	return ((VM*)Vm)->Interrupt();
}

void EMI::VMHandle::ReleaseVM()
{
	::ReleaseVM(Index);
	Index = 0;
}

void EMI::VMHandle::ReinitializeGrammar(const char* grammar)
{
	auto vm = GetVM(Index);
	vm->ReinitializeGrammar(grammar);
}

void EMI::ReleaseEnvironment(VMHandle handle)
{
	handle.ReleaseVM();
}