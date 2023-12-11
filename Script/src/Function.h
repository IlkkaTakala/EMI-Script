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


	std::vector<uint8> Bytecode;
};