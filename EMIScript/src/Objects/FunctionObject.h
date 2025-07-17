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
	FunctionObject(const PathType& name) : FunctionObject(FunctionType::None, name) {}
	FunctionObject(FunctionType type, const PathType& name);
	FunctionObject() : FunctionObject(FunctionType::None, "") {}

	static Allocator<FunctionObject>* GetAllocator();

	FunctionType InternalType;
	PathType Name;

	std::variant<Function*, EMI::_internal_function*, IntrinsicPtr> Callee;

};