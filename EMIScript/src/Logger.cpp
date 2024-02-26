#include "Logger.h"
#include <iostream>
#include <fstream>
#include <filesystem>

Logger::Logger() : Output(nullptr)
{
	//std::ofstream out("../../log.txt");
	Output.rdbuf(std::cout.rdbuf());
	CurrentLevel = LogLevel::Info;
	CurrentLevel = LogLevel::Debug;
}

std::string MakePath(const std::string& path)
{
	return std::filesystem::absolute(path).string();
}
