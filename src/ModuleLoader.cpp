#include "ModuleLoader.h"
#include "EMIDev/EmiModule.h"

// @todo: Add linux support
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else 

#endif

ModuleWrapper ModuleLoader::LoadModule(const char* path)
{
    HMODULE dll = LoadLibrary(path);

    if (!dll)
        return ModuleWrapper();

    auto loader = GetProcAddress(dll, "");
    loader;
    return ModuleWrapper();
}
