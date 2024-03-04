#include <EMI/EMI.h>
#include <thread>
#include <chrono>
#include <numeric>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <functional>

using namespace std::literals::chrono_literals;

template <typename Out>
void split(const std::string& s, char delim, Out result) {
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim)) {
		*result++ = item;
	}
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, std::back_inserter(elems));
	return elems;
}

int print(const char* number) {
	std::cout << number << '\n';

	return 100;
}

EMI_REGISTER(Global, printStr, print);

int main()
{
	srand(time(0));
	std::unordered_map<std::string, std::function<int(EMI::VMHandle&, const std::vector<std::string>&)>> commandTable = {
		{"exit", [](EMI::VMHandle& , const std::vector<std::string>&) { exit(0); return 0; }},
		{"compile", [](EMI::VMHandle& vm, const std::vector<std::string>& params) {
			static std::unordered_map<std::string, std::function<void(EMI::Options&)>> optionMap = {
				//{"-s", [](EMI::Options& options) { options.Simplify = true; }}
			};
			std::vector<std::string> path;
			EMI::Options options;
			for (int i = 1; i < params.size(); i++) {
				if (auto it = optionMap.find(params[i]); it != optionMap.end()) {
					it->second(options);
				}
				else {
					path.push_back(params[i]);
				}
			}
			int result = 0;
			for (const auto& p : path) 
				result += vm.CompileScript(p.c_str(), options); 
			return result > 0 ? 1 : 0;
		}},
		{"reinit", [](EMI::VMHandle& vm, const std::vector<std::string>&) { vm.ReinitializeGrammar("../../.grammar"); return 0; }},
		{"emi", [](EMI::VMHandle&, const std::vector<std::string>&) { return 2; }},
		{"loglevel", [](EMI::VMHandle&, const std::vector<std::string>& params) { if (params.size() > 1) EMI::SetLogLevel(std::atoi(params[1].c_str())); return 0; }},
	};

	auto vm = EMI::CreateEnvironment();
	vm.ReinitializeGrammar("../../.grammar");

	bool run = true;
	std::string input;
	while (run) {
		std::getline(std::cin, input);

		auto res = split(input, ' ');

		if (res.size() < 1) continue;

		int result = 0;
		if (auto it = commandTable.find(res[0]); it != commandTable.end()) {
			result = it->second(vm, res);
		}

		printf("\n\n");
		if (result == 0) continue;

		switch (result)
		{
		case 1: {
			bool runScriptMode = true;
			while (runScriptMode)
			{
				printf(">> ");
				std::getline(std::cin, input);
				if (input == "quit") {
					runScriptMode = false;
					break;
				}
				res = split(input, ' ');
				if (res.size() > 0) {
					EMI::FunctionHandle h = vm.GetFunctionHandle(res[0].c_str());
					auto handle = h(res.size() > 1 ? res[1].c_str() : "10");
					/*if (handle.get<const char*>()) 
						std::cout << handle.get<const char*>() << '\n';*/
				}
			}
			vm.Interrupt();
			printf("Exiting script stage\n\n");
		} break;

		case 2: {
			printf("Running in scripting mode\n");
			bool runScriptMode = true;
			while (runScriptMode)
			{
				printf(">> ");
				std::getline(std::cin, input);
				if (input == "quit") {
					runScriptMode = false;
					break;
				}

				vm.CompileTemporary(input.c_str());
			}
			printf("Exiting script stage\n\n");
		} break;
		default:
			break;
		}
		
	}

	EMI::ReleaseEnvironment(vm);
	std::this_thread::sleep_for(10ms);

	/*srand(time(NULL));

	int count = 4096;

	char* test = new char[count]();

	for (int i = 0; i < count; i++) {
		test[i] = rand() % 128;
	}

	std::vector<float> stdTimes;
	std::vector<bool> stdRes;
	stdTimes.reserve(100);
	stdRes.reserve(100);

	for (int i = 0; i < 100; i++) {
		auto start = std::chrono::steady_clock::now();

		for (int j = 0; j < count; j++)
			stdRes.push_back(isalnum((unsigned char)test[j]));

		std::chrono::duration<float> dur = std::chrono::steady_clock::now() - start;
		stdTimes.push_back(dur.count());
	}

	float total = std::reduce(stdTimes.begin(), stdTimes.end());
	total /= 100;

	std::cout << "Average time was: " << total;*/
}

