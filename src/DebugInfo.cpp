#include "DebugInfo.h"

QueryResult DebugInfo::Query(std::string_view fnName, int instruction)
{
	QueryResult res;
	auto it = Functions.find(std::string(fnName));
	if (it == Functions.end()) return res;

	const DebugFunctionInfo& funcInfo = it->second;
	res.LineNumber = funcInfo.GetLineForInstruction(instruction);
	res.File = funcInfo.File;
	res.VariableInfo = funcInfo.GetVisibleVariables(instruction);
	return res;
}

std::unordered_map<std::string, DebugVariableInfo> DebugFunctionInfo::GetVisibleVariables(int instruction) const
{
	std::unordered_map<std::string, DebugVariableInfo> out;
	for (const auto& s : Scopes) {
		if (instruction >= s.StartInstr && instruction <= s.EndInstr) {
			for (const auto& kv : s.Variables) out.emplace(kv.first, kv.second);
		}
	}
	return out;
}

DebugVariableInfo& DebugFunctionInfo::AddVariable(std::string_view name, Symbol* sym, int reg)
{
	DebugVariableInfo info{ std::string(name), sym->Flags, sym->VarType, sym, reg };
	auto result = Scopes.rend()->Variables.emplace(std::string(name), info);
	return result.first->second;
}