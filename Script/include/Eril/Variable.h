#ifndef _VARIABLE_INC_GUARD_H_
#define _VARIABLE_INC_GUARD_H_
#pragma once
#include <cstdint>
#include <type_traits>
/**
 
Using NaN boxing 

https://craftinginterpreters.com/optimization.html#nan-boxing

@todo: store small strings directly

*/

#define SIGN_BIT		((uint64_t)0x8000000000000000)
#define QNAN			((uint64_t)0x7ffc000000000000)
#define TAG_NIL			1
#define TAG_FALSE		2
#define TAG_TRUE		3
#define TAG_EXT			4
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
	Object
};

class Variable
{
	uint64_t value;

public:
	Variable() : value(NIL_VAL) {}
	Variable(int v) {
		double val = static_cast<double>(v);
		value = *(uint64_t*)&val;
	}
	Variable(double v) {
		value = *(uint64_t*)&v;
	}
	Variable(uint64_t v) {
		if (v == 0) value = NIL_VAL;
		else value = OBJ_VAL(v);
	}
	Variable(bool v) {
		value = BOOL_VAL(v);
	}
	Variable(const char* v) {
		if (v == nullptr) value = NIL_VAL;
		else value = OBJ_VAL(v);
	}
	Variable(const void* v) {
		if (v == nullptr) value = NIL_VAL;
		else value = OBJ_VAL(v) | TAG_NIL;
	}

	void setUndefined() { value = NIL_VAL; }

	inline bool isNumber() const {
		return (((value)&QNAN) != QNAN);
	}
	inline bool isUndefined() const {
		return value == NIL_VAL;
	}
	inline bool isBool() const {
		return (value | 1) == TRUE_VAL;
	}
	inline bool isExternal() const {
		return ((value) & (QNAN | SIGN_BIT | TAG_NIL)) == (QNAN | SIGN_BIT | TAG_NIL);
	}
	inline bool isObject() const {
		return ((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT) && !(value & TAG_NIL);
	}

	VariableType getType() {
		if (isNumber()) return VariableType::Number;
		if (isUndefined()) return VariableType::Undefined;
		if (isObject()) return VariableType::Object;
		if (isBool()) return VariableType::Boolean;
		if (isExternal()) return VariableType::External;
		return VariableType::Undefined;
	}

	template<typename T>
	T as() const {
		static_assert("No conversion found, check types!");
		return T();
	}

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
	
	template<typename T> requires std::is_pointer_v<T>
	T as() const {
		return ((T)(uintptr_t)((value) & ~(SIGN_BIT | QNAN | TAG_NIL)));
	}

	bool operator==(const Variable& rhs) {
		return value == rhs.value;
	}
};

#endif //_VARIABLE_INC_GUARD_H_