#pragma once
#include "Defines.h"

uint CreateVM();

void ReleaseVM(uint handle);

class VM* GetVM(uint handle);