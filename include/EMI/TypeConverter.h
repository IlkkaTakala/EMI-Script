#ifndef _EMITYPECONVERTER_INC_GUARD_HPP
#define _EMITYPECONVERTER_INC_GUARD_HPP
#pragma once

#include "Value.h"

template<typename T>
class TypeConverter
{
public:

	static bool ToHost(const InternalValue& value, T& out) {
		return false;
	}

	static void ToVm(const T& val, InternalValue& out) {
		out = InternalValue();
	}
};

template<typename T>
class TypeConverter
{
public:

	static bool ToHost(const InternalValue& value, T& out) {
		return false;
	}

	static void ToVm(const T& val, InternalValue& out) {
		out = InternalValue();
	}
};

template<typename T>
class TypeConverter
{
public:

	static bool ToHost(const InternalValue& value, T& out) {
		return false;
	}

	static void ToVm(const T& val, InternalValue& out) {
		out = InternalValue();
	}
};



#endif //_EMITYPECONVERTER_INC_GUARD_HPP