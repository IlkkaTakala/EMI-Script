#pragma once
#include "Logger.h"

#ifdef _DEBUG
#define DEBUG
#endif

inline Logger& gLogger()
{
	static Logger log;
	return log;
}

typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int16 uint32;
typedef __int16 int16;
typedef __int64 int64;
typedef unsigned long ScriptHandle;

enum class OpCodes : uint8
{
	LoadConst,
	LoadSymbol,
	Pop,
	CallSymbol,

	JumpBackward,
	JumpForward,
	Return,
	ReturnValue,
};