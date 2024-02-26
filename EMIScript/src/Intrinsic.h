#pragma once
#include "Defines.h"
#include "Variable.h"
#include <ankerl/unordered_dense.h>

typedef void(*IntrinsicPtr)(Variable& out, Variable* args, size_t argc);

extern ankerl::unordered_dense::map<std::string, IntrinsicPtr> IntrinsicFunctions;
extern ankerl::unordered_dense::map<std::string, std::vector<VariableType>> IntrinsicFunctionTypes;

extern std::vector<std::string> DefaultNamespaces;