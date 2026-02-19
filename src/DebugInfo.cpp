#include "DebugInfo.h"

QueryResult DebugInfo::Query(PathType fnName, size_t instruction)
{
	QueryResult res;
	auto fn = GetFunction(fnName);

	const DebugFunctionInfo& funcInfo = *fn;
	res.LineNumber = funcInfo.GetLineForInstruction((int)instruction);
	res.File = funcInfo.File;
	res.VariableInfo = funcInfo.GetVisibleVariables((int)instruction);
	return res;
}

size_t DebugInfo::QueryLine(PathType fnName, size_t instruction)
{
	auto fn = GetFunction(fnName);

	const DebugFunctionInfo& funcInfo = *fn;
	return funcInfo.GetLineForInstruction((int)instruction);;
}

std::unordered_map<std::string, DebugVariableInfo> DebugFunctionInfo::GetVisibleVariables(size_t instruction) const
{
	std::unordered_map<std::string, DebugVariableInfo> out;
	for (const auto& s : Scopes) {
		if (instruction >= s.StartInstr && instruction <= s.EndInstr) {
			for (const auto& kv : s.Variables) out.emplace(kv.first, kv.second);
		}
	}
	return out;
}

DebugVariableInfo& DebugFunctionInfo::AddVariable(const std::string& name, Symbol* sym, int reg)
{
	DebugVariableInfo info{ std::string(name), sym->Flags, sym->VarType, sym, reg };
	auto result = Scopes.rbegin()->Variables.emplace(name, info);
	return result.first->second;
}