#include "String.h"
#include "StringObject.h"

String::String(const char* str, size_t s)
{
	Type = ObjectType::String;
	Size = s;
	Data = new char[s]();
	if (str) memcpy(Data, str, s - 1);
	Data[s - 1] = '\0';
	Index = 0;
	Capacity = Size;
}

StringAllocator* String::GetAllocator()
{
	static StringAllocator alloc;
	return &alloc;
}