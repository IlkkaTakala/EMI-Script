#include "Logger.h"
#include <iostream>
#include <fstream>
#include <filesystem>

Logger::Logger() : Output(nullptr)
{
	//std::ofstream out("../../log.txt");
	Output.rdbuf(std::cout.rdbuf());
	CurrentLevel = LogLevel::Info;
	OutputLevel = LogLevel::Debug;
}

void Logger::SetLogLevel(int level)
{
	switch (level)
	{
	case 0: OutputLevel = LogLevel::Debug; break;
	case 1: OutputLevel = LogLevel::Info; break;
	case 2: OutputLevel = LogLevel::Warning; break;
	case 3: OutputLevel = LogLevel::Error; break;
	case 4: OutputLevel = LogLevel::None; break;
	default:
		OutputLevel = LogLevel::None;
		break;
	}
}

std::string MakePath(const std::string& path)
{
	return std::filesystem::absolute(path).string();
}
