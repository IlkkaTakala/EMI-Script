#include "Intrinsic.h"
#include "Defines.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Objects/UserObject.h"
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
	else {
		out.setUndefined();
	}
}

void arrayResize(Variable& out, Variable* args, size_t argc) {
	if (argc >= 2 && args[0].getType() == VariableType::Array) {
		size_t size = static_cast<size_t>(toNumber(args[1]));
		Variable fill;
		if (argc == 3) {
			fill = args[2];
		}
		args[0].as<Array>()->data().resize(size, fill);
		out = static_cast<double>(size);
	}
	else {
		out.setUndefined();
	}
}

void arrayPush(Variable&, Variable* args, size_t argc) {
	if (argc == 2 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.push_back(args[1]);
	}
}

void arrayPushFront(Variable&, Variable* args, size_t argc) {
	if (argc == 2 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.insert(data.begin(), args[1]);
	}
}

void arrayPushUnique(Variable& out, Variable* args, size_t argc) {
	if (argc == 2 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		if (auto it = std::find(data.begin(), data.end(), args[1]); it == data.end()) {
			data.push_back(args[1]);
			out = static_cast<int>(data.size() - 1);
		}
		else {
			out = static_cast<int>(it - data.begin());
		}
	}
	else {
		out.setUndefined();
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
		auto idx = toNumber(args[1]);
		if (idx < data.size())
			data.erase(data.begin() + static_cast<size_t>(idx));
	}
}

void arrayClear(Variable&, Variable* args, size_t argc) {
	if (argc == 1 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		data.clear();
	}
}

void arrayFind(Variable& out, Variable* args, size_t argc) {
	if (argc == 1 && args[0].getType() == VariableType::Array) {
		auto& data = args[0].as<Array>()->data();
		if (auto it = std::find(data.begin(), data.end(), args[1]); it != data.end()) {
			out = static_cast<int>(it - data.begin());
		}
		else {
			out = static_cast<int>(data.size());
		}
	}
	else {
		out.setUndefined();
	}
}


void copy(Variable& out, Variable* args, size_t argc) {
	if (argc == 1) {
		switch (args[0].getType())
		{
		case VariableType::Array: {
			auto& old = args[0].as<Array>()->data();
			auto next = Array::GetAllocator()->Make();
			next->data() = old;
			out = next;
		} break;

		case VariableType::String: {
			auto old = args[0].as<String>();
			auto next = String::GetAllocator()->Make(old->data(), old->size());
			out = next;
		} break;

		default:
			if (args[0].getType() > VariableType::Object) {
				auto obj = args[0].as<UserObject>();
				auto next = GetManager().Make(obj->Type);
				auto nextObj = next.as<UserObject>();

				for (uint16 i = 0; i < obj->size(); i++) {
					copy((*nextObj)[i], &(*obj)[i], 1);
				}
				out = next;
			}
			else {
				out = args[0];
			}
			break;
		}
	}
	else {
		out.setUndefined();
	}
}

void mathsqrt(Variable& out, Variable* args, size_t argc) {
	if (argc == 1) {
		out = sqrt(args[0].as<double>());
	}
}






ankerl::unordered_dense::map<std::string, IntrinsicPtr> IntrinsicFunctions {
	{ "print", print },
	{ "println", printLn },
	{ "delay", delay },
	{ "Array.Size", arraySize },
	{ "Array.Resize", arrayResize },
	{ "Array.Push", arrayPush },
	{ "Array.PushFront", arrayPushFront },
	{ "Array.PushUnique", arrayPushUnique },
	{ "Array.Pop", arrayPop },
	{ "Array.Remove", arrayRemove },
	{ "Array.RemoveIndex", arrayRemoveIdx },
	{ "Array.Clear", arrayClear },
	{ "Array.Find", arrayFind },
	{ "Math.Sqrt", mathsqrt },
	{ "Copy", copy },

};

ankerl::unordered_dense::map<std::string, std::vector<VariableType>> IntrinsicFunctionTypes {
	{ "print", { VariableType::Undefined } },
	{ "println", { VariableType::Undefined } },
	{ "delay", { VariableType::Undefined, VariableType::Number } },
	{ "Array.Size", { VariableType::Number, VariableType::Array } },
	{ "Array.Resize", { VariableType::Number, VariableType::Array, VariableType::Number, VariableType::Undefined } },
	{ "Array.Push", { VariableType::Undefined, VariableType::Array, VariableType::Undefined } },
	{ "Array.PushFront", { VariableType::Undefined, VariableType::Array, VariableType::Undefined } },
	{ "Array.PushUnique", { VariableType::Number, VariableType::Array, VariableType::Undefined } },
	{ "Array.Pop", { VariableType::Undefined, VariableType::Array} },
	{ "Array.Remove", { VariableType::Undefined, VariableType::Array, VariableType::Undefined } },
	{ "Array.RemoveIndex", { VariableType::Undefined, VariableType::Array, VariableType::Number } },
	{ "Array.Clear", { VariableType::Undefined, VariableType::Array } },
	{ "Array.Find", { VariableType::Number, VariableType::Array } },
	{ "Math.Sqrt", { VariableType::Number, VariableType::Number } },
	{ "Copy", { VariableType::Undefined, VariableType::Undefined } },
};

std::vector<std::string> DefaultNamespaces = {
	"Array",
	"System",
	"String",
	"Function",
	"Core",
	"Math",
};
