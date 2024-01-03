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

#include <future>
#include "Eril/Variable.h"

namespace Eril
{
	class VMHandle;
	typedef unsigned long ScriptHandle; // @todo: fix type
	struct FunctionHandle
	{
		unsigned long id = 0;
		VMHandle* vm = nullptr;

		operator unsigned long() {
			return id;
		}
		explicit FunctionHandle() {}
		FunctionHandle(unsigned long in) {
			id = in;
		}
	};

	struct Options
	{
		bool Simplify = false;
	};

	class CORE_API VMHandle
	{
	public:
		VMHandle(unsigned int, void*);

		ScriptHandle CompileScript(const char* file, const Options& options = {});

		ScriptHandle CompileMod(const char* file);

		ScriptHandle RecompileScript(ScriptHandle);

		ScriptHandle RecompileMod(ScriptHandle);

		ScriptHandle LoadMod(const char* data);

		bool ExportCompiledMod(const char* file);

		FunctionHandle GetFunctionHandle(const char* name);

		std::future<Variable> CallFunction(FunctionHandle handle, const std::vector<Variable>& args);

		void ExecuteScript();

		void ReleaseMod();

		void ReleaseUnbound();

		void ReleaseVM();

		void RegisterFunction();

		void ReinitializeGrammar(const char* grammar);

	private:
		unsigned int Index;
		void* Vm;
	};

	VMHandle CreateEnvironment();
	void ReleaseEnvironment(VMHandle handle);
}

#endif // !_ERIL_INC_GUARD_HPP