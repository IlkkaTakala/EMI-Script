#include "EMI/EMI.h"
#include "Core.h"
#include "VM.h"

using namespace EMI;

bool EMI::__internal_register(__internal_function* func)
{
	if (ValidHostFunctions().find((uint64_t)func) != ValidHostFunctions().end()) return false;
	std::string name;
	if (func->space) {
		name = std::string(func->space) + "." + func->name;
	}
	else {
		name = func->name;
	}
	auto& function = HostFunctions()[name];
	if (function) return false;
	function = func;
	ValidHostFunctions().emplace((uint64_t)func);
	return true;
}

bool EMI::__internal_unregister(const char* space, const char* name)
{
	auto& f = HostFunctions();
	std::string strname;
	if (space) {
		strname = std::string(space) + "." + name;
	}
	else {
		strname = name;
	}
	if (auto it = f.find(strname); it != f.end()) {
		ValidHostFunctions().erase((uint64_t)it->second);
		delete it->second;
		f.erase(it);
	}
	return false;
}

CORE_API void EMI::UnregisterAllExternals()
{
	ValidHostFunctions().clear();
	for (auto& f : HostFunctions()) {
		delete f.second;
	}
	HostFunctions().clear();
}

VMHandle EMI::CreateEnvironment()
{
	uint idx = CreateVM();
	return VMHandle(idx, GetVM(idx));
}

CORE_API void EMI::SetLogLevel(int level)
{
	gLogger().SetLogLevel(level);
}

EMI::VMHandle::VMHandle(unsigned int value, void* ptr) : Index(value), Vm(ptr)
{

}

ScriptHandle EMI::VMHandle::CompileScript(const char* file, const Options& options)
{
	auto vm = GetVM(Index);
	return vm->Compile(file, options);
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

InternalValue EMI::VMHandle::GetReturn(ValueHandle handle)
{
	return ((VM*)Vm)->GetReturnValue(handle);
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