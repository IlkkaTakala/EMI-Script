#ifndef _VARIABLE_INC_GUARD_H_
#define _VARIABLE_INC_GUARD_H_
#pragma once
#include <type_traits>
#include "Eril/Value.h"
/**
 
Using NaN boxing 

https://craftinginterpreters.com/optimization.html#nan-boxing

*/

#define OBJ_TYPE(obj)	(obj & 0xffff)

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
	uint64_t value;

public:
	Variable() : value(NIL_VAL) {}
	Variable(const Variable&);
	Variable(Variable&&) noexcept;
	Variable(int v) {
		double val = static_cast<double>(v);
		value = *(uint64_t*)&val;
	}
	Variable(double v) {
		value = *(uint64_t*)&v;
	}
	Variable(Object* ptr);
	Variable(bool v) {
		value = BOOL_VAL(v);
	}

	~Variable();

	inline void setUndefined() { value = NIL_VAL; }

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

	VariableType getType() const;

	template<typename T> requires (std::is_convertible_v<T, double>)
	T as() const {
		double num;
		num = *(double*)&value;
		return static_cast<T>(num);
	}

	template<>
	bool as() const {
		return value == TRUE_VAL;
	}

	template<typename T> requires std::is_class_v<T>
	T* as() const {
		return ((T*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)));
	}

	bool operator==(const Variable& rhs) const {
		return value == rhs.value;
	}

	operator InternalValue() const;

	Variable& operator=(const Variable& rhs);
	Variable& operator=(Variable&& rhs) noexcept;
};

#endif //_VARIABLE_INC_GUARD_H_