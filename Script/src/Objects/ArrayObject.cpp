#include "ArrayObject.h"

Array::Array(size_t s)
{
	Type = VariableType::Array;
	Data.reserve(s);
}

Allocator<Array>* Array::GetAllocator()
{
	static Allocator<Array> alloc;
	return &alloc;
}