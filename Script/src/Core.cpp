#include "Core.h"
#include <ankerl/unordered_dense.h>
#include "VM.h"

uint Index = 0;
ankerl::unordered_dense::map<uint, VM*> VMs = {};

uint CreateVM()
{
    auto vm = new VM();
    uint idx = ++Index;
    VMs.emplace(idx, vm);



    return idx;
}

void ReleaseVM(uint handle)
{
    delete VMs[handle];
    VMs.erase(handle);
}

VM* GetVM(uint handle)
{
    return VMs[handle];
}
