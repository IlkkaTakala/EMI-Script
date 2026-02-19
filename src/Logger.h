#pragma once
#include <ostream>
#include <array>
#include <sstream>
#include "EMI/EMI.h"

class DefaultLogger : public EMI::Logger
{
public:
	void Print(const char*) override;
	void PrintError(const char*) override;

private:

};

class LogService
{
public:
	explicit LogService();
	void SetLogLevel(EMI::LogLevel level);
	void SetLogger(EMI::Logger* log);

	LogService& operator<<(EMI::LogLevel level)
	{
		CurrentLevel = level;
		*this << LogNames[(uint8_t)CurrentLevel];
		return *this;
	}

	template<typename T>
	friend LogService& operator<<(LogService& log, const T& arg) {
		if (log.OutputLevel > log.CurrentLevel) return log;
		std::stringstream ss;
		ss << arg;
		if (log.CurrentLevel < EMI::LogLevel::Warning)
			log.Output->Print(ss.str().c_str());
		else
			log.Output->PrintError(ss.str().c_str());
		return log;
	}

private:
	EMI::Logger* Output;
	EMI::LogLevel CurrentLevel;
	EMI::LogLevel OutputLevel;

	static constexpr std::array<const char*, 4> LogNames = { "[Debug] ", "[Info] ", "[Warning] ", "[Error] " };

	bool operator=(const LogService & other) const = delete;
	bool operator=(const LogService && other) const = delete;
	LogService(const LogService& other) = delete;
	LogService(const LogService&& other) = delete;
};

std::string MakePath(const std::string& path);