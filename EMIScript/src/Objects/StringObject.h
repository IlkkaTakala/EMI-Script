#pragma once
#include "BaseObject.h"

class String : public Object
{
public:
	String(const char* str, size_t s);
	String(const char* str) : String(str, strlen(str) + 1) {}
	String(size_t s) : String(nullptr, s) {};
	String() : String("", 1) {}

	void Realloc(const char* str) { Realloc(str, strlen(str) + 1); }
	void Realloc(size_t s) { Realloc(nullptr, s); };
	void Realloc() { Realloc("", 1); }

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