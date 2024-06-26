#ifndef _VARIABLE_INC_GUARD_H_
#define _VARIABLE_INC_GUARD_H_
#pragma once
#include <type_traits>
#include <cstdint>
#include <cstring>

/**
 
Using NaN boxing 

https://craftinginterpreters.com/optimization.html#nan-boxing

*/

#define SIGN_BIT		((uint64_t)0x8000000000000000)
#define QNAN			((uint64_t)0x7ffc000000000000)
#define TAG_NIL			1
#define TAG_FALSE		2
#define TAG_TRUE		3
#define NIL_VAL			(uint64_t)(QNAN | TAG_NIL)
#define FALSE_VAL		(uint64_t)(QNAN | TAG_FALSE)
#define TRUE_VAL		(uint64_t)(QNAN | TAG_TRUE)
#define BOOL_VAL(b)		((b) ? TRUE_VAL : FALSE_VAL)
#define OBJ_VAL(obj)	(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

enum class VariableType
{
	Undefined,
	Number,
	Boolean,
	External,
	String,
	Array,
	Function,
	Object
};

class Object;

class Variable
{
	uint64_t value = NIL_VAL;

public:
	Variable() : value(NIL_VAL) {}
	Variable(const Variable&);
	Variable(Variable&&) noexcept;
	Variable(int v) {
		double val = static_cast<double>(v);
		memcpy(&value, &val, sizeof(value));
	}
	Variable(double v);
	Variable(Object* ptr);
	Variable(bool v) {
		value = BOOL_VAL(v);
	}

	~Variable();

	uint64_t get() const { return value; }

	void setUndefined();

	inline bool isNumber() const {
		return (((value)&QNAN) != QNAN);
	}
	inline bool isUndefined() const {
		return value == NIL_VAL;
	}
	inline bool isBool() const {
		return (value | 1) == TRUE_VAL;
	}
	inline bool isObject() const {
		return ((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT);
	}
	bool isString() const;

	inline VariableType getType() const
	{
		if (isUndefined()) return VariableType::Undefined;
		if (isNumber()) return VariableType::Number;
		if (isBool()) return VariableType::Boolean;
		if (isObject()) return getObjectType();
		return VariableType::Undefined;
	}

	template<typename T> requires (std::is_convertible_v<T, double>)
	T as() const {
		double num;
		memcpy(&num, &value, sizeof(num));
		return static_cast<T>(num);
	}

	template<typename T> requires std::is_class_v<T>
	T* as() const {
		return ((T*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)));
	}

	bool operator==(const Variable& rhs) const {
		return value == rhs.value;
	}

	Variable& operator=(const Variable& rhs);
	Variable& operator=(Variable&& rhs) noexcept;

private:
	VariableType getObjectType() const;
};

	template<>
	inline bool Variable::as<bool>() const {
		return value == TRUE_VAL;
	}
#endif //_VARIABLE_INC_GUARD_H_