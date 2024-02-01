#include "Eril/Eril.hpp"
#include "Core.h"
#include "VM.h"

using namespace Eril;

bool Eril::__internal_register(__internal_function* func)
{
	if (ValidHostFunctions().find((uint64_t)func) != ValidHostFunctions().end()) return false;
	auto& space = HostFunctions()[func->space];
	if (space.find(func->name) != space.end()) return false;
	space[func->name] = func;
	ValidHostFunctions().emplace((uint64_t)func);
	return true;
}

bool Eril::__internal_unregister(const char* space, const char* name)
{
	auto& f = HostFunctions();
	if (auto it = f.find(space); it != f.end()) {
		if (auto fun = it->second.find(name); fun != it->second.end()) {
			ValidHostFunctions().erase((uint64_t)fun->second);
			delete fun->second;
			it->second.erase(fun);
		}
	}
	return false;
}

CORE_API void Eril::UnregisterAllExternals()
{
	ValidHostFunctions().clear();
	for (auto& space : HostFunctions()) {
		for (auto& f : space.second) {
			delete f.second;
		}
	}
	HostFunctions().clear();
}

VMHandle Eril::CreateEnvironment()
{
	uint idx = CreateVM();
	return VMHandle(idx, GetVM(idx));
}

Eril::VMHandle::VMHandle(unsigned int value, void* ptr) : Index(value), Vm(ptr)
{

}

ScriptHandle Eril::VMHandle::CompileScript(const char* file, const Options& options)
{
	auto vm = GetVM(Index);
	return vm->Compile(file, options);
}

void Eril::VMHandle::CompileTemporary(const char* data)
{
	((VM*)Vm)->CompileTemporary(data);
}

FunctionHandle Eril::VMHandle::GetFunctionHandle(const char* name)
{
	auto id = ((VM*)Vm)->GetFunctionID(name);
	return FunctionHandle{id, this};
}

VariableHandle Eril::VMHandle::_internal_call(FunctionHandle handle, size_t count, Variable* args)
{
	const std::span<Variable> s(args, count);
	size_t out = ((VM*)Vm)->CallFunction(handle, s);
	return VariableHandle{ out, this };
}

Variable Eril::VMHandle::GetReturn(VariableHandle handle)
{
	return ((VM*)Vm)->GetReturnValue(handle);
}

void Eril::VMHandle::ReleaseVM()
{
	::ReleaseVM(Index);
	Index = 0;
}

void Eril::VMHandle::ReinitializeGrammar(const char* grammar)
{
	auto vm = GetVM(Index);
	vm->ReinitializeGrammar(grammar);
}

void Eril::ReleaseEnvironment(VMHandle handle)
{
	handle.ReleaseVM();
}


/*
Start -> Program
Program -> RProgram Program 
Program -> '' 
RProgram -> ObjectDef 
RProgram -> FunctionDef 
RProgram -> NamespaceDef 
ObjectDef -> Object id 
FunctionDef -> def id ( ) Scope
Scope -> { MStmt } 
MStmt -> Stmt MStmt 
MStmt -> Scope MStmt 
MStmt -> ''
Stmt -> Expr ; 
Expr -> Expr + Expr 
Expr -> x 
NamespaceDef -> Extend id : 

def id ( ) { x ; x ; { x ; } }


FunctionDef -> def id Scope
Scope -> ( MStmt )
MStmt -> Stmt MStmt
MStmt -> Scope MStmt
MStmt -> ''
Stmt -> x ;


def id { x ; x ; { x ; { } } }



*/