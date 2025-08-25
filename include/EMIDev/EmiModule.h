#pragma once
#if defined(BUILD_SHARED_LIBS) && defined(_WIN64)
#ifdef MODULEEXPORT
#define MODULE_API __declspec(dllexport)
#else
#define MODULE_API __declspec(dllimport) 
#endif
#else
#define MODULE_API
#endif
#include "EMIDev/Variable.h"

typedef void(*ModuleFnPtr)(Variable& out, Variable* args, size_t argc);

struct ModuleFunction {
	const char* Name;
	ModuleFnPtr FnPtr;
	VariableType ReturnType;
	int ArgCount;
	VariableType* Args;
	const char** ArgNames;
	bool HasReturn = false;
	bool AnyArgs = false;
};

struct ModuleVariable {
	const char* name;
	void* VarPtr;
	size_t Size;
	VariableType StaticType;

};

struct ModuleType {
	const char* name;
	int FieldCount;
	const char** FieldNames;
	Variable* DefaultFields;
	VariableType* DefaultTypes;
};

struct Module {
	const char* name;

	int FunctionCount;
	ModuleFunction* Functions;
	int VariableCount;
	ModuleVariable* Variables;
	int TypeCount;
	ModuleType* Types;
};

MODULE_API Module loader();