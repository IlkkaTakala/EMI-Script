#pragma once
#include <variant>
#include "ParseHelper.h"
#include "Namespace.h"

using NodeDataType = std::variant<std::string, double, bool>;

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
			if (c != this)
				delete c;
		}
	}
	VariableType varType;
	Token type = Token::None;
	uint8 regTarget;
	size_t line;
	size_t depth;
	NodeDataType data;
	std::vector<Node*> children;
	Symbol* sym;
	size_t instruction;

	Variable toVariable() const;

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
	ankerl::unordered_dense::map<std::string, Namespace> namespaces;
	bool HasError;
private:
	void WalkLoad(Node*);
	void WalkStore(Node*, uint8 reg);
	Symbol* findSymbol(const std::string& name, const std::string& space, bool& isNamespace);

	void handleFunction(Node* n, Function* f, Symbol* s);

	bool HasDebug;
	VM* vm;
	Node* root;
	Namespace* currentNamespace;


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
		uint8 idx = 255;
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

	void placeBreaks(Node* n, size_t start, size_t end);
};

void TypeConverter(NodeDataType& n, const TokenHolder& h);
bool Optimize(Node*&);
void Desugar(Node*&);
