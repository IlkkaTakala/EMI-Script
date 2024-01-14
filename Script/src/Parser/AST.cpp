#include "AST.h"
#include "NativeFuncs.h"
#include "Helpers.h"
#include <iostream>

constexpr unsigned char first[] = { 195, 196, 196 };
constexpr unsigned char last[] = { 192, 196, 196 };
constexpr unsigned char add[] = { 179, 32, 32, 32, 0 };

std::string VariantToStr(Node* n) {
	switch (n->data.index())
	{
	case 0: return std::get<std::string>(n->data);
	case 1: return FloatToStr(std::get<double>(n->data));
	case 2: return BoolToStr(std::get<bool>(n->data));
	default:
		return "";
		break;
	}
}

int VariantToInt(Node* n) {
	switch (n->data.index())
	{
	case 0: return StrToInt(std::get<std::string>(n->data).c_str());
	case 1: return FloatToInt(std::get<double>(n->data));
	case 2: return BoolToInt(std::get<bool>(n->data));
	default:
		return 0;
		break;
	}
}

double VariantToFloat(Node* n) {
	switch (n->data.index())
	{
	case 0: return StrToFloat(std::get<std::string>(n->data).c_str());
	case 1: return (std::get<double>(n->data));
	case 2: return BoolToFloat(std::get<bool>(n->data));
	default:
		return 0.f;
		break;
	}
}

bool VariantToBool(Node* n) {
	switch (n->data.index())
	{
	case 0: return StrToBool(std::get<std::string>(n->data).c_str());
	case 1: return FloatToBool(std::get<double>(n->data));
	case 2: return (std::get<bool>(n->data));
	default:
		return false;
		break;
	}
}

