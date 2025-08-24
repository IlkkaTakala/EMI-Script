#include "ArrayObject.h"

Array::Array(size_t s)
{
	Type = VariableType::Array;
	Data.clear();
	Data.reserve(s);
}

Array::Array(const Array& array)
{
	Type = VariableType::Array;
	Data.clear();
	Data = array.Data;
}

Array::~Array()
{
	Data.clear();
}

void Array::Realloc(size_t s)
{
	Data.clear();
	Data.reserve(s);
}

void Array::Clear()
{
	Data.clear();
}

Allocator<Array>* Array::GetAllocator()
{
	static Allocator<Array> alloc;
	return &alloc;
}