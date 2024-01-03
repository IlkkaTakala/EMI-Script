#include <Eril/Eril.hpp>
#include <thread>
#include <chrono>
#include <numeric>
#include <iostream>
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

int main()
{
	std::unordered_map<std::string, std::function<bool(Eril::VMHandle&, const std::vector<std::string>&)>> commandTable = {
		{"exit", [](Eril::VMHandle& vm, const std::vector<std::string>&) { exit(0); return false; }},
		{"compile", [](Eril::VMHandle& vm, const std::vector<std::string>& params) {
			static std::unordered_map<std::string, std::function<void(Eril::Options&)>> optionMap = {
				{"-s", [](Eril::Options& options) { options.Simplify = true; }}
			};
			std::vector<std::string> path;
			Eril::Options options;
			for (int i = 1; i < params.size(); i++) {
				if (auto it = optionMap.find(params[i]); it != optionMap.end()) {
					it->second(options);
				}
				else {
					path.push_back(params[i]);
				}
			}
			bool result = false;
			for (const auto& p : path) 
				result |= (bool)vm.CompileScript(p.c_str(), options); 
			return result;
		}},
		{"reinit", [](Eril::VMHandle& vm, const std::vector<std::string>& params) { vm.ReinitializeGrammar("../../.grammar"); return false; }},
	};

	auto vm = Eril::CreateEnvironment();
	vm.ReinitializeGrammar("../../.grammar");

	bool run = true;
	std::string input;
	while (run) {
		std::getline(std::cin, input);

		auto res = split(input, ' ');

		if (res.size() < 1) continue;

		bool result = false;
		if (auto it = commandTable.find(res[0]); it != commandTable.end()) {
			result = it->second(vm, res);
		}

		printf("\n\n");
		if (!result) continue;

		bool runScriptMode = true;
		while (runScriptMode)
		{
			printf(">> ");
			std::getline(std::cin, input);
			if (input == "quit") {
				runScriptMode = false;
				break;
			}

			Eril::FunctionHandle h = vm.GetFunctionHandle("test");
			auto handle = h(10, 5423.453, "Test", true, &input, 34.f);
			std::cout << handle.get<int>();
		}
		printf("Exiting script stage\n\n");
	}

	Eril::ReleaseEnvironment(vm);
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