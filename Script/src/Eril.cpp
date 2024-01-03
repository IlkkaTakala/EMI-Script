#include "Eril/Eril.hpp"
#include "Core.h"
#include "VM.h"

using namespace Eril;

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