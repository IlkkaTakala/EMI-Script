#include "Intrinsic.h"
#include "Defines.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Objects/UserObject.h"
#include "Helpers.h"
#include <numeric>
#include <math.h>
#include <thread>

void print(Variable&, Variable* args, size_t argc) {
	if (argc > 0) {
		std::string format = toStdString(args[0]);

		// @todo: Add print formatting

		gScriptLogger() << format;
	}
}

void printLn(Variable& out, Variable* args, size_t argc) {
	if (argc > 0) {
		print(out, args, argc);
		gScriptLogger() << '\n';
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

				for (uint16_t i = 0; i < obj->size(); i++) {
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



#define FUNC(named, fn, ret) { named##_name, new Symbol{ SymbolType::Function, SymbolFlags::Typed, VariableType::Function, new FunctionSymbol{FunctionType::Intrinsic, (void*)fn, Variable{},VariableType::ret, 
#define NAMESPACE(named) { named##_name, new Symbol{ SymbolType::Namespace, SymbolFlags::None, VariableType::Undefined, new Namespace{ named##_name }, true } },

// @todo: Something better for these intrinsic definitions
SymbolTable IntrinsicFunctions = { {
	FUNC("print", print,						Undefined)		{} }, true}},
	FUNC("println", printLn,					Undefined)		{} }, true}},
	FUNC("delay", delay,						Undefined)		{ VariableType::Number } }, true}},
	NAMESPACE("Array")
	FUNC("Array.Size", arraySize,				Number)			{ VariableType::Array } }, true}},
	FUNC("Array.Resize", arrayResize,			Number)			{ VariableType::Array, VariableType::Number, VariableType::Undefined } }, true}},
	FUNC("Array.Push", arrayPush,				Undefined)		{ VariableType::Array, VariableType::Undefined } }, true}},
	FUNC("Array.PushFront", arrayPushFront,		Undefined)		{ VariableType::Array, VariableType::Undefined } }, true}},
	FUNC("Array.PushUnique", arrayPushUnique,	Number)			{ VariableType::Array, VariableType::Undefined } }, true}},
	FUNC("Array.Pop", arrayPop,					Undefined)		{ VariableType::Array } }, true}},
	FUNC("Array.Remove", arrayRemove,			Undefined)		{ VariableType::Array, VariableType::Undefined } }, true}},
	FUNC("Array.RemoveIndex", arrayRemoveIdx,	Undefined)		{ VariableType::Array, VariableType::Number } }, true}},
	FUNC("Array.Clear", arrayClear,				Undefined)		{ VariableType::Array } }, true}},
	FUNC("Array.Find", arrayFind,				Number)			{ VariableType::Array } }, true}},
	NAMESPACE("Math")
	FUNC("Math.Sqrt", mathsqrt,					Number)			{ VariableType::Number } }, true}},
	FUNC("Copy", copy,							Undefined)		{ VariableType::Undefined } }, true}},
}};

#undef FUNC