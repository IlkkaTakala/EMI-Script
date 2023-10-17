#pragma once
#include "Defines.h"
#include "VM.h"
#include "Lexer.h"

class Parser
{
public:
	static void InitializeParser();
	static void ThreadedParse(VM* vm);
	static void Parse(VM* vm, CompileOptions& options);
};
