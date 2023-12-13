#ifndef _ERIL_INC_GUARD_HPP
#define _ERIL_INC_GUARD_HPP
#pragma once

#ifdef SHARED_LIBS
#ifdef SCRIPTEXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport) 
#endif
#else
#define CORE_API
#endif

#include <string>

typedef unsigned long ScriptHandle;

namespace Eril
{
	struct Options
	{
		bool Simplify = false;
	};

	class CORE_API VMHandle
	{
	public:
		VMHandle(unsigned int);

		ScriptHandle CompileScript(const char* file, const Options& options = {});

		ScriptHandle CompileMod(const char* file);

		ScriptHandle RecompileScript(ScriptHandle);

		ScriptHandle RecompileMod(ScriptHandle);

		ScriptHandle LoadMod(const char* data);

		bool ExportCompiledMod(const char* file);

		void GetFunctionHandle();

		void CallFunction();

		void ExecuteScript();

		void ReleaseMod();

		void ReleaseUnbound();

		void ReleaseVM();

		void RegisterFunction();

		void ReinitializeGrammar(const char* grammar);

	private:
		unsigned int Index;
	};

	VMHandle CreateEnvironment();
	void ReleaseEnvironment(VMHandle handle);
}

#endif // !_ERIL_INC_GUARD_HPP