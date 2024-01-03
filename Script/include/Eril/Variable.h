#pragma once
#include <variant>

class Variable
{
	std::variant<int64_t, double, const char*, uint32_t> value;

public:
	Variable() {}


	template<typename T>
	T as() {
		return T();
	}
};

