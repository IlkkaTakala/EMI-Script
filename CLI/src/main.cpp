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
	std::unordered_map<std::string, std::function<void(Eril::CompileOptions&)>> commandTable = {
		{"exit", [](Eril::CompileOptions&) { exit(0); }},
		{"-s", [](Eril::CompileOptions& options) { options.Simplify = true; }},
	};

	auto vm = Eril::CreateEnvironment();

	bool run = true;
	std::string input;
	while (run) {
		std::getline(std::cin, input);

		auto res = split(input, ' ');

		Eril::CompileOptions options;
		for (auto& s : res) {
			if (auto it = commandTable.find(s); it != commandTable.end()) {
				it->second(options);
			}
			else {
				options.Path = s;
			}
		}

		vm.CompileScript(options.Path.c_str(), options);

		printf("\n\n");
	}

	std::this_thread::sleep_for(10ms);
	Eril::ReleaseEnvironment(vm);

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