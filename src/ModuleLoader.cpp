#include "ModuleLoader.h"
#include "EMIDev/EmiModule.h"

// @todo: Add linux support
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

ModuleWrapper ModuleLoader::LoadModule(const char* path)
{
    HMODULE dll = LoadLibrary(path);

    if (!dll)
        return ModuleWrapper();

    auto loader = GetProcAddress(dll, "");
    loader;
    return ModuleWrapper();
}

#else 
ModuleWrapper ModuleLoader::LoadModule(const char*)
{
    return ModuleWrapper();
}
#endif
