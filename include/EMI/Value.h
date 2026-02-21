#ifndef _VALUE_INC_GUARD_H_
#define _VALUE_INC_GUARD_H_
#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
/**

Using NaN boxing

https://craftinginterpreters.com/optimization.html#nan-boxing

*/

#define SIGN_BIT		((uint64_t)0x8000000000000000)
#define QNAN			((uint64_t)0x7ffc000000000000)
#define TAG				((uint64_t)0x0000000000000003)
#define TAG_OBJECT		0
#define TAG_NIL			1
#define TAG_BOOL		2
#define NIL_VAL			(uint64_t)(QNAN | TAG_NIL)
#define BOOL_VAL(b)		(QNAN | ((uint64_t)b << 2) | TAG_BOOL)
#define OBJ_VAL(obj)	(QNAN | (uint64_t)(uintptr_t)(obj))

enum class ValueType
{
	Undefined,
	Number,
	Boolean,
	External,
	String,
};

class InternalValue
{
	uint64_t value;

public:
	InternalValue() : value(NIL_VAL) {}
	InternalValue(int v) {
		double val = static_cast<double>(v);
		memcpy(&value, &val, sizeof(value));
	}
	InternalValue(double v) {
		memcpy(&value, &v, sizeof(value));
	}
	InternalValue(bool v) {
		value = BOOL_VAL(v);
	}
	InternalValue(const char* v) {
		if (v == nullptr) value = NIL_VAL;
		else value = OBJ_VAL(v);
	}
	InternalValue(const void*) {
		value = NIL_VAL;
	}

	void setUndefined() { value = NIL_VAL; }

	inline bool isNumber() const {
		return (((value)&QNAN) != QNAN);
	}
	inline bool isUndefined() const {
		return value == NIL_VAL;
	}
	inline bool isBool() const {
		return (value & TAG) == TAG_BOOL;
	}
	inline bool isExternal() const {
		return ((value) & (QNAN | SIGN_BIT | TAG_NIL)) == (QNAN | SIGN_BIT | TAG_NIL);
	}
	inline bool isString() const {
		return ((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT) && !(value & TAG_NIL);
	}

	ValueType getType() const {
		if (isNumber()) return ValueType::Number;
		if (isUndefined()) return ValueType::Undefined;
		if (isString()) return ValueType::String;
		if (isBool()) return ValueType::Boolean;
		if (isExternal()) return ValueType::External;
		return ValueType::Undefined;
	}

	template<typename T>
	T as() const {
		static_assert("No conversion found, check types!");
		return T();
	}

	template<typename T> requires (std::is_convertible_v<T, double>)
		T as() const {
		double num;
		memcpy(&num, &value, sizeof(num));
		return static_cast<T>(num);
	}

	template<typename T> requires std::is_pointer_v<T>
	T as() const {
		return ((T)(uintptr_t)((value) & ~(SIGN_BIT | QNAN | TAG_NIL)));
	}

	bool operator==(const InternalValue& rhs) const {
		return value == rhs.value;
	}
};

template<>
inline bool InternalValue::as() const {
	return (bool)(value & ~(QNAN | TAG));
}

template <typename T>
inline ValueType type() {
	return ValueType::Undefined;
}
template<>
inline ValueType type<bool>() {
	return ValueType::Boolean;
}
template <typename T> requires (std::is_convertible_v<T, double>)
inline ValueType type() {
	return ValueType::Number;
}
template <>
inline ValueType type<const char*>() {
	return ValueType::String;
}/*
template<typename T> requires std::is_pointer_v<T>
inline ValueType type() {
	return ValueType::External;
}*/

#endif //_VARIABLE_INC_GUARD_H_