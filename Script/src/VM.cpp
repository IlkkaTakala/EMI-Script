#include "VM.h"
#include "Parser.h"

VM::VM()
{
	Parser::InitializeParser();
	auto counter = std::thread::hardware_concurrency();
	CompileRunning = true;

	for (uint i = 0; i < counter; i++) {
		ParserPool.emplace_back(Parser::ThreadedParse, this);
	}
}

VM::~VM()
{
	CompileRunning = false;
	QueueNotify.notify_all();
	for (auto& t : ParserPool) {
		t.join();
	}
}

ScriptHandle VM::Compile(const char* path)
{
	ScriptHandle handle;
	{
		std::lock_guard lock(HandleMutex);
		handle = (ScriptHandle)++HandleCounter;
	}

	CompileOptions options{ handle, path };

	{
		std::lock_guard lock(CompileMutex);
		CompileQueue.push(options);
	}
	QueueNotify.notify_one();

	return handle;
}
