#pragma once
#include "Defines.h"

uint32_t CreateVM();

void ReleaseVM(uint32_t handle);

class VM* GetVM(uint32_t handle);