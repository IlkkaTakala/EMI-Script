#pragma once
#include <variant>
#include "Lexer.h"

struct Node
{
	Node() {}
	~Node() {
		for (auto& c : children) {
			delete c;
		}
	}
	Token type = None;
	std::variant<std::string, double, bool> data;

	std::vector<Node*> parent;
	std::list<Node*> children;

	void print(const std::string& prefix, bool isLast = false);
};

bool Optimize(Node*&);