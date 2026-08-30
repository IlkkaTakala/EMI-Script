#pragma once
#include <exception>
#include <string>
#include "EMIDev/Variable.h"
#include "Core.h"

class RuntimeException : public std::exception
{
public:
	RuntimeException(const PathType& type, const char* message) : std::exception(message), TypeName(type) {}
	RuntimeException(const PathType& type, const std::string& message) : std::exception(message.c_str()), TypeName(type) {}

	Variable ToVMException() const;

private:

	PathType TypeName;
};
