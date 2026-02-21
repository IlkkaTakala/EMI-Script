#pragma once
#include <variant>
#include "ParseHelper.h"
#include "Namespace.h"
#include "Function.h"
#include "Parser/Node.h"
#include "DebugInfo.h"
#include "EMI/EMI.h"

class VM;
class ASTWalker
{
public:
	ASTWalker(VM*, Node*, const std::string&, const EMI::Options&);
	~ASTWalker();
	void Run();
	SymbolTable Global;
	ScriptFunction* InitFunction;
	bool HasError;
	const DebugInfo& GetDebugInfo() const { return CurrentDebugInfo; }

private:
	void WalkLoad(Node*);
	uint8_t WalkStore(Node*);
	CompileSymbol* FindLocalSymbol(const PathType& name);
	CompileSymbol* FindOrCreateLocalSymbol(const PathType& name);
	std::pair<PathType, Symbol*> FindSymbol(const PathTypeQuery& name);
	std::pair<PathType, Symbol*> FindSymbol(const PathType& name);
	std::pair<PathType, Symbol*> FindOrCreateSymbol(const PathType& name, SymbolType type = SymbolType::None);

	void HandleFunction(Node* n, ScriptFunction* f, CompileSymbol* s);
	void HandleInit();
	void HandleObject(Node* n);

	// per-token handlers (used by WalkLoad jump table)
	void handle_default(Node* n);
	void handle_Scope(Node* n);
	void handle_TypeNumber(Node* n);
	void handle_TypeBoolean(Node* n);
	void handle_TypeString(Node* n);
	void handle_TypeArray(Node* n);
	void handle_TypeFunction(Node* n);
	void handle_AnyType(Node* n);
	void handle_Typename(Node* n);
	void handle_Number(Node* n);
	void handle_Literal(Node* n);
	void handle_Null(Node* n);
	void handle_Array(Node* n);
	void handle_Indexer(Node* n);
	void handle_True(Node* n);
	void handle_False(Node* n);
	void handle_Add(Node* n);
	void handle_Sub(Node* n);
	void handle_Div(Node* n);
	void handle_Mult(Node* n);
	void handle_Power(Node* n);
	void handle_Mod(Node* n);
	void handle_AssignAdd(Node* n);
	void handle_AssignSub(Node* n);
	void handle_AssignDiv(Node* n);
	void handle_AssignMult(Node* n);
	void handle_AssignPower(Node* n);
	void handle_AssignMod(Node* n);
	void handle_Id(Node* n);
	void handle_Property(Node* n);
	void handle_Negate(Node* n);
	void handle_Not(Node* n);
	void handle_Less(Node* n);
	void handle_LessEqual(Node* n);
	void handle_Larger(Node* n);
	void handle_LargerEqual(Node* n);
	void handle_Equal(Node* n);
	void handle_NotEqual(Node* n);
	void handle_And(Node* n);
	void handle_Or(Node* n);
	void handle_Increment(Node* n);
	void handle_PreIncrement(Node* n);
	void handle_Assign(Node* n);
	void handle_VarDeclare(Node* n);
	void handle_ObjectInit(Node* n);
	void handle_Return(Node* n);
	void handle_Conditional(Node* n);
	void handle_If(Node* n);
	void handle_For(Node* n);
	void handle_While(Node* n);
	void handle_Break(Node* n);
	void handle_Else(Node* n);
	void handle_FunctionCall(Node* n);

	void helper_Assign(Node* n, Node* last);

	void(ASTWalker::*TokenJumpTable[(unsigned long long)Token::Last])(Node*);

	std::string Filename;
	VM* Vm;
	Node* Root;
	std::vector<PathType> SearchPaths;
	std::vector<std::pair<size_t, PathType>> AllSearchPaths;

	// Function parsing
	bool HasDebug;
	DebugInfo CurrentDebugInfo;
	DebugFunctionInfo* CurrentDebugFunction;
	int CurrentDebugScope;
	size_t CurrentLine;
	std::vector<int> BreakPoints;
	EMI::Options CompileOptions;

	ScopeType* CurrentScope;
	ScriptFunction* CurrentFunction;
	ankerl::unordered_dense::set<std::string> StringList;
	std::vector<Instruction> InstructionList;
	std::array<bool, 256> Registers;
	uint8_t MaxRegister;

	void InitRegisters() {
		for (auto& r : Registers) r = false;
		MaxRegister = 0;
	}

	uint8_t GetFirstFree() {
		for (uint8_t i = 0; i < 255; i++) {
			if (!Registers[i]) {
				Registers[i] = true;
				if (MaxRegister < i) MaxRegister = i;
				return i;
			}
		}
		gCompileError() << "No free registers, this shouldn't happen";
		HasError = true;
		return 0;
	}

	uint8_t GetLastFree() {
		uint8_t idx = 0;
		for (uint8_t i = 0; i < 254; i++) {
			if (Registers[i]) {
				idx = i + 1;
			}
		}
		Registers[idx] = true;
		MaxRegister = idx;
		return idx;
	}

	void FreeRegister(uint8_t reg) {
		Registers[reg] = false;
	}

	void PlaceBreaks(Node* n, size_t start, size_t end);
};

void TypeConverter(NodeDataType& n, const TokenHolder& h);
bool Optimize(Node*&);
void Desugar(Node*&);
