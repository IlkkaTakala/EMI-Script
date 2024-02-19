#include "Intrinsic.h"
#include "Defines.h"
#include "Objects/StringObject.h"
#include "Helpers.h"
#include <numeric>

void print(Variable&, Variable* args, size_t argc) {
	if (argc > 0) {
		std::string format = toStdString(args[0]);
		std::vector<std::string> out_args(argc - 1);

		for (int i = 1; i < argc; ++i) {
			format = std::vformat(format, std::make_format_args(toStdString(args[i])));
		}

		gLogger() << format;
	}
}

void printLn(Variable& out, Variable* args, size_t argc) {
	if (argc > 0) {
		print(out, args, argc);
		gLogger() << '\n';
	}
}

void delay(Variable&, Variable* args, size_t argc) {
	if (argc > 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds((size_t)toNumber(args[0])));
	}
}











ankerl::unordered_dense::map<std::string, IntrinsicPtr> IntrinsicFunctions {
	{ "print", print },
	{ "println", printLn },
	{ "delay", delay },
};

ankerl::unordered_dense::map<std::string, std::vector<VariableType>> IntrinsicFunctionTypes {
	{ "print", { VariableType::Undefined } },
	{ "println", { VariableType::Undefined } },
	{ "delay", { VariableType::Undefined, VariableType::Number } },
};