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

#include <vector>
#include "Eril/Variable.h"

namespace Eril
{
	class VMHandle;
	typedef unsigned long ScriptHandle; // @todo: fix type
	
	class VariableHandle
	{
		size_t id = 0;
		VMHandle* vm = nullptr;

	public:
		operator size_t() {
			return id;
		}

		template<typename T>
		T get();

		explicit VariableHandle(size_t in, VMHandle* handle) {
			id = in;
			vm = handle;
		}
	};

	class FunctionHandle
	{
		size_t id = 0;
		VMHandle* vm = nullptr;

	public:
		template<typename ...Args> requires (std::is_convertible_v<Args, Variable> && ...)
		VariableHandle operator()(Args... args);

		operator size_t() {
			return id;
		}
		explicit FunctionHandle(size_t in, VMHandle* handle) {
			id = in;
			vm = handle;
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

		FunctionHandle GetFunctionHandle(const char* name);

		VariableHandle _internal_call(FunctionHandle handle, size_t count, Variable* args);

		template<typename ...Args> requires (std::is_convertible_v<Args, Variable> && ...)
		VariableHandle CallFunction(FunctionHandle handle, Args... args) {
			std::vector<Variable> params = { Variable(args)... };
			return _internal_call(handle, params.size(), params.data());
		}

		Variable GetReturn(VariableHandle handle);

		void ReleaseVM();

		void RegisterFunction();

		void ReinitializeGrammar(const char* grammar);

	private:
		unsigned int Index;
		void* Vm;
	};

	VMHandle CreateEnvironment();
	void ReleaseEnvironment(VMHandle handle);

	template<typename T>
	T VariableHandle::get() {
		return vm->GetReturn(*this).as<T>();
	}
	template<typename ...Args> requires (std::is_convertible_v<Args, Variable> && ...)
		VariableHandle FunctionHandle::operator()(Args... args) {
		return vm->CallFunction(*this, args...);
	}
}

#endif // !_ERIL_INC_GUARD_HPP