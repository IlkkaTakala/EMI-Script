#pragma once
#include "BaseObject.h"

class ArrayAllocator;

class Array : public Object
{
public:
	Array() : Array(0) {}

	Array(size_t s);

	std::vector<Variable>& data() { return Data; }
	size_t size() const { return Data.size(); }

	static Allocator<Array>* GetAllocator();

private:
	std::vector<Variable> Data;
};