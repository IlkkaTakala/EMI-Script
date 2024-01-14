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
	const Function* FunctionPtr;
	const uint8* Ptr;
	const uint8* End;
	size_t Location;
	uint16 StackOffset;
	std::vector<Variable> Arguments;
	std::promise<Variable> Return;

	CallObject(Function* function);
	CallObject(const CallObject& obj) = delete;
	CallObject operator=(const CallObject& obj) = delete;
};

template <typename T>
class stack
{
public:
	T pop() {
		if (top == 0) return T();
		top--;
		return _stack[top];
	}

	T& peek() {
		return _stack[top - 1];
	}

	void push(const T& v) {
		top++;
		if (_stack.size() <= top) _stack.push_back(v);
		else _stack[top - 1] = v;
	}

	void pop(size_t count) {
		if (top < count)
			top -= count;
		else
			top = 0;
	}

	void to(size_t i) {
		top = i;
	}

private:

	size_t top;
	std::vector<T> _stack;
};

class Runner
{
public:
	Runner(VM* vm);
	~Runner();

	void operator()();

private:
	VM* Owner;
	std::stack<CallObject*> CallStack;
	stack<Variable> Stack;
};

class VM
{
public:
	VM();
	~VM();

	void ReinitializeGrammar(const char* grammar);
	ScriptHandle Compile(const char* path, const Options& options);
	void CompileTemporary(const char* data);

	size_t GetFunctionID(const std::string& name);

	size_t CallFunction(FunctionHandle handle, const std::span<Variable>& args);
	Variable GetReturnValue(size_t index);

	void AddNamespace(ankerl::unordered_dense::map<std::string, Namespace>& space);

	inline bool IsRunning() const { return VMRunning; }
	//void Step();

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
	std::queue<CallObject*> CallQueue;
	std::vector<std::thread> RunnerPool;
	std::vector<std::future<Variable>> ReturnValues;
	std::vector<size_t> ReturnFreeList;
	bool VMRunning;

	ankerl::unordered_dense::map<uint32, Function*> FunctionMap;
	ankerl::unordered_dense::map<std::string, uint32> NameToFunctionMap;
	ankerl::unordered_dense::map<std::string, Namespace> Namespaces;
	std::stack<uint32> FunctionFreeList;
	uint32 FunctionHandleCounter = 0;


	ObjectManager Manager;
	std::vector<Variable> GlobalVariables;
	// Variable heap?
};