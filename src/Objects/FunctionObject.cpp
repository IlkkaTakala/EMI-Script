#include "FunctionObject.h"

FunctionObject::FunctionObject(const PathType& name, FunctionTable* symbol)
	: Name(name), Table(symbol)
{
	Type = VariableType::Function;
}

FunctionObject::FunctionObject(const FunctionObject& object)
{
	Type = VariableType::Function;
	Name = object.Name;
}

Allocator<FunctionObject>* FunctionObject::GetAllocator()
{
	static Allocator<FunctionObject> alloc;
	return &alloc;
}