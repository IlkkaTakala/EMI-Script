#pragma once
#include "Defines.h"
#include "Variable.h"
#include "Namespace.h"
#include <ankerl/unordered_dense.h>

typedef void(*IntrinsicPtr)(Variable& out, Variable* args, size_t argc);

extern SymbolTable IntrinsicFunctions;

