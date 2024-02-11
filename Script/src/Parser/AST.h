#pragma once
#include <variant>
#include "ParseHelper.h"
#include "Namespace.h"

class Node
{
public:
	Node() :
		line(0),
		depth(0),
		regTarget(0),
		sym(nullptr),
		varType(VariableType::Undefined),
		instruction(0)
	{}
	~Node() {
		for (auto& c : children) {
			delete c;
		}
	}
	Token type = Token::None;
	size_t line;
	size_t depth;
	std::variant<std::string, double, bool> data;
	std::list<Node*> children;
	Symbol* sym;
	VariableType varType;
	uint8 regTarget;
	size_t instruction;

	Variable toVariable() const;

#ifdef DEBUG
	void print(const std::string& prefix, bool isLast = false);
#endif
};

class VM;
struct ASTWalker
{
	ASTWalker(VM*, Node*);
	~ASTWalker();
	void Run();
	void Walk(Node*);

	Symbol* findSymbol(const std::string& name, const std::string& space, bool& isNamespace);

	void handleFunction(Node* n, Function* f, Symbol* s);

	VM* vm;
	Node* root;
	Namespace* currentNamespace;
	ankerl::unordered_dense::map<std::string, Namespace> namespaces;

	bool HasError;
	bool HasDebug;

	// Function parsing
	Scoped* currentScope;
	Function* currentFunction;
	ankerl::unordered_dense::set<std::string> stringList;
	std::vector<Instruction> instructionList;
	std::array<bool, 255> registers;
	uint8 maxRegister;

	void initRegs() {
		for (auto& r : registers) r = false;
		maxRegister = 0;
	}

	uint8 getFirstFree() {
		for (uint8 i = 0; i < 255; i++) {
			if (!registers[i]) {
				registers[i] = true;
				if (maxRegister < i) maxRegister = i;
				return i;
			}
		}
		gError() << "No free registers, this shouldn't happen\n";
		HasError = true;
		return 0;
	}

	uint8 getLastFree() {
		uint8 idx = 0;
		for (uint8 i = 0; i < 255; i++) {
			if (registers[i]) {
				idx = i;
			}
		}
		idx++;
		registers[idx] = true;
		maxRegister = idx;
		return idx;
	}

	void freeReg(uint8 reg) {
		registers[reg] = false;
	}

};

void TypeConverter(Node* n, const TokenHolder& h);
bool Optimize(Node*&);
void Desugar(Node*&);
