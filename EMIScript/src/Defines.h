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