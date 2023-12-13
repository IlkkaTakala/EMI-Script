#pragma once
#include "Defines.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <stack>

#include "Function.h"

class VM;
struct CompileOptions
{
	std::string Path;
	ScriptHandle Handle = 0;
	Options UserOptions;
};

struct CallObject {
	const Function* function;
	size_t location;
};

class Runner
{
public:
	Runner(VM* vm);
	~Runner();

	void operator()(const Function& f);

private:
	VM* Owner;
	std::stack<CallObject> CallStack;
};

class VM
{
public:
	VM();
	~VM();

	void ReinitializeGrammar(const char* grammar);
	ScriptHandle Compile(const char* path, const Options& options);

	//void Step();
	//void CallFunction();

private:
	friend class Parser;

	// When adding new compile targets
	std::mutex CompileMutex;
	// When merging compile results to VM
	std::mutex MergeMutex;
	// When getting new handles
	std::mutex HandleMutex;

	std::condition_variable QueueNotify;
	std::queue<CompileOptions> CompileQueue;
	std::vector<std::thread> ParserPool;
	size_t HandleCounter = 0;
	bool CompileRunning;


	std::unordered_map<uint16, Function> FunctionMap;
	std::unordered_map<std::string, uint16> NameToFunctionMap;

	// Variable heap
};