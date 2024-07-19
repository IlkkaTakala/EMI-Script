#ifndef _DEFINES_H_GUARD
#define _DEFINES_H_GUARD
#pragma once
#include "Logger.h"

#ifdef _DEBUG
#define DEBUG
#else
#undef EMI_PARSE_GRAMMAR
#endif

inline BaseLogger& gCompileLogger()
{
	static BaseLogger log;
	return log;
}
inline BaseLogger& gCompileDebug()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Debug;
}
inline BaseLogger& gCompileInfo()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Info;
}
inline BaseLogger& gCompileWarn()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Warning;
}
inline BaseLogger& gCompileError()
{
	return gCompileLogger() << '\n' << EMI::LogLevel::Error;
}

inline BaseLogger& gRuntimeLogger()
{
	static BaseLogger log;
	return log;
}
inline BaseLogger& gRuntimeDebug()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Debug;
}
inline BaseLogger& gRuntimeInfo()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Info;
}
inline BaseLogger& gRuntimeWarn()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Warning;
}
inline BaseLogger& gRuntimeError()
{
	return gRuntimeLogger() << '\n' << EMI::LogLevel::Error;
}

inline BaseLogger& gScriptLogger()
{
	static BaseLogger log;
	return log;
}
inline BaseLogger& gScriptDebug()
{
	return gScriptLogger() << '\n' << EMI::LogLevel::Debug;
}
inline BaseLogger& gScriptInfo()
{
	return gScriptLogger() << '\n' << EMI::LogLevel::Info;
}
inline BaseLogger& gScriptWarn()
{
	return gScriptLogger() << '\n' << EMI::LogLevel::Warning;
}
inline BaseLogger& gScriptError()
{
	return gScriptLogger() << '\n' << EMI::LogLevel::Error;
}

#define X(x) x,
enum class OpCodes : uint8_t
{
#include "Opcodes.h"
};
#undef X

#endif