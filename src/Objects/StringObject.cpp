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

String::String(const String& str)
{
	Type = VariableType::String;
	Size = str.Size;
	Data = new char[Size]();
	memcpy(Data, str.Data, Size);
	Capacity = Size;
}

Allocator<String>* String::GetAllocator()
{
	static Allocator<String> alloc;
	return &alloc;
}

void String::Realloc(const char* str, size_t len)
{
	if (Capacity < len) {
		delete[] Data;
		Size = len;
		Data = new char[len]();
		if (str) memcpy(Data, str, len - 1);
		Data[len - 1] = '\0';
		Capacity = Size;
	}
	else {
		Size = len;
		if (str != nullptr) memcpy(Data, str, len);
	}
}