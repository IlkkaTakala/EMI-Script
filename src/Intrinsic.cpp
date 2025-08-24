#include "Intrinsic.h"
#include "Defines.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "Objects/UserObject.h"
#include "Helpers.h"
#include "Function.h"
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



auto AddFunction(const char* name, IntrinsicPtr fn, VariableType ret, std::vector<std::pair<const char*, VariableType>> args, bool hasReturn = false, bool anyargs = false) {
	auto sym = new Symbol{};
	sym->Flags = SymbolFlags::Typed;
	sym->Type = SymbolType::Function;
	sym->VarType = VariableType::Function;

	auto table = new FunctionTable();
	sym->Function = table;

	auto func = new FunctionSymbol();
	func->Type = FunctionType::Intrinsic;
	func->Signature.Return = ret;
	func->Signature.HasReturn = hasReturn;
	func->Signature.AnyNumArgs = anyargs;
	for (auto& [argname, type] : args) {
		func->Signature.Arguments.push_back(type);
		func->Signature.ArgumentNames.push_back(argname);
	}
	func->Intrinsic = fn;

	table->AddFunction((int)args.size(), func);

	return std::pair<PathType, Symbol*>{name, sym};
}
auto AddNamespace(const char* name) {
	auto sym = new Symbol{ SymbolType::Namespace, SymbolFlags::None, VariableType::Undefined, new Namespace{ name }, true };

	return std::pair<PathType, Symbol*>{name, sym};
}

// @todo: Something better for these intrinsic definitions
SymbolTable IntrinsicFunctions = { {
	AddFunction("print", print, VariableType::Undefined, { {"text", VariableType::String } }),
	AddFunction("println", printLn, VariableType::Undefined, { {"text", VariableType::String } }),

	AddFunction("delay", delay, VariableType::Undefined, { {"delay", VariableType::Number } }),

	AddNamespace("Array"),
	AddFunction("Array.Size", arraySize,				VariableType::Number,	 { {"array", VariableType::Array } }, true),
	AddFunction("Array.Resize", arrayResize,			VariableType::Number,	 { {"array", VariableType::Array }, { "new size", VariableType::Number}, { "fill", VariableType::Undefined}}, true),
	AddFunction("Array.Push", arrayPush,				VariableType::Undefined, { {"array", VariableType::Array }, { "value", VariableType::Undefined } }),
	AddFunction("Array.PushFront", arrayPushFront,		VariableType::Undefined, { {"array", VariableType::Array }, { "value", VariableType::Undefined } }),
	AddFunction("Array.PushUnique", arrayPushUnique,	VariableType::Number,	 { {"array", VariableType::Array }, { "value", VariableType::Undefined } }, true),
	AddFunction("Array.Pop", arrayPop,					VariableType::Undefined, { {"array", VariableType::Array } }),
	AddFunction("Array.Remove", arrayRemove,			VariableType::Undefined, { {"array", VariableType::Array }, { "value", VariableType::Undefined } }),
	AddFunction("Array.RemoveIndex", arrayRemoveIdx,	VariableType::Undefined, { {"array", VariableType::Array }, { "index", VariableType::Number } }),
	AddFunction("Array.Clear", arrayClear,				VariableType::Undefined, { {"array", VariableType::Array } } ),
	AddFunction("Array.Find", arrayFind,				VariableType::Number,	 { {"array", VariableType::Array } }, true),
	
	AddNamespace("Math"),
	AddFunction("Math.Sqrt", mathsqrt,					VariableType::Number,			{ { "value", VariableType::Number } }, true ),
	AddFunction("Copy", copy,							VariableType::Undefined,		{ { "value", VariableType::Undefined } }, true ),
} };