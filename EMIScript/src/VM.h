#pragma once
#include "Defines.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <stack>
#include <future>
#include <span>
#include "ankerl/unordered_dense.h"

#include "EMI/EMI.h"
#include "Variable.h"
#include "Function.h"
#include "Intrinsic.h"
#include "Namespace.h"
#include "Objects/UserObject.h"

using namespace EMI;

class VM;
struct CompileOptions
{
	std::string Path;
	std::string Data;
	Options UserOptions;
	std::promise<bool> CompileResult;

	CompileOptions() {}
	CompileOptions& operator=(CompileOptions&& in) noexcept {
		Path = in.Path;
		Data = in.Data;
		UserOptions = in.UserOptions;
		CompileResult = std::move(in.CompileResult);

		return *this;
	}
	CompileOptions(CompileOptions&& in) noexcept {
		Path = in.Path;
		Data = in.Data;
		UserOptions = in.UserOptions;
		CompileResult = std::move(in.CompileResult);
	}
};

struct CallObject 
{
	Function* FunctionPtr;
	const uint32_t* Ptr;
	const uint32_t* End;
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
		fast = &stack[top];
	}

	void destroy(size_t count) {
		for (size_t i = 0; i < count; i++) {
			fast[i].~T();
		}
	}

	void reserve(size_t count) {
		if (count > stack.size()) stack.resize(count);
		fast = &stack[top];
	}

	T& operator[](size_t location) {
		return fast[location];
	}

private:

	size_t top = 0;
	std::vector<T> stack;
	T* fast = nullptr;
};

class Runner
{
public:
	Runner(VM* vm);
	~Runner();
	void Join();

	void SetRunning(bool value) { Running = value; }

private:
	void Run();
	bool Running;
	VM* Owner;
	std::thread RunThread;
	std::vector<CallObject> CallStack;
	RegisterStack<Variable> Registers;
};

inline auto& HostFunctions() {
	static SymbolTable f;
	return f;
}

class VM
{
public:
	VM();
	~VM();

	void ReinitializeGrammar(const char* grammar);
	void* Compile(const char* path, const Options& options);
	void CompileTemporary(const char* data);
	void Interrupt();

	void* GetFunctionID(const std::string& name);

	size_t CallFunction(FunctionHandle handle, const std::span<InternalValue>& args);
	InternalValue GetReturnValue(size_t index);
	bool WaitForResult(void* ptr);

	std::pair<TName, Symbol*> FindSymbol(const TNameQuery& name);
	void AddNamespace(const std::string& path, const SymbolTable& space);

	inline bool IsRunning() const { return VMRunning; }

	void RemoveUnit(const std::string& unit);

private:
	friend class Parser;
	friend class Runner;

	void GarbageCollect();

	// When adding new compile targets
	std::mutex CompileMutex;
	// When merging compile results to VM
	std::mutex MergeMutex;
	// When editing call queue
	std::mutex CallMutex;

	std::list<std::future<bool>> CompileRequests;

	std::condition_variable QueueNotify;
	std::queue<CompileOptions> CompileQueue;
	std::vector<std::thread> ParserPool;
	std::thread* GarbageCollector;
	bool CompileRunning;
	bool VMRunning;

	std::condition_variable CallQueueNotify;
	std::queue<CallObject> CallQueue;
	std::vector<Runner*> RunnerPool;
	std::vector<std::future<Variable>> ReturnValues;
	std::vector<std::promise<Variable>> ReturnPromiseValues;
	std::vector<size_t> ReturnFreeList;

	ankerl::unordered_dense::map<std::string, CompileUnit> Units;
	ankerl::unordered_dense::set<Function*> ValidFunctions;
	SymbolTable GlobalSymbols;
};