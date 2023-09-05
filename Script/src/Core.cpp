#include "Core.h"
#include <unordered_map>
#include "VM.h"

uint Index = 0;
std::unordered_map<uint, VM*> VMs = {};

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
