#include "FunctionObject.h"

FunctionObject::FunctionObject(FunctionType type, const std::string& name)
	: InternalType(type), Name(name)
{
	Type = VariableType::Function;
}

Allocator<FunctionObject>* FunctionObject::GetAllocator()
{
	static Allocator<FunctionObject> alloc;
	return &alloc;
}