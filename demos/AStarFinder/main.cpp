#include "EMI/EMI.h"
#include <filesystem>

int main()
{
	srand(time(0));
	printf("Waiting for compile...\n");
	auto vm = EMI::CreateEnvironment();
	vm.ReinitializeGrammar("../../.grammar");
	EMI::SetLogLevel(EMI::LogLevel::Debug);
	auto result = vm.CompileScript("Scripts/game.ril");

	result.wait();
	printf("\nCompile done\n");
	
	EMI::FunctionHandle game = vm.GetFunctionHandle("game");
	EMI::FunctionHandle input = vm.GetFunctionHandle("input");
	EMI::FunctionHandle render = vm.GetFunctionHandle("renderFrame");
	
	auto res = game();
	input();
	render();

	res.get<bool>();

	EMI::ReleaseEnvironment(vm);
}