Variable Node::toVariable() const
{
	Variable v;
	switch (data.index())
	{
	case 0: v = std::get<0>(data).c_str(); break;
	case 1: v = std::get<1>(data); break;
	case 2: v = std::get<2>(data); break;
	default:
		break;
	}
	moveOwnershipToVM(v);
	return v;
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
	case Number: {
		n->data = VariantToFloat(l) + VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) || VariantToBool(r);
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
	case Number: {
		n->data = VariantToFloat(l) - VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) && VariantToBool(r);
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
	case Number: {
		n->data = VariantToFloat(l) * VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) || VariantToBool(r);
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
	case Number: {
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
	case Number: {
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
		if (n->children.empty()) break;
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

		auto i = n->children.begin();
		while (i != n->children.end())
		{
			if (IsConstant((*i)->type))
			{
				n->children.erase(i++);
			}
			++i;
		}

	} break;

	case Static: {
		if (n->children.size() > 0) {
			if (!IsConstant(n->children.front()->type)) {
				delete n;
				n = nullptr;
				return false;
			}
		}
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

void TypeConverter(Node* n, const TokenHolder& h)
{
	switch (h.token)
	{
	case Number:
		n->data = 0.f;
		std::from_chars(h.data.data(), h.data.data() + h.data.size(), std::get<double>(n->data));
		break;
	case True:
		n->data = true;
		break;
	case False:
		n->data = false;
		break;
	default:
		std::get<std::string>(n->data) = h.data;
		break;
	}
}

ASTWalker::ASTWalker(VM*, Node* n)
{
	root = n;
	auto it = namespaces.emplace("Global", Namespace{});
	currentNamespace = &it.first->second;
	currentFunction = nullptr;
	currentScope = nullptr;
}

void ASTWalker::Run()
{
	for (auto& c : root->children) {
		switch (c->type)
		{
		case NamespaceDef:
		{
			currentNamespace = &namespaces[std::get<0>(c->data)];
		} break;
		
		case ObjectDef:
		{
			auto& data = std::get<0>(c->data);
			auto s = findSymbol(data);
			if (!s) {
				auto it = currentNamespace->symbols.emplace(data, Symbol{});
				auto& symbol = it.first->second;
				symbol.setType(SymbolType::Object);
				auto& object = currentNamespace->objects.emplace(data, UserDefined{}).first->second;
				
				for (auto& field : c->children) {
					Symbol flags;
					flags.flags = SymbolFlags::None;
					Variable var;

					if (field->children.size() == 2) {
						if (field->children.front()->type != AnyType) {
							flags.flags = SymbolFlags::Typed;
						}
						if (IsConstant(field->children.back()->type)) {
							if (TypesMatch(field->children.front()->type, field->children.back()->type)) {
								var = field->children.back()->toVariable(); // @todo: Add implicit conversions
							}
							else {
								gError() << "Trying to initialize with wrong type: In object " << data << ", field " << std::get<0>(field->data) << '\n';
							}
						}
						else if (field->children.back()->type == OExpr) {
							switch (field->children.front()->type)
							{
							case TypeNumber:
								var = Variable(0.0);
							default:
								break;
							}
						}
						else {
							gError() << "Trying to initialize to non-static: In object " << data << ", field " << std::get<0>(field->data) << ", line " << field->line << '\n';
						}
					} 
					else {
						gError() << "Parse error in object " << data << '\n';
					}
					
					object.AddField(std::get<0>(field->data), var, flags);
				}

				symbol.resolved = true;
			}
			else {
				gError() << "Line " << c->line << ": Symbol '" << data << "' already defined\n";
			}
		} break;
		
		case PublicFunctionDef:
		case FunctionDef:
		{
			auto& data = std::get<0>(c->data);
			auto s = findSymbol(data);
			if (!s) {
				auto it = currentNamespace->symbols.emplace(data, Symbol{});
				auto& symbol = it.first->second;
				symbol.setType(SymbolType::Function);
				Function* function = new Function();
				function->Name = data;
				currentNamespace->functions.emplace(function->Name.c_str(), function).first->second;
				
				handleFunction(c, function, symbol);
			}
			else {
				gError() << "Line " << c->line << ": Symbol '" << data << "' already defined\n";
			}
		} break;

		case Static:
		{
		} break;

		case Const:
		{
		} break;

		case VarDeclare:
		{
		} break;

		default:
			break;
		}
	}
}

#define _Op(code) f->Bytecode.push_back((uint8)OpCodes::code)
#define _N(n, func) case Token::n: {func} break;
#define _Walk for(auto& child : n->children) Walk(child);

void ASTWalker::Walk(Node* n)
{
	Function* f = currentFunction;
	switch (n->type) {
		_N(Number, {
			_Op(LoadNumber);
			auto res = f->numberTable.emplace(std::get<double>(n->data)); 
			uint16 idx = (uint16)std::distance(f->numberTable.begin(), res.first);
			auto t = f->Bytecode.size();
			f->Bytecode.resize(f->Bytecode.size() + 2);
			f->Bytecode[t] = *(uint8*)(&idx);
			}
		)

		_N(String,
			f->stringTable.emplace(std::get<std::string>(n->data));
			_Op(LoadString);
		)

		_N(Add,
			_Walk;
			_Op(NumAdd);
			)



		_N(Return,
			_Walk;
			if (n->children.empty()) _Op(PushUndefined);
			_Op(Return);
		)

	default:
		_Walk;
		break;
	}
}

bool ASTWalker::findSymbol(const std::string& name)
{
	if (namespaces.find(name) != namespaces.end()) return true;
	return currentNamespace->findSymbol(name);
}

void ASTWalker::handleFunction(Node* n, Function* f, Symbol& s)
{
	f->scope = new Scoped();
	currentFunction = f;

	for (auto& c : n->children) {
		switch (c->type)
		{
		case CallParams: {
			f->ArgCount = (uint8)c->children.size();
			for (auto& v : c->children) {
				auto symbol = f->scope->addSymbol(std::get<0>(v->data));
				symbol->setType(SymbolType::Variable);
				symbol->resolved = true;
			}
		} break;

		case Scope: {

			currentScope = f->scope;
			Walk(c);

		} break;

		default:
			gWarn() << "Something unexpected in function " << f->Name << '\n';
			break;
		}
	}

	s.resolved = true;
}
