#ifndef _ERIL_INC_GUARD_HPP
#define _ERIL_INC_GUARD_HPP
#pragma once

#ifdef SCRIPTEXPORT
#define CORE_API __declspec(dllexport)
#else
#define CORE_API __declspec(dllimport) 
#endif

typedef unsigned long ScriptHandle;

namespace Eril
{
	class CORE_API VMHandle
	{
	public:
		VMHandle(unsigned int);

		ScriptHandle CompileScript(const char* file);

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

	private:
		unsigned int Index;
	};

	VMHandle CreateEnvironment();
	void ReleaseVM(VMHandle handle);
}

#endif // !_ERIL_INC_GUARD_HPP