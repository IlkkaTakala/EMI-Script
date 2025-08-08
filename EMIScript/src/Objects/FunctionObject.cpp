#include "FunctionObject.h"

FunctionObject::FunctionObject(FunctionType type, const PathType& name)
	: InternalType(type), Name(name)
{
	Type = VariableType::Function;
}

FunctionObject::FunctionObject(const FunctionObject& object)
{
	Type = VariableType::Function;
	InternalType = object.InternalType;
	Name = object.Name;
}

Allocator<FunctionObject>* FunctionObject::GetAllocator()
{
	static Allocator<FunctionObject> alloc;
	return &alloc;
}