#pragma once
#include "Defines.h"
#include "BaseObject.h"
#include <variant>
#include "Function.h"
#include "EMI/EMI.h"

class FunctionAllocator;

class FunctionObject : public Object
{
public:
	FunctionObject(const PathType& name, FunctionTable* table);
	FunctionObject() : FunctionObject("", nullptr) {}
	FunctionObject(const FunctionObject& object);

	static Allocator<FunctionObject>* GetAllocator();

	FunctionTable* Table;
	PathType Name;
};