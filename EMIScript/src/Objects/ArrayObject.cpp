#include "ArrayObject.h"

Array::Array(size_t s)
{
	Type = VariableType::Array;
	Data.clear();
	Data.reserve(s);
}

void Array::Realloc(size_t s)
{
	Data.clear();
	Data.reserve(s);
}

Allocator<Array>* Array::GetAllocator()
{
	static Allocator<Array> alloc;
	return &alloc;
}