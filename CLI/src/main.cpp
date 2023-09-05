#include <Eril/Eril.hpp>
#include <thread>
#include <chrono>

using namespace std::literals::chrono_literals;

int main()
{
	auto vm = Eril::CreateEnvironment();

	vm.CompileScript("../../ScriptTest.ril");

	std::this_thread::sleep_for(10ms);
	Eril::ReleaseVM(vm);

}