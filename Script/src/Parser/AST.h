#pragma once
#include <variant>
#include "ParseHelper.h"
#include "Namespace.h"

struct Node
{
	Node() : line(0), parent(nullptr) {}
	~Node() {
		for (auto& c : children) {
			delete c;
		}
	}
	Token type = None;
	size_t line;
	size_t depth;
	std::variant<std::string, double, bool> data;

	Variable toVariable() const;

	Node* parent;
	std::list<Node*> children;

	void print(const std::string& prefix, bool isLast = false);
};

class VM;
struct ASTWalker
{
	ASTWalker(VM*, Node*);
	void Run();
	void Walk(Node*);

	bool findSymbol(const std::string & name);

	void handleFunction(Node* n, Function& f, Symbol& s);

	Node* root;
	Namespace* currentNamespace;
	ankerl::unordered_dense::map<std::string, Namespace> namespaces;

	// Function parsing
	Scoped* currentScope;
	Function* currentFunction;
};

void TypeConverter(Node* n, const TokenHolder& h);
bool Optimize(Node*&);
void Desugar(Node*);
