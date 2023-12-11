#pragma once
#include "Eril/Eril.hpp"
using namespace Eril;

#ifdef _DEBUG
#define DEBUG
#endif

typedef unsigned int uint;
typedef unsigned char uint8;
typedef unsigned __int16 uint16;
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