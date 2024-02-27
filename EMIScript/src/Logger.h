#pragma once
#include <format>
#include <ostream>
#include <array>

enum class LogLevel : uint8_t
{
	Debug,
	Info,
	Warning,
	Error,
	None,
};

class Logger
{
public:
	explicit Logger();
	void SetLogLevel(int level);

	Logger& operator<<(LogLevel level)
	{
		CurrentLevel = level;
		*this << LogNames[(uint8_t)CurrentLevel];
		return *this;
	}

	template<typename T>
	friend Logger& operator<<(Logger& log, const T& arg) {
		if (log.OutputLevel <= log.CurrentLevel)
			log.Output << arg;
		return log;
	}

private:
	std::ostream Output;
	LogLevel CurrentLevel;
	LogLevel OutputLevel;

	static constexpr std::array<const char*, 4> LogNames = { "[Debug] ", "[Info] ", "[Warning] ", "[Error] " };

	bool operator=(const Logger & other) const = delete;
	bool operator=(const Logger && other) const = delete;
	Logger(const Logger& other) = delete;
	Logger(const Logger&& other) = delete;
};

std::string MakePath(const std::string& path);