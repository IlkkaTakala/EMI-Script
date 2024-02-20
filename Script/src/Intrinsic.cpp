#include "Intrinsic.h"
#include "Defines.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Helpers.h"
#include <numeric>

void print(Variable&, Variable* args, size_t argc) {
	if (argc > 0) {
		std::string format = toStdString(args[0]);
		std::vector<std::string> out_args(argc - 1);

		for (int i = 1; i < argc; ++i) {
			try {
				format = std::vformat(format, std::make_format_args(toStdString(args[i])));
			}
			catch (std::exception e) {
				gError() << "Exception: " << e.what() << "\n";
			}
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

void arraySize(Variable& out, Variable* args, size_t argc) {
	if (argc == 1 && args[0].getType() == VariableType::Array) {
		out = static_cast<double>(args[0].as<Array>()->size());
	}
}

void arrayPush(Variable&, Variable* args, size_t argc) {
	if (argc == 2 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.push_back(args[1]);

	}
}

void arrayPop(Variable&, Variable* args, size_t argc) {
	if (argc == 1 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.pop_back();
	}
}

void arrayRemove(Variable&, Variable* args, size_t argc) {
	if (argc == 2 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		std::erase(data, args[1]);
	}
}

void arrayRemoveIdx(Variable&, Variable* args, size_t argc) {
	if (argc == 2 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.erase(data.begin() + static_cast<size_t>(toNumber(args[1])));
	}
}

void arrayClear(Variable&, Variable* args, size_t argc) {
	if (argc == 1 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.clear();
	}
}










ankerl::unordered_dense::map<std::string, IntrinsicPtr> IntrinsicFunctions {
	{ "print", print },
	{ "println", printLn },
	{ "delay", delay },
	{ "Array.Size", arraySize },
	{ "Array.Push", arrayPush },
	{ "Array.Pop", arrayPop },
	{ "Array.Remove", arrayRemove },
	{ "Array.RemoveIndex", arrayRemoveIdx },
	{ "Array.Clear", arrayClear },

};

ankerl::unordered_dense::map<std::string, std::vector<VariableType>> IntrinsicFunctionTypes {
	{ "print", { VariableType::Undefined } },
	{ "println", { VariableType::Undefined } },
	{ "delay", { VariableType::Undefined, VariableType::Number } },
	{ "Array.Size", { VariableType::Number, VariableType::Array } },
	{ "Array.Push", { VariableType::Undefined, VariableType::Array, VariableType::Undefined } },
	{ "Array.Pop", { VariableType::Undefined, VariableType::Array} },
	{ "Array.Remove", { VariableType::Undefined, VariableType::Array, VariableType::Undefined } },
	{ "Array.RemoveIndex", { VariableType::Undefined, VariableType::Array, VariableType::Number } },
	{ "Array.Clear", { VariableType::Undefined, VariableType::Array } },
};