#pragma once
#include "Defines.h"
#include <string>
#include <vector>

struct Function
{
	std::string Name;
	std::string Path;

	// Definition/Signature?
	// Call statistics?
	uint8 ArgCount;

	std::vector<uint8> Bytecode;
};