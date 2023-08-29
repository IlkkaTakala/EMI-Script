#ifndef _ERIL_INC_GUARD_
#define _ERIL_INC_GUARD_
#pragma once

#ifdef SCRIPTEXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport) 
#endif

#define _C extern "C"

typedef unsigned long ScriptHandle;

_C CORE_API ScriptHandle CompileScript(const char* file);

_C CORE_API ScriptHandle CompileMod(const char* file);

_C CORE_API ScriptHandle RecompileScript(ScriptHandle);

_C CORE_API ScriptHandle RecompileMod(ScriptHandle);

_C CORE_API ScriptHandle LoadMod(const char* data);

_C CORE_API bool ExportCompiledMod(const char* file);

_C CORE_API void GetFunctionHandle();

_C CORE_API void CallFunction();

_C CORE_API void ExecuteScript();

_C CORE_API void ReleaseMod();

_C CORE_API void ReleaseUnbound();

_C CORE_API void ReleaseAll();

_C CORE_API void RegisterFunction();

#endif // !_ERIL_INC_GUARD_