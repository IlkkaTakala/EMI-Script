#pragma once
#include "Object.h"
#include <queue>

class StringAllocator;

class String : public Object
{
public:
	String(const char* str) : String(str, strlen(str) + 1) {}

	String(const char* str, size_t s);

	char* data() { return Data; }
	size_t size() const { return Size; }

	static StringAllocator* GetAllocator();

private:
	friend StringAllocator;
	char* Data;
	size_t Size;
	size_t Capacity;
	size_t Index;
};

class StringAllocator
{
public:
	StringAllocator() {
		MakeString("");
	}

	String* GetDefault() {
		return &Strings[0];
	}

	size_t MakeString(size_t len) {
		return MakeString(nullptr, len);
	}

	size_t MakeString(const char* data) {
		return MakeString(data, strlen(data) + 1);
	}

	size_t MakeString(const char* data, size_t len) {
		if (FreeList.empty()) {
			auto& str = Strings.emplace_back(data, len);
			str.Index = Strings.size() - 1;
			return str.Index;
		}
		else {
			auto idx = FreeList.front();
			FreeList.pop();
			auto& str = Strings[idx];
			if (str.Capacity < len) {
				delete[] str.Data;
				str = std::move(String(data, len));
			}
			else {
				str.Size = len;
				if (data != nullptr) memcpy(str.Data, data, len);
			}
			return idx;
		}
	}

	String* GetString(size_t idx) {
		return &Strings[idx];
	}

	void MakeFree() {
		for (int i = 1; i < Strings.size(); i++) {
			if (Strings[i].RefCount <= 0) {
				FreeList.push(i);
			}
		}
	}

private:

	std::vector<String> Strings;
	std::queue<size_t> FreeList;
};