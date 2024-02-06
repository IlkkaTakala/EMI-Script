#include "Helpers.h"
#include "StringObject.h"

Variable moveOwnershipToVM(InternalValue& var)
{
	switch (var.getType())
	{
	case ValueType::String: {
		auto idx = String::GetAllocator()->MakeString(var.as<const char*>());
		return { idx, VariableType::String };

	} break;

	case ValueType::Number:
		return var.as<double>();
	case ValueType::Boolean:
		return var.as<bool>();
	case ValueType::External:
		return var.as<void*>();
	default:
		break;
	}
	return {};
}

InternalValue moveOwnershipToHost(Variable& var)
{
	switch (var.getType())
	{
	case VariableType::String: {
		auto str = String::GetAllocator()->GetString(var.as<String>());
		char* out = new char[str->size()]();
		strcpy_s(out, str->size(), str->data());
		return out;
	} break;

	case VariableType::Number:
		return var.as<double>();
	case VariableType::Boolean:
		return var.as<bool>();
	/*case VariableType::External:
		return var.as<void*>();*/
	default:
		break;
	}
	return {};
}

Variable GetTypeDefault(VariableType type)
{
	switch (type)
	{
	case VariableType::Undefined:
		return Variable();
	case VariableType::Number:
		return Variable(0.0);
	case VariableType::Boolean:
		return Variable(false);
	case VariableType::External:
		return Variable();
	case VariableType::String:
		return Variable((uint64_t)0, VariableType::String);
	case VariableType::Array:
		return Variable();
	case VariableType::Object:
		return Variable();
	default:
		// @todo: find proper defaults
		return Variable();
	}
}

bool isTruthy(const Variable& var)
{
	switch (var.getType())
	{
	case VariableType::Undefined:
		return false;
	case VariableType::Number:
		return var.as<int>() != 0;
	case VariableType::Boolean:
		return var.as<bool>();
	case VariableType::String:
		return String::GetAllocator()->GetString(var.as<String>())->size() > 0; // @todo: string check
	case VariableType::Array:
		return true;
	case VariableType::Object:
		return true;
	default:
		// @todo: find proper truths
		return false;
	}
}

bool equal(const Variable& lhs, const Variable& rhs)
{
	if (lhs.getType() != rhs.getType()) return false;
	switch (lhs.getType())
	{
	case VariableType::String: return strcmp(String::GetAllocator()->GetString(lhs.as<String>())->data(), toString(rhs)->data()) == 0;
	default:
		return lhs.operator==(rhs);
		break;
	}
}

void stradd(Variable& out, const Variable& lhs, const Variable& rhs)
{
	auto l = *toString(lhs);
	auto r = *toString(rhs);

	size_t len = l.size() + r.size() - 1;
	auto idx = String::GetAllocator()->MakeString(len);
	auto str = String::GetAllocator()->GetString(idx);

	memcpy(str->data(), l.data(), l.size());
	memcpy(str->data() + l.size() - 1, r.data(), r.size());

	out = { idx, VariableType::String };
}

void add(Variable& out, const Variable& lhs, const Variable& rhs)
{
	switch (lhs.getType())
	{
	case VariableType::Number: out = lhs.as<double>() + toNumber(rhs.as<double>()); return;
	case VariableType::String: stradd(out, lhs, rhs); return;

	default:
		return;
	}
}

void sub(Variable& out, const Variable& lhs, const Variable& rhs)
{
	if (lhs.getType() != rhs.getType()) return;
	switch (lhs.getType())
	{
	case VariableType::Number: out = lhs.as<double>() - rhs.as<double>(); return;

	default:
		return;
	}
}

void mul(Variable& out, const Variable& lhs, const Variable& rhs)
{
	if (lhs.getType() != rhs.getType()) return;
	switch (lhs.getType())
	{
	case VariableType::Number: out = lhs.as<double>() * rhs.as<double>(); return;

	default:
		return;
	}
}

void div(Variable& out, const Variable& lhs, const Variable& rhs)
{
	if (lhs.getType() != rhs.getType()) return;
	switch (lhs.getType())
	{
	case VariableType::Number: out = lhs.as<double>() / rhs.as<double>(); return;

	default:
		return;
	}
}

double toNumber(const Variable& in)
{
	switch (in.getType())
	{
	case VariableType::Number: return in.as<double>();
	case VariableType::Boolean: return in.as<bool>() ? 1.0 : 0.0;
	case VariableType::String: return atof(String::GetAllocator()->GetString(in.as<String>())->data());

	default:
		return 0.0;
	}
}

String* toString(const Variable& in)
{
	auto alloc = String::GetAllocator();
	switch (in.getType())
	{
	case VariableType::Number: return alloc->GetString(alloc->MakeString(std::to_string(in.as<double>()).c_str()));
	case VariableType::Boolean: return alloc->GetString(alloc->MakeString(in.as<bool>() ? "true" : "false"));
	case VariableType::String: return String::GetAllocator()->GetString(in.as<String>());

	default:
		return alloc->GetDefault();
	}
}
