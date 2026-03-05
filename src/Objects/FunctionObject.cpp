#include "FunctionObject.h"

FunctionObject::FunctionObject(const PathType& name, FunctionTable* symbol)
	: Table(symbol), Name(name)
{
	Type = VariableType::Function;
}

FunctionObject::FunctionObject(const FunctionObject& object)
{
	Type = VariableType::Function;
	Name = object.Name;
	Table = object.Table;
}

Allocator<FunctionObject>* FunctionObject::GetAllocator()
{
	static Allocator<FunctionObject> alloc;
	return &alloc;
}