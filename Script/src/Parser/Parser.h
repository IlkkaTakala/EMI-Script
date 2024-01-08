#pragma once
#include "Defines.h"
#include "VM.h"

struct Node;
class Parser
{
public:
	static void InitializeParser();
	static void InitializeGrammar(const char* grammar);
	static void ReleaseParser();
	static void ThreadedParse(VM* vm);
	static void Parse(VM* vm, const CompileOptions& options);
	static Node* ConstructAST(const CompileOptions& options);
};
