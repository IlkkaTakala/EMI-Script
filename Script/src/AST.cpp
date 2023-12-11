#include "AST.h"
#include "NativeFuncs.h"
#include <iostream>

constexpr unsigned char first[] = { 195, 196, 196 };
constexpr unsigned char last[] = { 192, 196, 196 };
constexpr unsigned char add[] = { 179, 32, 32, 32, 0 };

std::string VariantToStr(Node* n) {
	switch (n->data.index())
	{
	case 0: return std::get<std::string>(n->data);
	case 1: return IntToStr(std::get<int>(n->data));
	case 2: return FloatToStr(std::get<float>(n->data));
	case 3: return BoolToStr(std::get<bool>(n->data));
	default:
		return "";
		break;
	}
}

int VariantToInt(Node* n) {
	switch (n->data.index())
	{
	case 0: return StrToInt(std::get<std::string>(n->data).c_str());
	case 1: return std::get<int>(n->data);
	case 2: return FloatToInt(std::get<float>(n->data));
	case 3: return BoolToInt(std::get<bool>(n->data));
	default:
		return 0;
		break;
	}
}

float VariantToFloat(Node* n) {
	switch (n->data.index())
	{
	case 0: return StrToFloat(std::get<std::string>(n->data).c_str());
	case 1: return IntToFloat(std::get<int>(n->data));
	case 2: return (std::get<float>(n->data));
	case 3: return BoolToFloat(std::get<bool>(n->data));
	default:
		return 0.f;
		break;
	}
}

bool VariantToBool(Node* n) {
	switch (n->data.index())
	{
	case 0: return StrToBool(std::get<std::string>(n->data).c_str());
	case 1: return IntToBool(std::get<int>(n->data));
	case 2: return FloatToBool(std::get<float>(n->data));
	case 3: return (std::get<bool>(n->data));
	default:
		return false;
		break;
	}
}

void Node::print(const std::string& prefix, bool isLast)
{
	std::cout << prefix;
	std::cout.write(isLast ? (char*)last : (char*)first, 3);
	std::cout << TokensToName[type] << ": " << VariantToStr(this) << '\n';

	for (auto& node : children) {
		node->print(prefix + (isLast ? "    " : (char*)add), node == children.back());
	}
}

inline bool IsControl(Token t) {
	return t == Return || t == Break || t == Continue;
}

inline bool IsConstant(Token t) {
	return t >= String && t <= False;
}

inline bool IsOperator(Token t) {
	return t >= Equal && t <= Decrement;
}

void OpAdd(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case String: {
		n->data = VariantToStr(l) + VariantToStr(r);
	} break;
	case Integer: {
		n->data = VariantToInt(l) + VariantToInt(r);
	} break;
	case Float: {
		n->data = VariantToFloat(l) + VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) + VariantToBool(r);
	} break;
	default:
		break;
	}
}

void OpSub(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case String: {
		n->data = VariantToStr(l);// -VariantToStr(r); // TODO Remove substrings?
	} break;
	case Integer: {
		n->data = VariantToInt(l) - VariantToInt(r);
	} break;
	case Float: {
		n->data = VariantToFloat(l) - VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) - VariantToBool(r);
	} break;
	default:
		break;
	}
}

void OpMult(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case String: {
		n->data = VariantToStr(l);// -VariantToStr(r); // TODO Remove substrings?
	} break;
	case Integer: {
		n->data = VariantToInt(l) * VariantToInt(r);
	} break;
	case Float: {
		n->data = VariantToFloat(l) * VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) * VariantToBool(r);
	} break;
	default:
		break;
	}
}

void OpDiv(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case String: {
		n->data = VariantToStr(l);// -VariantToStr(r); // TODO Remove substrings?
	} break;
	case Integer: {
		n->data = VariantToInt(l) / VariantToInt(r);
	} break;
	case Float: {
		n->data = VariantToFloat(l) / VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) || VariantToBool(r);
	} break;
	default:
		break;
	}
}

void OpEqual(Node* n, Node* l, Node* r) {
	switch (l->type)
	{
	case String: {
		n->data = VariantToStr(l) == VariantToStr(r); // TODO Remove substrings?
	} break;
	case Integer: {
		n->data = VariantToInt(l) == VariantToInt(r);
	} break;
	case Float: {
		n->data = VariantToFloat(l) == VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) == VariantToBool(r);
	} break;
	default:
		break;
	}
	n->type = std::get<bool>(n->data) ? True : False;
}

bool Optimize(Node*& n)
{
	switch (n->type)
	{
	case Conditional: {
		if (n->children.size() == 3) {
			if (IsConstant(n->children.front()->type)) {
				auto old = n;
				n = *std::next(old->children.begin(), VariantToBool(n->children.front()) ? 1 : 2);
				old->children.remove(n);
				delete old;
			}
		}
	} break;

	case Stmt: {
		if (IsConstant(n->children.front()->type)) {
			delete n;
			n = nullptr;
			return false;
		}
	} break;

	case Scope: {
		bool hasControl = false;
		auto end = n->children.end();

		for (auto it = n->children.begin(); it != n->children.end(); ++it) {
			if (hasControl) {
				delete* it;
			}
			else {
				if (IsControl((*it)->type)) {
					hasControl = true;
					end = it;
					if (end != n->children.end()) end++;
				}
			}
		}
		n->children.erase(end, n->children.end());

	} break;

	default:
	if (IsOperator(n->type)) {
		if (n->children.size() < 1) return true;
		bool allConstant = true;
		Node* l = nullptr, *r = nullptr;
		for (auto& c : n->children) {
			if (!l) l = c; else r = c;
			if (!IsConstant(c->type)) {
				allConstant = false;
				break;
			}
		}
		if (allConstant) {
			switch (n->type)
			{
			case Add: OpAdd(n, l, r); break;
			case Sub: OpSub(n, l, r); break;
			case Mult: OpMult(n, l, r); break;
			case Div: OpDiv(n, l, r); break;
			case Equal: OpEqual(n, l, r); break;
			default:
				break;
			}

			for (auto& c : n->children) {
				delete c;
			}
			n->children.clear();
		}
	}

	break;
	}

	return true;
}
