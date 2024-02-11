#pragma once
#include "BaseObject.h"

class String : public Object
{
public:
	String() : String("", 1) {}
	String(const char* str) : String(str, strlen(str) + 1) {}

	String(const char* str, size_t s);

	String(size_t s) : String(nullptr, s) {};

	~String() {
		delete[] Data;
		Data = nullptr;
	}

	char* data() { return Data; }
	size_t size() const { return Size; }

	static Allocator<String>* GetAllocator();

	void Realloc(const char* str, size_t len) {
		if (Capacity < len) {
			delete[] Data;
			Data = nullptr;
			*this = String(str, len);
		}
		else {
			Size = len;
			if (str != nullptr) memcpy(Data, str, len);
		}
	}

private:
	char* Data;
	size_t Size;
	size_t Capacity;
};