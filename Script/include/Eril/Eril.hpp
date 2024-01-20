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
#include <string>
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

	// https://stackoverflow.com/a/65382619
	struct __internal_function {
		void* state = 0;
		Variable(*operate)(void*, size_t, Variable*) = 0;
		void(*cleanup)(void*) = 0;
		size_t arg_count = 0;
		const char* name = nullptr;
		const char* space = nullptr;
		VariableType* arg_types = nullptr;

		void clear() { 
			if (cleanup) { cleanup(state); } 
			if (name) delete[] name; 
			if (space) delete[] space; 
			if (arg_types) delete[] arg_types;
			state = 0; operate = 0; cleanup = 0; arg_count = 0; name = nullptr; space = nullptr; arg_types = nullptr;
		}
		__internal_function(__internal_function const&) = delete;
		__internal_function(__internal_function&& o) noexcept : 
			state(o.state), operate(o.operate), cleanup(o.cleanup), arg_count(o.arg_count), name(o.name), space(o.space)
		{ o.cleanup = 0; o.name = nullptr; o.space = nullptr; o.arg_types = nullptr; o.clear(); }
		__internal_function& operator=(__internal_function&& o) noexcept {
			if (this == &o) return *this;
			clear();
			state = o.state;
			operate = o.operate;
			cleanup = o.cleanup;
			arg_count = o.arg_count;
			name = o.name;
			space = o.space;
			o.cleanup = 0; 
			o.name = nullptr; 
			o.space = nullptr;
			o.arg_types = nullptr;
			o.clear();
			return *this;
		}

		__internal_function() {}
		~__internal_function() { clear(); }
		Variable operator()(size_t s, Variable* args)const { 
			if ((!args && s != 0) || s != arg_count) return {};
			return operate(state, s, args); 
		}
	};

	CORE_API bool __internal_register(__internal_function* func);
	CORE_API bool __internal_unregister(const char*, const char*);

	template<class V, class F, typename ...Args, size_t... S> 
	constexpr auto __make_caller(std::index_sequence<S...>) {
		return +[](void* ptr, size_t s, Variable* args)->Variable {
			return (*(F*)(ptr))((args[S].as<Args>())...); };
	}

	template<class V, class F, typename ...Args, size_t... S> requires std::is_void_v<V>
	constexpr auto __make_caller(std::index_sequence<S...>) {
		return +[](void* ptr, size_t s, Variable* args)->Variable {
			(*(F*)(ptr))((args[S].as<Args>())...); return {}; };
	}

	template<class F, class...Args> requires (std::is_convertible_v<F, Variable> || std::is_void_v<F>) && ((std::is_convertible_v<Args, Variable>) && ...)
	bool RegisterFunction(const std::string& space, const std::string& name, std::function<F(Args...)>&& f) {
		auto retval = new __internal_function();
		constexpr size_t size = sizeof...(Args);
		constexpr auto seq = std::make_index_sequence<size>();
		char* c = new char[name.length() + 1];
		strcpy_s(c, name.length() + 1, name.c_str());
		retval->name = c;
		c = new char[space.length() + 1];
		strcpy_s(c, space.length() + 1, space.c_str());
		retval->space = c;
		retval->arg_types = new VariableType[size]{type<Args>()...};
		retval->arg_count = size;
		retval->state = new std::decay_t<std::function<F(Args...)>>(std::forward<std::function<F(Args...)>>(f));
		retval->operate = __make_caller<F, std::decay_t<std::function<F(Args...)>>, Args...>(std::make_index_sequence<size>());
		retval->cleanup = +[](void* ptr) {delete static_cast<std::decay_t<std::function<F(Args...)>>*>(ptr); };
		return __internal_register(retval);
	}

	inline bool UnregisterFunction(const std::string& space, const std::string& name) {
		return __internal_unregister(space.c_str(), name.c_str());
	}

	CORE_API void UnregisterAllExternals();

#define REGISTER_NOVA(Namespace, name, func) static inline bool __nova_reg_##Namespace##_##name = Eril::RegisterFunction(#Namespace, #name, std::function{func});

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