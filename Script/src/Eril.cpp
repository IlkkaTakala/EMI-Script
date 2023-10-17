#include "Eril/Eril.hpp"
#include "Core.h"
#include "VM.h"

using namespace Eril;

VMHandle Eril::CreateEnvironment()
{
	uint idx = CreateVM();
	return VMHandle(idx);
}

Eril::VMHandle::VMHandle(unsigned int value) : Index(value)
{

}

ScriptHandle Eril::VMHandle::CompileScript(const char* file)
{
	auto vm = GetVM(Index);
	return vm->Compile(file);
}

void Eril::VMHandle::ReleaseVM()
{
	::ReleaseVM(Index);
	Index = 0;
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