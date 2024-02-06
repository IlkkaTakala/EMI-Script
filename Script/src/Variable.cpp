#include "Variable.h"
#include "StringObject.h"

Variable::Variable(const Variable& rhs)
{
	value = rhs.value;

	if (isObject()) {
		String::GetAllocator()->GetString(as<String>())->RefCount++;
	}
}

Variable::Variable(size_t v, VariableType type)
{
	if (v == 0 && type == VariableType::Undefined) value = NIL_VAL;
	else {
		value = OBJ_VAL(v << 16) | (uint16)type;
		switch (type)
		{
		case VariableType::String: {
			auto& count = String::GetAllocator()->GetString(v)->RefCount;
			count--;
		} break;
		default:
			break;
		}
	}
}

Variable::~Variable()
{
	if (isObject()) {
		switch (getType())
		{
		case VariableType::String:
			String::GetAllocator()->GetString(as<String>())->RefCount--;
			break;
		default:
			break;
		}
	}
}