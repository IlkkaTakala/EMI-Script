#include "EMI/EMI.h"
#include "Core.h"
#include "VM.h"
#include "Helpers.h"

using namespace EMI;

bool EMI::_internal_register(_internal_function* func)
{
	std::string name;
	name = func->name;
	auto sym = new Symbol();
	sym->Type = SymbolType::Function;
	sym->VarType = VariableType::Function;
	std::vector<VariableType> types;
	types.resize(func->arg_count);
	for (int i = 0; i < types.size(); i++) {
		types[i] = TypeFromValue(func->arg_types[i]);
	}
	sym->Data = new FunctionSymbol{ FunctionType::Host, func, Variable{}, TypeFromValue(func->return_type) };

	TName fnname = toName(func->name);

	TName space = fnname.Pop();
	while (space.Length() > 0) {
		auto res = HostFunctions().FindName(space);
		if (!res.second && space.toString() != "Global") {
			auto spaceSym = new Symbol();
			spaceSym->setType(SymbolType::Namespace);
			spaceSym->Data = new Namespace{space};
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
	TName out;
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