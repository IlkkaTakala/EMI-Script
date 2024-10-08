#pragma once
#include <variant>
#include "ParseHelper.h"
#include "Namespace.h"
#include "Function.h"

using NodeDataType = std::variant<std::string, double, bool>;

class Node
{
public:
	Node() :
		varType(VariableType::Undefined),
		regTarget(254),
		line(0),
		depth(0),
		sym(nullptr),
		instruction(0)
	{}
	~Node() {
		for (auto& c : children) {
			if (c != this)
				delete c;
		}
	}
	VariableType varType;
	Token type = Token::None;
	uint8_t regTarget;
	size_t line;
	size_t depth;
	NodeDataType data;
	std::vector<Node*> children;
	CompileSymbol* sym;
	size_t instruction;

	Variable ToVariable() const;

#ifdef DEBUG
	void print(const std::string& prefix, bool isLast = false);
#endif
};

class VM;
class ASTWalker
{
public:
	ASTWalker(VM*, Node*);
	~ASTWalker();
	void Run();
	SymbolTable Global;
	Function* InitFunction;
	bool HasError;
private:
	void WalkLoad(Node*);
	uint8_t WalkStore(Node*);
	CompileSymbol* FindLocalSymbol(const TName& name);
	CompileSymbol* FindOrCreateLocalSymbol(const TName& name);
	std::pair<TName, Symbol*> FindSymbol(const TNameQuery& name);
	std::pair<TName, Symbol*> FindSymbol(const TName& name);
	std::pair<TName, Symbol*> FindOrCreateSymbol(const TName& name, SymbolType type = SymbolType::None);

	void HandleFunction(Node* n, Function* f, CompileSymbol* s);
	void HandleInit();
	void HandleObject(Node* n);

	bool HasDebug;
	VM* Vm;
	Node* Root;
	std::vector<TName> SearchPaths;
	std::vector<std::pair<size_t, TName>> AllSearchPaths;

	// Function parsing
	Scoped* CurrentScope;
	Function* CurrentFunction;
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
