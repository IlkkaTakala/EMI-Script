#include "Logger.h"
#include <iostream>
#include <fstream>
#include <filesystem>

LogService::LogService() : Output(nullptr)
{
	Output = new DefaultLogger();
	CurrentLevel = EMI::LogLevel::Info;
	OutputLevel = EMI::LogLevel::Debug;
}

void LogService::SetLogLevel(EMI::LogLevel level)
{
	OutputLevel = level; 
}

void LogService::SetLogger(EMI::Logger* log)
{
	if (Output) delete Output;
	Output = log;
}

std::string MakePath(const std::string& path)
{
	if (std::filesystem::is_regular_file(path))
		return std::filesystem::absolute(path).string();
	else return "";
}

std::string MakeShortPath(const std::string& path)
{
	if (std::filesystem::is_regular_file(path))
		return std::filesystem::path(path).filename().string();
	else return "";
}

void DefaultLogger::Print(const char* str)
{
	std::cout << str;
}

void DefaultLogger::PrintError(const char* str)
{
	std::cerr << str;
}

LogHandle::~LogHandle()
{
	Log << "\n";
}
