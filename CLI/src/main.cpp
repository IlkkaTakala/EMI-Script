#include <Eril/Eril.hpp>
#include <thread>
#include <chrono>
#include <numeric>
#include <iostream>

using namespace std::literals::chrono_literals;

inline bool is_digit(unsigned char c) {
	return c >= 48 && c <= 57;
}

inline bool is_alpha(unsigned char c) {
	return (c >= 97 && c <= 122) || (c >= 65 && c <= 90);
}

inline bool is_alnum(unsigned char c) {
	return is_alpha(c) || is_digit(c);
}

int main()
{
	auto vm = Eril::CreateEnvironment();

	vm.CompileScript("../../ScriptTest.ril");

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