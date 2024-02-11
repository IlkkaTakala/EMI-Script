#include "StringObject.h"

String::String(const char* str, size_t s)
{
	Type = VariableType::String;
	Size = s;
	Data = new char[s]();
	if (str) memcpy(Data, str, s - 1);
	Data[s - 1] = '\0';
	Capacity = Size;
}

Allocator<String>* String::GetAllocator()
{
	static Allocator<String> alloc;
	return &alloc;
}