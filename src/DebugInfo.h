#pragma once
#include <string_view>
#include <filesystem>
#include <unordered_map>
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
		if (it == InstructionToLine.end()) return -1;
		return it->second;
	}

	struct Scope {
		int StartInstr = 0;
		int EndInstr = 0;
		std::unordered_map<std::string, DebugVariableInfo> Variables;
	};

	void AddScope(int startInstr, int endInstr, const std::unordered_map<std::string, DebugVariableInfo>& vars) {
		Scope s; s.StartInstr = startInstr; s.EndInstr = endInstr; s.Variables = vars;
		Scopes.push_back(std::move(s));
	}
	std::unordered_map<std::string, DebugVariableInfo> GetVisibleVariables(int instruction) const;

	void AddNamespace(std::string_view ns) { Namespaces.emplace_back(ns); }
	const std::vector<std::string>& GetNamespaces() const { return Namespaces; }

	DebugVariableInfo& AddVariable(std::string_view name, Symbol* sym, int reg);

	std::filesystem::path File;
	std::string Name;

private:
	std::unordered_map<int, int> InstructionToLine;
	std::vector<Scope> Scopes;
	std::vector<std::string> Namespaces;
};

struct QueryResult
{
	int LineNumber = -1;
	std::filesystem::path File;
	std::unordered_map<std::string, DebugVariableInfo> VariableInfo;
};

class DebugInfo
{
public:

	QueryResult Query(std::string_view fnName, int instruction);

	DebugFunctionInfo& EnsureFunction(std::string_view fnName) {
		return Functions[std::string(fnName)];
	}

private:
	std::unordered_map<std::string, DebugFunctionInfo> Functions;
};

