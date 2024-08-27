#ifndef _DEFINES_H_GUARD
#define _DEFINES_H_GUARD
#pragma once
#include "Logger.h"

#ifdef _DEBUG
#define DEBUG
#else
#undef EMI_PARSE_GRAMMAR
#endif

constexpr uint16_t EMI_VERSION = 10100; // Major 01 Minor 01 Patch 00;
constexpr uint8_t FORMAT_VERSION = 1; // Major 01 Minor 01 Patch 00;

inline Logger& gLogger()
{
	static Logger log;
	return log;
}

inline Logger& gDebug()
{
	return gLogger() << '\n' << LogLevel::Debug;
}

inline Logger& gInfo()
{
	return gLogger() << '\n' << LogLevel::Info;
}

inline Logger& gWarn()
{
	return gLogger() << '\n' << LogLevel::Warning;
}

inline Logger& gError()
{
	return gLogger() << '\n' << LogLevel::Error;
}

#define X(x) x,
enum class OpCodes : uint8_t
{
#include "Opcodes.h"
};
#undef X

#endif