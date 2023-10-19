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
	Parser::ReleaseParser();
}

ScriptHandle VM::Compile(const char* path, const CompileOptions& options)
{
	ScriptHandle handle;
	{
		std::lock_guard lock(HandleMutex);
		handle = (ScriptHandle)++HandleCounter;
	}

	CompileOptions fulloptions = options;
	fulloptions.Handle = handle;
	fulloptions.Path = path;

	{
		std::lock_guard lock(CompileMutex);
		CompileQueue.push(fulloptions);
	}
	QueueNotify.notify_one();

	return handle;
}
