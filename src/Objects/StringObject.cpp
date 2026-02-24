#include "StringObject.h"

String::String(const char* str, size_t s)
{
	Type = VariableType::String;
	Size = s;
	Data = new char[s + 1]();
	if (str) memcpy(Data, str, s);
	Data[s] = '\0';
	Capacity = Size;
}

String::String(const String& str)
{
	Type = VariableType::String;
	Size = str.Size;
	Data = new char[Size + 1]();
	memcpy(Data, str.Data, Size);
	Data[Size] = '\0';
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
		Data = new char[len + 1]();
		if (str) memcpy(Data, str, len);
		Data[len] = '\0';
		Capacity = Size;
	}
	else {
		Size = len;
		if (str != nullptr) memcpy(Data, str, len);
		Data[len] = '\0';
	}
}