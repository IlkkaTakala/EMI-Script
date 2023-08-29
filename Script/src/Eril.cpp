#include "Eril/Eril.hpp"
#include "Core.h"

using namespace Eril;

VMHandle CreateEnvironment()
{
	uint idx = CreateVM();

	VMHandle handle(idx);

	return handle;
}

Eril::VMHandle::VMHandle(unsigned int value) : Index(value)
{

}

void Eril::VMHandle::ReleaseVM()
{
	::ReleaseVM(Index);
	Index = 0;
}

void Eril::ReleaseVM(VMHandle handle)
{
	handle.ReleaseVM();
}
