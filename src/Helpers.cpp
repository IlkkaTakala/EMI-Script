#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Objects/FunctionObject.h"
#include "Objects/UserObject.h"
#include <string>
#include <numeric>
#include <cmath>
#include <math.h>

Variable moveOwnershipToVM(const InternalValue& var)
{
	switch (var.getType())
	{
	case ValueType::String: {
		auto idx = String::GetAllocator()->Make(var.as<const char*>());
		return idx;

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

InternalValue moveOwnershipToHost(const Variable& var)
{
	switch (var.getType())
	{
	case VariableType::String: {
		auto str = var.as<String>();
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

InternalValue makeHostArg(const Variable& var)
{
	switch (var.getType())
	{
	case VariableType::String: {
		return var.as<String>()->data();
	}
	case VariableType::Number:
		return var.as<double>();
	case VariableType::Boolean:
		return var.as<bool>();
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
		return String::GetAllocator()->Make();
	case VariableType::Function:
		return FunctionObject::GetAllocator()->Make();
	case VariableType::Array:
		return Array::GetAllocator()->Make();
	case VariableType::Object:
	default:
		return GetManager().Make(type);
	}
}

Variable CopyVariable(const Variable& var)
{
	switch (var.getType())
	{
	case VariableType::Undefined:
	case VariableType::Number:
	case VariableType::Boolean:
	case VariableType::External:
		return var;
	case VariableType::String:
		return String::GetAllocator()->Copy(var.as<String>());
	case VariableType::Function:
		return FunctionObject::GetAllocator()->Copy(var.as<FunctionObject>());
	case VariableType::Array:
		return Array::GetAllocator()->Copy(var.as<Array>());
	case VariableType::Object:
	default:
		return UserObject::GetAllocator()->Copy(var.as<UserObject>());
	}
}

VariableType TypeFromValue(ValueType type)
{
	switch (type)
	{
	case ValueType::Undefined:
		return VariableType::Undefined;
	case ValueType::Number:
		return VariableType::Number;
	case ValueType::Boolean:
		return VariableType::Boolean;
	case ValueType::External:
		return VariableType::External;
	case ValueType::String:
		return VariableType::String;
	default:
		return VariableType::Undefined;
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
		return var.as<String>()->size() > 0; // @todo: string check
	case VariableType::Array:
		return var.as<Array>()->size() > 0;
	case VariableType::Object:
		return true;
	case VariableType::Function:
		return var.as<FunctionObject>()->Table != nullptr;
	default:
		return false;
	}
}

bool equal(const Variable& lhs, const Variable& rhs)
{
	if (lhs.getType() != rhs.getType()) return false;
	switch (lhs.getType())
	{
	case VariableType::String: return strcmp(lhs.as<String>()->data(), rhs.as<String>()->data()) == 0;
	case VariableType::Number: return abs(lhs.as<double>() - rhs.as<double>()) <= 0.00001;
	case VariableType::Boolean: return lhs.as<bool>() == rhs.as<bool>();
	default:
		return lhs.operator==(rhs);
		break;
	}
}

void stradd(Variable& out, const Variable& lhs, const Variable& rhs)
{
	auto l = toString(lhs);
	String* str = nullptr;

	if (rhs.isString()) {
		auto r = rhs.as<String>();

		size_t len = l->size() + r->size();
		str = String::GetAllocator()->Make(len - 1);

		memcpy(str->data(), l->data(), l->size());
		memcpy(str->data() + l->size() - 1, r->data(), r->size());
	}
	else {
		auto r = toStdString(rhs);

		size_t len = l->size() + r.size();
		str = String::GetAllocator()->Make(len);

		memcpy(str->data(), l->data(), l->size());
		memcpy(str->data() + l->size() - 1, r.data(), r.size());
		str->data()[len - 1] = 0;
	}

	out = str;
}

void add(Variable& out, const Variable& lhs, const Variable& rhs)
{
	switch (lhs.getType())
	{
	case VariableType::Number: out = lhs.as<double>() + toNumber(rhs); return;
	case VariableType::String: stradd(out, lhs, rhs); return;

	default:
		return;
	}
}

void sub(Variable& out, const Variable& lhs, const Variable& rhs)
{
	switch (lhs.getType())
	{
	case VariableType::Number: out = lhs.as<double>() - toNumber(rhs); return;

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
	case VariableType::Number: {
		auto val = lhs.as<double>() / rhs.as<double>();
		if (!isnan(val)) {
			out = val;
		}
		else {
			out.setUndefined();
		}
		return;
	}

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
	case VariableType::String: return atof(in.as<String>()->data());

	default:
		return 0.0;
	}
}

String* toString(const Variable& in)
{
	auto alloc = String::GetAllocator();
	switch (in.getType())
	{
	case VariableType::Number: {
		double value = in.as<double>();
		if (trunc(value) == value) {
			return alloc->Make(std::to_string((size_t)value).c_str());
		}
		return alloc->Make(std::to_string(value).c_str());
	}
	case VariableType::Boolean: return alloc->Make(in.as<bool>() ? "true" : "false");
	case VariableType::String: return in.as<String>();
	case VariableType::Function: return alloc->Make((const char*)in.as<FunctionObject>()->Name);
	case VariableType::Array: {
		std::string out = "[ ";
		size_t index = 0;
		auto& arr = in.as<Array>()->data();
		for (auto& var : arr) {
			out += toStdString(var) + (index++ + 1 < arr.size() ? ", " : "");
		}
		out += " ]";
		return alloc->Make(out.c_str(), out.size());
	}
	default:
		return alloc->Make();
	}
}

std::string toStdString(const Variable& in)
{
	switch (in.getType())
	{
	case VariableType::Number: {
		double value = in.as<double>();
		if (trunc(value) == value) {
			return std::to_string((int64_t)value).c_str();
		}
		return std::to_string(value).c_str();
	}
	case VariableType::Boolean: return in.as<bool>() ? "true" : "false";
	case VariableType::String: return in.as<String>()->data();
	case VariableType::Function: return in.as<FunctionObject>()->Name.toString();
	case VariableType::Array: {
		std::string out = "[ ";
		size_t index = 0; 
		auto& arr = in.as<Array>()->data();
		for (auto& var : arr) {
			out += toStdString(var) + (index++ + 1 < arr.size() ? ", " : "");
		}
		return out + " ]";
	} 

	default:

		if (in.getType() > VariableType::Object) {
			auto obj = in.as<UserObject>();
			std::string out = "{ ";

			for (uint16_t i = 0; i < obj->size(); i++) {
				out += toStdString((*obj)[i]) + (i + 1 < obj->size() ? ", " : "");
			}

			out += " }";
			return out;
		}
		return "Undefined";
	}
}
