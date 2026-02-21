#pragma once
#include <variant>
#include <string>
#include <vector>
#include "EMIDev/Variable.h"
#include "Defines.h"
#include "Lexer.h"

using NodeDataType = std::variant<std::string, double, bool>;

struct CompileSymbol;
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
	{
	}
	~Node() {
		for (auto& c : children) {
			if (c != this)
				delete c;
		}
	}

	// Use a pool for Node allocations: custom new/delete operators
	static void* operator new(size_t sz);
	static void operator delete(void* ptr) noexcept;

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

std::string VariantToStr(Node* n);
int VariantToInt(Node* n);
double VariantToFloat(Node* n);
bool VariantToBool(Node* n);