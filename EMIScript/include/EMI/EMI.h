#ifndef _EMI_INC_GUARD_HPP
#define _EMI_INC_GUARD_HPP
#pragma once

#if defined(BUILD_SHARED_LIBS) && defined(_WIN64)
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
#include <cstring>
#include <functional>
#include "Value.h"

#ifndef _MSC_VER
inline void strcpy_s(char* dest, size_t size, const char* source) {
	strncpy(dest, source, size);
	dest[size - 1] = '\0';
}
#endif

namespace EMI
{
	class VMHandle;
	typedef unsigned long ScriptHandle; // @todo: fix type
	
	class ValueHandle
	{
		size_t id = 0;
		VMHandle* vm = nullptr;
		InternalValue cached;

	public:
		operator size_t() {
			return id;
		}

		template<typename T>
		T get();

		explicit ValueHandle(size_t in, VMHandle* handle) {
			id = in;
			vm = handle;
		}
	};

	class FunctionHandle
	{
		void* id = 0;
		VMHandle* vm = nullptr;

	public:
		template<typename ...Args> requires (std::is_convertible_v<Args, InternalValue> && ...)
		ValueHandle operator()(Args... args);

		operator void*() {
			return id;
		}
		explicit FunctionHandle(void* in, VMHandle* handle) {
			id = in;
			vm = handle;
		}
	};

	struct Options
	{
	};

	// https://stackoverflow.com/a/65382619
	struct __internal_function {
		void* state = 0;
		InternalValue(*operate)(void*, size_t, InternalValue*) = 0;
		void(*cleanup)(void*) = 0;
		size_t arg_count = 0;
		const char* name = nullptr;
		const char* space = nullptr;
		ValueType* arg_types = nullptr;
		ValueType return_type = ValueType::Undefined;

		void clear() { 
			if (cleanup) { cleanup(state); } 
			if (name) delete[] name; 
			if (space) delete[] space; 
			if (arg_types) delete[] arg_types;
			state = 0; operate = 0; cleanup = 0; arg_count = 0; name = nullptr; space = nullptr; arg_types = nullptr;
		}
		__internal_function(__internal_function const&) = delete;
		__internal_function(__internal_function&& o) noexcept : 
			state(o.state), operate(o.operate), cleanup(o.cleanup), arg_count(o.arg_count), name(o.name), space(o.space), return_type(o.return_type)
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
			return_type = o.return_type;
			o.cleanup = 0; 
			o.name = nullptr; 
			o.space = nullptr;
			o.arg_types = nullptr;
			o.clear();
			return *this;
		}

		__internal_function() {}
		~__internal_function() { clear(); }
		InternalValue operator()(size_t s, InternalValue* args)const { 
			if ((!args && s != 0) || s != arg_count) return {};
			return operate(state, s, args); 
		}
	};

	CORE_API bool __internal_register(__internal_function* func);
	CORE_API bool __internal_unregister(const char*, const char*);

	template<class V, class F, typename ...Args, size_t... S> 
	constexpr auto __make_caller(std::index_sequence<S...>) {
		return +[](void* ptr, size_t, InternalValue* args)->InternalValue {
			return (*(F*)(ptr))((args[S].as<Args>())...); };
	}

	template<class V, class F, typename ...Args, size_t... S> requires std::is_void_v<V>
	constexpr auto __make_caller(std::index_sequence<S...>) {
		return +[](void* ptr, size_t, InternalValue* args)->InternalValue {
			(*(F*)(ptr))((args[S].as<Args>())...); return {}; };
	}

	template<class F, class...Args> requires (std::is_convertible_v<F, InternalValue> || std::is_void_v<F>) && ((std::is_convertible_v<Args, InternalValue>) && ...)
	bool RegisterFunction(const std::string& space, const std::string& name, std::function<F(Args...)>&& f) {
		auto retval = new __internal_function();
		constexpr size_t size = sizeof...(Args);
		constexpr auto seq = std::make_index_sequence<size>();
		char* c = new char[name.length() + 1];
		strcpy_s(c, name.length() + 1, name.c_str());
		retval->name = c;
		retval->space = nullptr;
		if (space.length() > 0 && space != "Global") {
			c = new char[space.length() + 1];
			strcpy_s(c, space.length() + 1, space.c_str());
			retval->space = c;
		}
		retval->arg_types = new ValueType[size]{type<Args>()...};
		retval->return_type = type<F>();
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

#define EMI_REGISTER(Namespace, name, func) static inline bool __emi_reg_##Namespace##_##name = EMI::RegisterFunction(#Namespace, #name, std::function{func});

	class CORE_API VMHandle
	{
	public:
		VMHandle(unsigned int, void*);

		ScriptHandle CompileScript(const char* file, const Options& options = {});
		void CompileTemporary(const char* data);

		FunctionHandle GetFunctionHandle(const char* name);

		ValueHandle _internal_call(FunctionHandle handle, size_t count, InternalValue* args);

		InternalValue GetReturn(ValueHandle handle);

		void Interrupt();

		void ReleaseVM();

		void ReinitializeGrammar(const char* grammar);

	private:
		unsigned int Index;
		void* Vm;
	};

	CORE_API VMHandle CreateEnvironment();
	CORE_API void SetLogLevel(int level);
	CORE_API void ReleaseEnvironment(VMHandle handle);

	template<typename T>
	T ValueHandle::get() {
		if (vm) {
			cached = vm->GetReturn(*this);
			vm = nullptr;
			return cached.as<T>();
		}
		return cached.as<T>();
	}

	template<typename ...Args> requires (std::is_convertible_v<Args, InternalValue> && ...)
		ValueHandle CallFunction(VMHandle* vm, FunctionHandle handle, Args... args) {
		std::vector<InternalValue> params = { InternalValue(args)... };
		return vm->_internal_call(handle, params.size(), params.data());
	}

	template<typename ...Args> requires (std::is_convertible_v<Args, InternalValue> && ...)
		ValueHandle FunctionHandle::operator()(Args... args) {
		return CallFunction(vm, *this, args...);
	}

}

#endif // !_EMI_INC_GUARD_HPP