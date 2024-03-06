#include "Core.h"
#include <ankerl/unordered_dense.h>
#include "VM.h"

uint32_t Index = 0;
ankerl::unordered_dense::map<uint32_t, VM*> VMs = {};

uint32_t CreateVM()
{
    auto vm = new VM();
    uint32_t idx = ++Index;
    VMs.emplace(idx, vm);



    return idx;
}

void ReleaseVM(uint32_t handle)
{
    delete VMs[handle];
    VMs.erase(handle);
}

VM* GetVM(uint32_t handle)
{
    return VMs[handle];
}
