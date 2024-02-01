#pragma once
#include "Defines.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stack>
#include <future>
#include <span>
#include "ankerl/unordered_dense.h"

#include "Eril/Eril.hpp"
#include "Eril/Variable.h"
#include "Function.h"
#include "Object.h"
#include "Namespace.h"

using namespace Eril;

class VM;
struct CompileOptions
{
	std::string Path;
	std::string Data;
	ScriptHandle Handle = 0;
	Options UserOptions;
};

struct CallObject 
{
	Function* FunctionPtr;
	const uint32* Ptr;
	const uint32* End;
	size_t StackOffset;
	size_t PromiseIndex;
	std::vector<Variable> Arguments;

	CallObject(Function* function);
};

template <typename T>
class RegisterStack
{
public:
	void to(size_t location) {
		top = location;
		fast = &_stack[top];
	}

	void reserve(size_t count) {
		_stack.resize(count);
		fast = &_stack[top];
	}

	T& operator[](size_t location) {
		return fast[location];
	}

private:

	size_t top = 0;
	std::vector<T> _stack;
	T* fast = nullptr;
};

class Runner
{
public:
	Runner(VM* vm);
	~Runner();

	void operator()();

private:
	VM* Owner;
	std::vector<CallObject> CallStack;
	RegisterStack<Variable> Registers;
};

inline auto& HostFunctions() {
	static ankerl::unordered_dense::map<std::string, ankerl::unordered_dense::map<std::string, __internal_function*>> f;
	return f;
};

inline auto& ValidHostFunctions() {
	static ankerl::unordered_dense::set<uint64_t> f;
	return f;
}

class VM
{
public:
	VM();
	~VM();

	void ReinitializeGrammar(const char* grammar);
	ScriptHandle Compile(const char* path, const Options& options);
	void CompileTemporary(const char* data);

	void* GetFunctionID(const std::string& name);

	size_t CallFunction(FunctionHandle handle, const std::span<Variable>& args);
	Variable GetReturnValue(size_t index);

	void AddNamespace(const std::string& path, ankerl::unordered_dense::map<std::string, Namespace>& space);

	inline bool IsRunning() const { return VMRunning; }
	//void Step();

	void RemoveUnit(const std::string& unit);

private:
	friend class Parser;
	friend class Runner;

	// When adding new compile targets
	std::mutex CompileMutex;
	// When merging compile results to VM
	std::mutex MergeMutex;
	// When getting new handles
	std::mutex HandleMutex;
	// When editing call queue
	std::mutex CallMutex;

	std::condition_variable QueueNotify;
	std::queue<CompileOptions> CompileQueue;
	std::vector<std::thread> ParserPool;
	size_t HandleCounter = 0;
	bool CompileRunning;

	std::condition_variable CallQueueNotify;
	std::queue<CallObject> CallQueue;
	std::vector<std::thread> RunnerPool;
	std::vector<std::future<Variable>> ReturnValues;
	std::vector<std::promise<Variable>> ReturnPromiseValues;
	std::vector<size_t> ReturnFreeList;
	bool VMRunning;

	ankerl::unordered_dense::map<std::string, CompileUnit> Units;
	ankerl::unordered_dense::set<Function*> ValidFunctions;
	ankerl::unordered_dense::map<std::string, Function*> NameToFunctionMap;
	ankerl::unordered_dense::map<std::string, Namespace> Namespaces;

	ObjectManager Manager;
	std::vector<Variable> GlobalVariables;
	// Variable heap?
};