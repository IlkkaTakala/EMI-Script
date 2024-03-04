#include "Variable.h"
#include "BaseObject.h"
#include "Objects/StringObject.h"
#include <math.h>

Variable::Variable(const Variable& rhs)
{
	if (isObject()) {
		as<Object>()->RefCount--;
	}
	value = rhs.value;

	if (isObject()) {
		as<Object>()->RefCount++;
	}
}

Variable::Variable(Variable&& rhs) noexcept
{
	if (isObject()) {
		as<Object>()->RefCount--;
	}
	value = rhs.value;

	if (isObject()) {
		as<Object>()->RefCount++;
	}
}

Variable::Variable(double v)
{
	memcpy(&value, &v, sizeof(value));
}

Variable::Variable(Object* ptr)
{
	if (ptr == nullptr) value = NIL_VAL;
	else {
		value = OBJ_VAL(ptr);
		as<Object>()->RefCount++;
	}
}

Variable::~Variable()
{
	if (isObject()) {
		as<Object>()->RefCount--;
	}
	value = NIL_VAL;
}

void Variable::setUndefined()
{
	if (isObject()) {
		as<Object>()->RefCount--;
	}
	value = NIL_VAL;
}

bool Variable::isString() const
{
	return isObject() && as<Object>()->Type == VariableType::String;
}

VariableType Variable::getType() const
{
	if (isUndefined()) return VariableType::Undefined;
	if (isNumber()) return VariableType::Number;
	if (isBool()) return VariableType::Boolean;
	if (isObject()) return as<Object>()->Type;
	return VariableType::Undefined;
}

Variable& Variable::operator=(const Variable& rhs)
{
	if (isObject()) {
		as<Object>()->RefCount--;
	}

	value = rhs.value;

	if (isObject()) {
		as<Object>()->RefCount++;
	}

	return *this;
}

Variable& Variable::operator=(Variable&& rhs) noexcept
{
	if (isObject()) {
		as<Object>()->RefCount--;
	}
	value = rhs.value;

	if (isObject()) {
		as<Object>()->RefCount++;
	}
	return *this;
}
