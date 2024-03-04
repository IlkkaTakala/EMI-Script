#pragma once
#include "Defines.h"
#include "BaseObject.h"
#include <variant>
#include "Function.h"
#include "EMI/EMI.h"

class FunctionAllocator;

enum class FunctionType 
{
	User,
	Host,
	Intrinsic,
	None
};

class FunctionObject : public Object
{
public:
	FunctionObject(const std::string& name) : FunctionObject(FunctionType::None, name) {}
	FunctionObject(FunctionType type, const std::string& name);
	FunctionObject() : FunctionObject(FunctionType::None, "") {}

	static Allocator<FunctionObject>* GetAllocator();

	FunctionType InternalType;
	std::string Name;

	std::variant<Function*, EMI::__internal_function*, IntrinsicPtr> Callee;

};