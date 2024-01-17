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
#include <functional>
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


	typedef std::function<Variable(size_t, Variable*)> __internal_function_;
	typedef std::vector<__internal_function_> __internal_function_lut_;
	inline __internal_function_lut_& __getFunctionTable() {
		static __internal_function_lut_ functions;
		return functions;
	}

	template <typename Ret, typename ...T> requires std::is_convertible_v<Ret, Variable> && ((std::is_convertible_v<T, Variable>) && ...)
	bool RegisterFunction(std::function<Ret(T...)> func) {
		constexpr size_t size = sizeof...(T);
		 [fs = size] <typename Ret, typename ...V, size_t... S> (const std::function<Ret(V...)>& func, const std::index_sequence<S...>) {
			__getFunctionTable().push_back([func, fs](size_t s, Variable* var) -> Variable {
				if (s != fs) return {};
				return func((var[S].as<V>())...);
				}); 
		} (func, std::make_index_sequence<size>());
		printf("Register called on %s\n", func.target_type().name());
		return true;
	}

#define REGISTER_NOVA(Namespace, func) static inline bool __nova_reg_##Namespace##_##function = Eril::RegisterFunction(std::function<decltype(func)>{func});

	class CORE_API VMHandle
	{
	public:
		VMHandle(unsigned int, void*);

		ScriptHandle CompileScript(const char* file, const Options& options = {});
		void CompileTemporary(const char* data);

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