#pragma once
#include <string_view>
#include <filesystem>
#include <unordered_map>
#include <map>
#include <vector>
#include <string>
#include "Symbol.h"

struct DebugVariableInfo
{
	std::string Name;
	SymbolFlags Flags = SymbolFlags::None;
	VariableType Type = VariableType::Undefined;
	Symbol* Sym = nullptr;
	int Register = -1;
};

class DebugFunctionInfo
{
public:
	DebugFunctionInfo() = default;
	DebugFunctionInfo(std::string_view name) : Name(name) {}

	void AddInstructionLine(int instruction, int line) { InstructionToLine[instruction] = line; }
	int GetLineForInstruction(int instruction) const {
		auto it = InstructionToLine.find(instruction);
		if (it == InstructionToLine.end()) return 0;
		return it->second;
	}
	int GetInstructionForLine(int line) const {
		if (InstructionToLine.empty() || line < InstructionToLine.begin()->second) return -1;
		for (const auto& kv : InstructionToLine) {
			if (kv.second >= line) return (int)kv.first;
		}
		return -1;
	}

	struct Scope {
		size_t StartInstr = 0;
		size_t EndInstr = 0;
		std::unordered_map<std::string, DebugVariableInfo> Variables;
	};

	int AddScope(size_t startInstr) {
		Scope s; s.StartInstr = startInstr;
		Scopes.push_back(std::move(s));
		return static_cast<int>(Scopes.size() - 1);
	}
	Scope* GetScope(int index) {
		if (index < 0 || index >= Scopes.size()) return nullptr;
		return &Scopes[index];
	}

	std::unordered_map<std::string, DebugVariableInfo> GetVisibleVariables(size_t instruction) const;

	void AddNamespace(std::string_view ns) { Namespaces.emplace_back(ns); }
	const std::vector<std::string>& GetNamespaces() const { return Namespaces; }

	DebugVariableInfo& AddVariable(const std::string& name, Symbol* sym, int reg);

	std::filesystem::path File;
	std::string Name;

private:
	std::map<int, int> InstructionToLine;
	std::vector<Scope> Scopes;
	std::vector<std::string> Namespaces;
};

struct QueryResult
{
	size_t LineNumber = 0;
	std::filesystem::path File;
	std::unordered_map<std::string, DebugVariableInfo> VariableInfo;
};

class DebugInfo
{
public:

	QueryResult Query(PathType fnName, size_t instruction);
	size_t QueryLine(PathType fnName, size_t instruction);

	DebugFunctionInfo* AddFunction(const std::string& fnName) {
		DebugFunctionInfo fn;
		fn.Name = fnName;
		return &Functions[""].emplace(fnName, fn).first->second;
	}

	DebugFunctionInfo* GetFunction(const PathType& fnName) {
		for (auto& [name, fns] : Functions) {
			auto it = fns.find(fnName.toString());
			if (it != fns.end())
				return &it->second;
		}
		return nullptr;
	}

	void AddInfo(const std::string& path, const DebugInfo& info) {
		for (const auto& [fnName, fnInfo] : info.Functions.at("")) {
			Functions[path].emplace(fnName, fnInfo);
		}
	}

	void RemoveInfo(const std::string& path) {
		Functions.erase(path);
	}

private:
	std::unordered_map<std::string, std::unordered_map<std::string, DebugFunctionInfo>> Functions;
};

