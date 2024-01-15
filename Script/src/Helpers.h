#pragma once
#include "Eril/Variable.h"

void moveOwnershipToVM(Variable& var);
void moveOwnershipToHost(Variable& var);

Variable GetTypeDefault(VariableType type);