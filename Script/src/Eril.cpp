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
	name;
	return 0;
}

VariableHandle Eril::VMHandle::CallFunction(FunctionHandle handle, const std::vector<Variable>& args)
{
	return ((VM*)Vm)->CallFunction(handle, args);
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