#include "FunctionObject.h"

Allocator<Function>* Function::GetAllocator()
{
	static Allocator<Function> alloc;
	return &alloc;
}