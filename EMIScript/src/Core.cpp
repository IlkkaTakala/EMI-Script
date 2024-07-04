#include "Core.h"
#include <ankerl/unordered_dense.h>
#include "VM.h"
#include <map>

uint32_t Index = 0;
ankerl::unordered_dense::map<uint32_t, VM*> VMs = {};

std::unordered_map<std::string, size_t> Strings;
size_t StringCounter;
std::mutex StringMutex;

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

TName::TName() : Text(nullptr), ID(0) {}

TName::TName(const char* text) : ID(0), Text(nullptr)
{
    if (text) {
        std::unique_lock lk(StringMutex);

        if (auto it = Strings.find(text); it == Strings.end()) {
            ID = ++StringCounter;
            auto [str, success] = Strings.emplace(text, ID);
            Text = str->first.c_str();
        }
        else {
            ID = it->second;
            Text = it->first.c_str();
        }
    }
}

TName::~TName()
{
}
