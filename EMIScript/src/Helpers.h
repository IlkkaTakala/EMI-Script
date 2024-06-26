#pragma once
#include "Variable.h"
#include "EMI/Value.h"
#include <string>

// @todo: These should be inlined

Variable moveOwnershipToVM(const InternalValue& var);
InternalValue moveOwnershipToHost(const Variable& var);
InternalValue makeHostArg(const Variable& var);

Variable GetTypeDefault(VariableType type);

VariableType TypeFromValue(ValueType type);

bool isTruthy(const Variable& var);

bool equal(const Variable& lhs, const Variable& rhs);
void stradd(Variable& out, const Variable& lhs, const Variable& rhs);
void add(Variable& out, const Variable& lhs, const Variable& rhs);
void sub(Variable& out, const Variable& lhs, const Variable& rhs);
void mul(Variable& out, const Variable& lhs, const Variable& rhs);
void div(Variable& out, const Variable& lhs, const Variable& rhs);

double toNumber(const Variable& in);

class String;
String* toString(const Variable& in);
std::string toStdString(const Variable& in);