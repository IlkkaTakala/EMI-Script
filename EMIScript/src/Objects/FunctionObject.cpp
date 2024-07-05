#include "FunctionObject.h"

FunctionObject::FunctionObject(FunctionType type, const TName& name)
	: InternalType(type), Name(name)
{
	Type = VariableType::Function;
}

Allocator<FunctionObject>* FunctionObject::GetAllocator()
{
	static Allocator<FunctionObject> alloc;
	return &alloc;
}