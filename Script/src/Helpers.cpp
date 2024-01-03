#include "Helpers.h"

void moveOwnershipToVM(Variable& var)
{
	switch (var.getType())
	{
	case VariableType::Object: {
		// Currently all objects are char arrays

		// @todo: make string object

	} break;

	default:
		break;
	}
    return;
}

void moveOwnershipToHost(Variable& var)
{
	switch (var.getType())
	{
	case VariableType::Object: {
		// Currently all objects are char arrays

		// @todo: make string object

	} break;

	default:
		break;
	}
	return;
}
