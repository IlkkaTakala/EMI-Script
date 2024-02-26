#pragma once
#include "BaseObject.h"

class ArrayAllocator;

class Array : public Object
{
public:
	Array() : Array(0) {}
	Array(size_t s);

	void Realloc(size_t s);
	void Realloc() { Realloc(0); }

	std::vector<Variable>& data() { return Data; }
	size_t size() const { return Data.size(); }

	static Allocator<Array>* GetAllocator();

private:
	std::vector<Variable> Data;
};