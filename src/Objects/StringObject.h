#pragma once
#include "BaseObject.h"
#include <cstring>

class String : public Object
{
public:
	String(const char* str, size_t s);
	String(const char* str) : String(str, strlen(str)) {}
	String(size_t s) : String(nullptr, s) {};
	String() : String("", 0) {}
	String(const String& str);

	void Realloc(const char* str) { Realloc(str, strlen(str)); }
	void Realloc(size_t s) { Realloc(nullptr, s); };
	void Realloc() { Realloc("", 0); }

	~String() {
		delete[] Data;
		Data = nullptr;
	}

	char* data() { return Data; }
	size_t size() const { return Size; }

	static Allocator<String>* GetAllocator();

	void Realloc(const char* str, size_t len);

private:
	char* Data;
	size_t Size;
	size_t Capacity;
};