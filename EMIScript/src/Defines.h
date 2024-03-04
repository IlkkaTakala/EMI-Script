#ifndef _DEFINES_H_GUARD
#define _DEFINES_H_GUARD
#pragma once
#include "Logger.h"

#ifdef _DEBUG
#define DEBUG
#else
#undef EMI_PARSE_GRAMMAR
#endif

inline Logger& gLogger()
{
	static Logger log;
	return log;
}

inline Logger& gDebug()
{
	return gLogger() << LogLevel::Debug;
}

inline Logger& gWarn()
{
	return gLogger() << LogLevel::Warning;
}

inline Logger& gError()
{
	return gLogger() << LogLevel::Error;
}

#ifdef _MSC_VER 
typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
typedef unsigned __int64 uint64;
typedef __int8 int8;
typedef __int16 int16;
typedef __int32 int32;
typedef __int64 int64;
#else
typedef unsigned int uint;
typedef unsigned char uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
#endif

#define X(x) x,
enum class OpCodes : uint8
{
#include "Opcodes.h"
};
#undef X

#endif