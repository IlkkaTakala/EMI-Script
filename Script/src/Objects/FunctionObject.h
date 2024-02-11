#pragma once
#include "BaseObject.h"
#include <queue>
#include <mutex>

class FunctionAllocator;

class Function : public Object
{
public:
	Function() {}

	static Allocator<Function>* GetAllocator();

private:
};