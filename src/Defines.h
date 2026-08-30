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
#define gCompileDebug() gCompileLogger().GetHandle() << EMI::LogLevel::Debug
#define gCompileInfo() gCompileLogger().GetHandle() << EMI::LogLevel::Info
#define gCompileWarn() gCompileLogger().GetHandle() << EMI::LogLevel::Warning
#define gCompileError() gCompileLogger().GetHandle() << EMI::LogLevel::Error

inline LogService& gRuntimeLogger()
{
	static LogService log;
	return log;
}
#define gRuntimeDebug() gRuntimeLogger().GetHandle() << EMI::LogLevel::Debug
#define gRuntimeInfo() gRuntimeLogger().GetHandle() << EMI::LogLevel::Info
#define gRuntimeWarn() gRuntimeLogger().GetHandle() << EMI::LogLevel::Warning
#define gRuntimeError() gRuntimeLogger().GetHandle() << EMI::LogLevel::Error

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