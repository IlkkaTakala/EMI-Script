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
	FunctionObject(const TName& name) : FunctionObject(FunctionType::None, name) {}
	FunctionObject(FunctionType type, const TName& name);
	FunctionObject() : FunctionObject(FunctionType::None, "") {}

	static Allocator<FunctionObject>* GetAllocator();

	FunctionType InternalType;
	TName Name;

	std::variant<Function*, EMI::_internal_function*, IntrinsicPtr> Callee;

};