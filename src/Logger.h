#pragma once
#include <ostream>
#include <array>
#include <sstream>
#include "EMI/EMI.h"

class DefaultLogger : public EMI::Logger
{
public:
	void Print(const char*) override;

private:

};

class BaseLogger
{
public:
	explicit BaseLogger();
	void SetLogLevel(EMI::LogLevel level);
	void SetLogger(EMI::Logger* log);

	BaseLogger& operator<<(EMI::LogLevel level)
	{
		CurrentLevel = level;
		*this << LogNames[(uint8_t)CurrentLevel];
		return *this;
	}

	template<typename T>
	friend BaseLogger& operator<<(BaseLogger& log, const T& arg) {
		if (log.OutputLevel > log.CurrentLevel) return log;
		std::stringstream ss;
		ss << arg;
		log.Output->Print(ss.str().c_str());
		return log;
	}

private:
	EMI::Logger* Output;
	EMI::LogLevel CurrentLevel;
	EMI::LogLevel OutputLevel;

	static constexpr std::array<const char*, 4> LogNames = { "[Debug] ", "[Info] ", "[Warning] ", "[Error] " };

	bool operator=(const BaseLogger & other) const = delete;
	bool operator=(const BaseLogger && other) const = delete;
	BaseLogger(const BaseLogger& other) = delete;
	BaseLogger(const BaseLogger&& other) = delete;
};

std::string MakePath(const std::string& path);