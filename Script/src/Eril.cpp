#include "Eril/Eril.hpp"
#include "Core.h"
#include "VM.h"

using namespace Eril;

VMHandle Eril::CreateEnvironment()
{
	uint idx = CreateVM();
	return VMHandle(idx);
}

Eril::VMHandle::VMHandle(unsigned int value) : Index(value)
{

}

ScriptHandle Eril::VMHandle::CompileScript(const char* file)
{
	auto vm = GetVM(Index);
	return vm->Compile(file);
}

void Eril::VMHandle::ReleaseVM()
{
	::ReleaseVM(Index);
	Index = 0;
}

void Eril::ReleaseEnvironment(VMHandle handle)
{
	handle.ReleaseVM();
}
