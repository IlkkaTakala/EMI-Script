#pragma once
#include "Eril/Variable.h"

void moveOwnershipToVM(Variable& var);
void moveOwnershipToHost(Variable& var);

Variable GetTypeDefault(VariableType type);

bool isTruthy(const Variable& var);

bool equal(const Variable& lhs, const Variable& rhs);
void add(Variable& out, const Variable& lhs, const Variable& rhs);
void sub(Variable& out, const Variable& lhs, const Variable& rhs);
void mul(Variable& out, const Variable& lhs, const Variable& rhs);
void div(Variable& out, const Variable& lhs, const Variable& rhs);

double toNumber(const Variable& in);