#include "EMI/EMI.h"
#include <filesystem>

int main()
{
	srand(time(0));
	auto vm = EMI::CreateEnvironment();
	EMI::SetLogLevel(4);
	auto result = vm.CompileScript("Scripts/game.ril");

	printf("Waiting for compile...\n");
	result.wait();
	printf("Compile done\n");
	
	EMI::FunctionHandle game = vm.GetFunctionHandle("game");
	EMI::FunctionHandle input = vm.GetFunctionHandle("input");
	EMI::FunctionHandle render = vm.GetFunctionHandle("renderFrame");
	
	auto res = game();
	input();
	render();

	res.get<bool>();

	EMI::ReleaseEnvironment(vm);
}