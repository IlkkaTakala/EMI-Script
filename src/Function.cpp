#include "Function.h"

// @todo: Fix this, doesn't work with sets!!!
void ScriptFunction::Append(ScriptFunction fn)
{
	if (ArgCount != 0 || IsPublic != fn.IsPublic) {
		gCompileWarn() << "Cannot append functions";
		return;
	}

	for (size_t i = 0; i < fn.Bytecode.size(); i++) {
		auto ins = reinterpret_cast<Instruction*>(&fn.Bytecode[i]);

		switch (ins->code)
		{
		case OpCodes::LoadString: {
			ins->param += (uint16_t)StringTable.size();
		} break;
		case OpCodes::LoadNumber: {
			ins->param += (uint16_t)NumberTable.size();
		} break;
		case OpCodes::LoadSymbol: {
			ins->param += (uint16_t)GlobalTableSymbols.size();
		} break;
		case OpCodes::StoreSymbol: {
			ins->param += (uint16_t)GlobalTableSymbols.size();
		} break;
		case OpCodes::LoadProperty: {
			ins = reinterpret_cast<Instruction*>(&fn.Bytecode[++i]);
			ins->param += (uint16_t)PropertyTableSymbols.size();
		} break;
		case OpCodes::StoreProperty: {
			ins = reinterpret_cast<Instruction*>(&fn.Bytecode[++i]);
			ins->param += (uint16_t)PropertyTableSymbols.size();
		} break;
		case OpCodes::Jump: {
			ins->param += (uint16_t)Bytecode.size();
		} break;
		case OpCodes::PushObjectDefault: {
			ins->param += (uint16_t)TypeTableSymbols.size();
		} break;
		case OpCodes::InitObject: {
			ins = reinterpret_cast<Instruction*>(&fn.Bytecode[++i]);
			ins->param += (uint16_t)TypeTableSymbols.size();
		} break;
		case OpCodes::CallFunction: {
			ins = reinterpret_cast<Instruction*>(&fn.Bytecode[++i]);
			ins->data += (uint16_t)FunctionTableSymbols.size();
		} break;
		default:
			break;
		}
	}

	NumberTable.insert(fn.NumberTable.begin(), fn.NumberTable.end());
	StringTable.insert(StringTable.end(), fn.StringTable.begin(), fn.StringTable.end());
	FunctionTableSymbols.insert(FunctionTableSymbols.end(), fn.FunctionTableSymbols.begin(), fn.FunctionTableSymbols.end());
	PropertyTableSymbols.insert(PropertyTableSymbols.end(), fn.PropertyTableSymbols.begin(), fn.PropertyTableSymbols.end());
	TypeTableSymbols.insert(TypeTableSymbols.end(), fn.TypeTableSymbols.begin(), fn.TypeTableSymbols.end());
	GlobalTableSymbols.insert(GlobalTableSymbols.end(), fn.GlobalTableSymbols.begin(), fn.GlobalTableSymbols.end());

	FunctionTable.resize(FunctionTableSymbols.size());
	GlobalTable.resize(GlobalTableSymbols.size());
	PropertyTable.resize(PropertyTableSymbols.size());
	TypeTable.resize(TypeTableSymbols.size());

	DebugLines.clear();

	RegisterCount = std::max(RegisterCount, fn.RegisterCount);
	Bytecode.insert(Bytecode.end(), fn.Bytecode.begin(), fn.Bytecode.end());
}

FunctionSymbol::~FunctionSymbol()
{
	if (Type == FunctionType::User) {
		delete Local;
	}
}

FunctionSymbol* FunctionTable::GetFirstFitting(int args)
{
	if (auto it = Functions.find(args); it != Functions.end()) {
		return it->second;
	}
	for (auto& [count, fnSym] : Functions) {
		if (count > args) {
			return fnSym;
		}
	}
	return nullptr;
}

void FunctionTable::AddFunction(int args, FunctionSymbol* symbol)
{
	if (auto it = Functions.find(args); it != Functions.end()) {
		it->second->Next = symbol;
		symbol->Previous = it->second;
		it->second = symbol;
	}
	else {
		Functions[args] = symbol;
	}
}
