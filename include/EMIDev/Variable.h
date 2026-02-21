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
#define TAG				((uint64_t)0x0000000000000003)
#define TAG_OBJECT		0
#define TAG_NIL			1
#define TAG_BOOL		2
#define NIL_VAL			(uint64_t)(QNAN | TAG_NIL)
#define BOOL_VAL(b)		(QNAN | ((uint64_t)b << 2) | TAG_BOOL)
#define OBJ_VAL(obj)	(QNAN | (uint64_t)(uintptr_t)(obj))

// @todo: Maybe int type too?
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
	Variable(double v) {
		memcpy(&value, &v, sizeof(value));
	}
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
		return (value & TAG) == TAG_BOOL;
	}
	inline bool isObject() const {
		return (value&(QNAN|TAG)) == QNAN;
	}
	bool isString() const;

	inline VariableType getType() const
	{
		if (isNumber()) return VariableType::Number;

		switch (value & TAG)
		{
		case TAG_OBJECT: return getObjectType();
		case TAG_NIL: return VariableType::Undefined;
		case TAG_BOOL: return VariableType::Boolean;
		default: return VariableType::Undefined;
		}
	}

	template<typename T> requires (std::is_convertible_v<T, double>)
	T as() const {
		double num;
		memcpy(&num, &value, sizeof(num));
		return static_cast<T>(num);
	}

	template<typename T> requires std::is_class_v<T>
	T* as() const {
		return ((T*)(uintptr_t)((value) & ~(QNAN)));
	}

	bool operator==(const Variable& rhs) const {
		return value == rhs.value;
	}

	Variable& operator=(const Variable& rhs);
	Variable& operator=(Variable&& rhs) noexcept;
	Variable& operator=(double rhs) {
		memcpy(&value, &rhs, sizeof(value));
		return *this;
	}
	Variable& operator=(bool rhs) {
		value = BOOL_VAL(rhs);
		return *this;
	}
	Variable& operator=(int rhs) {
		double val = static_cast<double>(rhs);
		memcpy(&value, &val, sizeof(value));
		return *this;
	}

private:
	VariableType getObjectType() const;
};

template<>
inline bool Variable::as<bool>() const {
	return (bool)(value & ~(QNAN | TAG));
}
#endif //_VARIABLE_INC_GUARD_H_