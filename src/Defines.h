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

inline LogService& gCompileLogger()
{
	static LogService log;
	return log;
}
inline LogService& gCompileDebug()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Debug;
}
inline LogService& gCompileInfo()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Info;
}
inline LogService& gCompileWarn()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Warning;
}
inline LogService& gCompileError()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Error;
}

inline LogService& gRuntimeLogger()
{
	static LogService log;
	return log;
}
inline LogService& gRuntimeDebug()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Debug;
}
inline LogService& gRuntimeInfo()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Info;
}
inline LogService& gRuntimeWarn()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Warning;
}
inline LogService& gRuntimeError()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Error;
}

inline LogService& gScriptLogger()
{
	static LogService log;
	return log;
}

#define X(x) x,
enum class OpCodes : uint8_t
{
#include "Opcodes.h"
};
#undef X

#endif