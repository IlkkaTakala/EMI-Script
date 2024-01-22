#include "AST.h"
#include "NativeFuncs.h"
#include "Helpers.h"
#include <iostream>

constexpr unsigned char print_first[] = { 195, 196, 196 };
constexpr unsigned char print_last[] = { 192, 196, 196 };
constexpr unsigned char print_add[] = { 179, 32, 32, 32, 0 };

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
	std::cout.write(isLast ? (char*)print_last : (char*)print_first, 3);
	std::cout << TokensToName[type] << ": " << VariantToStr(this) << '\n';

	for (auto& node : children) {
		node->print(prefix + (isLast ? "    " : (char*)print_add), node == children.back());
	}
}

inline bool IsControl(Token t) {
	return t == Return || t == Break || t == Continue;
}

inline bool IsConstant(Token t) {
	return t >= String && t <= False;
}

inline bool IsOperator(Token t) {
	return t >= Equal && t <= Div || t == FunctionCall;
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
		n->type = std::get<bool>(n->data) ? True : False;
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
		n->data = VariantToStr(l);// -VariantToStr(r); // @todo: Remove substrings?
	} break;
	case Number: {
		n->data = VariantToFloat(l) - VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) && VariantToBool(r);
		n->type = std::get<bool>(n->data) ? True : False;
	} break;
	default:
		break;
	}
}

void OpNeg(Node* n, Node* l) {
	n->type = l->type;
	switch (l->type)
	{
	case Number: {
		n->data = -VariantToFloat(l);
	} break;
	case False: 
	case True: {
		n->data = !VariantToBool(l);
		n->type = std::get<bool>(n->data) ? True : False;
	} break;
	default:
		break;
	}
}

void OpNot(Node* n, Node* l) {
	n->type = l->type;
	switch (l->type)
	{
	case Number: {
		n->data = !VariantToBool(l);
	} break;
	case False: 
	case True: {
		n->data = !VariantToBool(l);
	} break;
	default:
		break;
	}
	n->type = std::get<bool>(n->data) ? True : False;
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
		n->type = std::get<bool>(n->data) ? True : False;
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
		n->type = std::get<bool>(n->data) ? True : False;
	} break;
	default:
		break;
	}
}

void OpEqual(Node* n, Node* l, Node* r) {
	n->type = l->type;
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

void OpNotEqual(Node* n, Node* l, Node* r) {
	switch (l->type)
	{
	case String: {
		n->data = VariantToStr(l) != VariantToStr(r); // TODO Remove substrings?
	} break;
	case Number: {
		n->data = VariantToFloat(l) != VariantToFloat(r);
	} break;
	case False: 
	case True: {
		n->data = VariantToBool(l) != VariantToBool(r);
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
			case Negate: OpNeg(n, l); break;
			case Not: OpNot(n, l); break;
			case Add: OpAdd(n, l, r); break;
			case Sub: OpSub(n, l, r); break;
			case Mult: OpMult(n, l, r); break;
			case Div: OpDiv(n, l, r); break;
			case Equal: OpEqual(n, l, r); break;
			case NotEqual: OpNotEqual(n, l, r); break;
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

void Desugar(Node*& n)
{
	for (auto& c : n->children) {
		Desugar(c);
	}

	switch (n->type)
	{
	case Pipe: {
		Node* previous = nullptr;
		for (auto& node : n->children) {
			if (node->type == FunctionCall && previous) {
				node->children.back()->children.emplace_front(previous);
			}
			previous = node;
		}
		n->children.clear();
		delete n;
		n = previous;
	} break;

	default:
		break;
	}
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
	HasError = false;
	maxRegister = 0;

	HasDebug = true; // @todo: disable debugs
}

ASTWalker::~ASTWalker()
{
	delete root;
}

void ASTWalker::Run()
{
	std::vector<std::pair<Node*, Function*>> functionList;
	for (auto& c : root->children) {
		switch (c->type)
		{
		case NamespaceDef:
		{
			currentNamespace = &namespaces[std::get<0>(c->data)];
			currentNamespace->Name = std::get<0>(c->data);
		} break;
		
		case ObjectDef:
		{
			auto& data = std::get<0>(c->data);
			auto s = findSymbol(data);
			if (!s) {
				auto it = currentNamespace->symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				symbol->setType(SymbolType::Object);
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

				symbol->resolved = true;
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
				auto it = currentNamespace->symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				symbol->setType(SymbolType::Function);
				symbol->varType = VariableType::Function;
				Function* function = new Function();
				function->Name = data;
				function->IsPublic = c->type == PublicFunctionDef;
				currentNamespace->functions.emplace(function->Name.c_str(), function).first->second;
				c->sym = symbol;
				function->Namespace = currentNamespace->Name;
				function->NamespaceHash = ankerl::unordered_dense::hash<std::basic_string<char>>()(currentNamespace->Name);
				functionList.emplace_back(c, function);
				function->Types.resize(1);
				function->scope = new Scoped();
				currentFunction = function;

				for (auto& node : c->children) {
					switch (node->type)
					{
					case Scope: break;
					case CallParams: {
						function->ArgCount = (uint8)node->children.size();
						for (auto& v : node->children) {
							auto symParam = function->scope->addSymbol(std::get<0>(v->data));
							Walk(v->children.front());
							symParam->setType(SymbolType::Variable);
							symParam->varType = v->children.front()->varType;
							if (symParam->varType != VariableType::Undefined) {
								symParam->flags = symParam->flags | SymbolFlags::Typed;
							}
							symParam->resolved = true;
							function->Types.push_back(symParam->varType);
						}
					} break;
					default:
						Walk(node);
						symbol->varType = node->varType;
						function->Types[0] = symbol->varType;
						break;
					}
				}

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

	for (auto& [node, function] : functionList) {
		handleFunction(node, function, node->sym);
		gDebug() << "Generated function '" << function->Name << "', used " << maxRegister + 1 << " registers and " << function->Bytecode.size() << " instructions\n";

	}
}

std::string getFullId(Node* n) {
	std::string full;
	for (auto& c : n->children) {
		if (c->type != Id) return full;
		full += getFullId(c) + ".";
	}
	full += std::get<0>(n->data);
	return full;
}

#define _Op(op) n->instruction = instructionList.size(); auto& instruction = instructionList.emplace_back(); instruction.code = OpCodes::op; 
#define _N(n, func) case Token::n: {func} break
#define _Walk for(auto& child : n->children) Walk(child);
#define _In16 instruction.param
#define _In8 instruction.in1
#define _In8_2 instruction.in2
#define _Out n->regTarget = instruction.target = getFirstFree()
#define _FreeChildren for (auto& c : n->children) { if (IsConstant(c->type) || IsOperator(c->type)) freeReg(c->regTarget); }
#define _Type n->varType 
#define _SetOut(n) n->regTarget = instructionList[n->instruction].target
#define _First() n->children.front()
#define _Last() n->children.back()
#define _Error(text) gError() << "Line " << n->line << ": " << text << "\n"; HasError = true;
#define _Warn(text) gWarn() << "Line " << n->line << ": " << text << "\n";
#define _FreeConstant(n) if (IsConstant(n->type) || IsOperator(n->type)) freeReg(n->regTarget);

#define _Operator(varTy, op) \
case VariableType::varTy: {\
	if (_Last()->varType != VariableType::Undefined && _Last()->varType != VariableType::varTy) {\
		_Error("Cannot perform arithmetic on types");\
	}\
	_Type = VariableType::varTy;\
	instruction.code = OpCodes::op; \
	_In8 = _First()->regTarget;\
	_In8_2 = _Last()->regTarget;\
	_FreeChildren;\
} break;

#define _OperatorAssign \
if (n->data.index() == 0 && *std::get<std::string>(n->data).c_str() == '=') {\
	if (_First()-> sym && isAssignable(_First()->sym->flags)) {\
		n->regTarget = instruction.target = _First()->regTarget;\
	}\
	else {\
		_Error("Cannot assign types");\
	}\
}\
else _Out;

#define _Compare(op) \
_Walk;\
if (_First()->varType != _Last()->varType && _First()->varType != VariableType::Undefined && _Last()->varType != VariableType::Undefined) {\
	_Error("Can only compare number types");\
}\
_Type = VariableType::Boolean;\
_Op(op);\
_In8 = _First()->regTarget;\
_In8_2 = _Last()->regTarget;\
_FreeChildren;\
_Out;\

void ASTWalker::Walk(Node* n)
{
	Function* f = currentFunction;
	if (HasDebug) f->debugLines[(int)n->line] = (int)instructionList.size();
	switch (n->type) {
		_N(Scope,
			auto& next = currentScope->children.emplace_back();
			next.parent = currentScope;
			currentScope = &next;
			auto regs = registers;
			_Walk;
			currentScope = currentScope->parent;
			registers = regs;
		);

		_N(TypeNumber,
			_Type = VariableType::Number;
		);
		_N(TypeString,
			_Type = VariableType::String;
		);
		_N(AnyType,
			_Type = VariableType::Undefined;
		);
		_N(Typename,
			_Type = VariableType::Object;
		);

		_N(Number, {
			_Op(LoadNumber);
			auto res = f->numberTable.emplace(std::get<double>(n->data));
			uint16 idx = (uint16)std::distance(f->numberTable.begin(), res.first);
			_In16 = idx;
			_Out;
			_Type = VariableType::Number;
			}
		);

		_N(True,
			_Op(PushBoolean);
			_Out;
			_In8 = 1;
			_Type = VariableType::Boolean;
		);
		_N(False,
			_Op(PushBoolean);
			_Out;
			_In8 = 0;
			_Type = VariableType::Boolean;
		);

		_N(Add, 
			_Walk; 
			_Op(NumAdd);
			switch (_First()->varType)
			{
			_Operator(Undefined, Add)
			_Operator(Number, NumAdd)
			_Operator(String, StrAdd)
			default:
				_Error("No correct operator + found for type"); 
				HasError = true; 
				break; 
			}
			_OperatorAssign;
		);
		_N(Sub,
			_Walk;
			_Op(NumAdd);
			switch (_First()->varType)
			{
				_Operator(Number, NumSub);
			default:
				_Error("No correct operator - found for type");
				HasError = true;
				break;
			}
			_OperatorAssign;

		);
		_N(Div,
			_Walk;
			_Op(NumAdd);
			switch (_First()->varType)
			{
				_Operator(Number, NumDiv);
			default:
				_Error("No correct operator / found for type");
				HasError = true;
				break;
			}
			_OperatorAssign;

		);
		_N(Mult,
			_Walk;
			_Op(NumAdd);
			switch (_First()->varType)
			{
				_Operator(Number, NumMul);
			default:
				_Error("No correct operator * found for type");
				HasError = true;
				break;
			}
			_OperatorAssign;

		);

		_N(Id,
			auto symbol = findSymbol(std::get<std::string>(n->data));
			if (symbol) {
				if (symbol->needsLoading) {
					//_Op(LoadSymbol) // @todo: actual loading here
				}
				else {
					n->regTarget = symbol->reg;
					symbol->endLife = instructionList.size();
				}
				_Type = symbol->varType;
				n->sym = symbol;
			}
			else { // @todo: fix this, symbols might be loaded later
				_Error("Symbol not defined: " << std::get<std::string>(n->data));
				HasError = true;
			}
		);

		_N(Negate, // @todo: Maybe own instruction
			auto& var = _First()->varType;
			if (var != VariableType::Number && var != VariableType::Undefined) {
				_Error("Cannot negate non number types");
			}
			auto& load = instructionList.emplace_back(); 
			load.code = OpCodes::LoadNumber;
			auto res = f->numberTable.emplace(0.0);
			uint16 idx = (uint16)std::distance(f->numberTable.begin(), res.first);
			load.target = getFirstFree();
			load.param = idx;

			_Op(NumSub);
			_Walk;
			_In8 = load.target;
			_In8_2 = _First()->regTarget;
			_FreeChildren; // @todo: Can we do that??
			freeReg(load.target);
			_Out;
			_Type = VariableType::Number;
		);

		_N(Not, // @todo: Maybe own instruction
			_Op(Not);
			_Walk;
			_In8 = _First()->regTarget;
			_FreeChildren; // @todo: Can we do that??
			_Out;
			_Type = VariableType::Boolean;
		);

		_N(Less,
			_Compare(Less)
		);

		_N(LessEqual,
			_Compare(LessEqual)
		);

		_N(Larger,
			_Compare(Greater)
		);

		_N(LargerEqual,
			_Compare(GreaterEqual)
		);

		_N(Equal,
			_Compare(Equal)
		);

		_N(NotEqual,
			_Compare(NotEqual)
		);

		case Decrement:
		_N(Increment,
			_Walk;
			if (_First()->varType != VariableType::Number) {
				_Error("Cannot increment");
			}
			_Op(PostMod);
			_In8 = _First()->regTarget;
			_In8_2 = n->type == Increment ? 0 : 1;
			_FreeChildren;
			_Out;
		);

		case PreDecrement:
		_N(PreIncrement,
			_Walk;
			if (_First()->varType != VariableType::Number) {
				_Error("Cannot increment");
			}
			_Op(PreMod);
			_In8 = _First()->regTarget;
			_In8_2 = n->type == PreIncrement ? 0 : 1;
			n->regTarget = instruction.target = _First()->regTarget;
		);

		_N(Assign,
			if (n->children.size() != 2) break;
			_Walk;
			auto& lhs = _First();
			auto& rhs = _Last();
			if (!lhs->sym || !isAssignable(lhs->sym->flags)) {
				_Error("Trying to assign to unassignable type");
				HasError = true;
			}
			if (rhs->varType != VariableType::Undefined && lhs->varType != rhs->varType && lhs->sym) {
				if (isTyped(lhs->sym->flags) || lhs->data.index() != 0) {
					_Error("Trying to assign unrelated types");
					HasError = true;
				}
				else { // @todo: something else here, lhs might not be symbol
					lhs->sym->varType = _Type = lhs->varType = rhs->varType;
				}
			}
			_FreeConstant(rhs);
			n->regTarget = _SetOut(rhs) = lhs->regTarget;
		);

		_N(VarDeclare,
			auto sym = currentScope->addSymbol(std::get<std::string>(n->data));
			sym->startLife = instructionList.size();
			sym->endLife = instructionList.size();
			sym->setType(SymbolType::Variable);
			sym->resolved = true;
			if (n->children.size() > 0) {
				_Walk;
				sym->varType = _Type = _First()->varType; // @todo: types for user defined objects;
				if (sym->varType != VariableType::Undefined) {
					sym->flags = sym->flags | SymbolFlags::Typed;
				}
				if (n->children.size() == 2) {
					if (_Last()->varType == _Type || _Type == VariableType::Undefined) {
						sym->varType = _Type = _Last()->varType;
						sym->reg = n->regTarget = _Last()->regTarget;
						break;
					}
					_Error("Cannot assign unrelated types");
				}
			}
			_Op(PushTypeDefault);
			_In16 = (uint16)sym->varType;
			sym->reg = _Out;
		);

		_N(Return,
			_Walk;
			_Op(Return);
			if (!n->children.empty()) {
				instruction.target = _First()->regTarget;
				_In8 = 1;
			}
		);

		_N(If,
			if (n->children.size() != 3) {
				_Error("Incorrect if-block");
			}

			auto& next = currentScope->children.emplace_back();
			next.parent = currentScope;
			currentScope = &next;
			auto regs = registers;
			Walk(_First());
			_Op(JumpNeg);
			n->regTarget = instruction.target = _First()->regTarget;
			_FreeConstant(n);

			auto oldSize = instructionList.size();
			auto it = ++n->children.begin();
			Walk(*it);
			_FreeConstant((*it));

			auto& elseinst = instructionList.emplace_back(); 
			elseinst.code = OpCodes::JumpForward;
			
			_In16 = instructionList.size() - oldSize;
			oldSize = instructionList.size();

			Walk(_Last());
			_FreeConstant(_Last());

			elseinst.param = instructionList.size() - oldSize;

			currentScope = currentScope->parent;
			registers = regs;
			);

		_N(For, // @todo: Add else block for loops
			if (n->children.size() != 4) {
				_Error("Incorrect for-block");
			}

			auto& next = currentScope->children.emplace_back();
			next.parent = currentScope;
			currentScope = &next;
			auto regs = registers;
			Walk(_First());

			auto start = instructionList.size();
			auto it = ++n->children.begin();
			Walk(*it);
			_FreeConstant((*it));

			_Op(JumpNeg);
			n->regTarget = instruction.target = (*it)->regTarget;
			auto jumpStart = instructionList.size();

			Walk(_Last());
			_FreeConstant((*it));

			it++;
			Walk(*it);
			_FreeConstant((*it));

			auto& elseinst = instructionList.emplace_back();
			elseinst.code = OpCodes::JumpBackward;
			elseinst.param = instructionList.size() - start;

			_In16 = instructionList.size() - jumpStart;

			currentScope = currentScope->parent;
			registers = regs;
			);

		_N(While,
			if (n->children.size() != 2) {
				_Error("Incorrect while-block");
			}

			auto& next = currentScope->children.emplace_back();
			next.parent = currentScope;
			currentScope = &next;
			auto regs = registers;

			auto start = instructionList.size();
			Walk(_First());
			_Op(JumpNeg);
			auto jumpStart = instructionList.size();
			n->regTarget = instruction.target = _First()->regTarget;

			auto it = ++n->children.begin();
			Walk(*it);
			_FreeConstant((*it));

			auto& elseinst = instructionList.emplace_back();
			elseinst.code = OpCodes::JumpBackward;
			elseinst.param = instructionList.size() - start;
			
			_In16 = instructionList.size() - jumpStart;

			currentScope = currentScope->parent;
			registers = regs;
			);

		_N(Else,
			_Walk;
		);

		_N(FunctionCall,

			Walk(_First());
			if (!_First()->sym || _First()->sym->type != SymbolType::Function) {
				_Error("Variable functions not supported yet"); // @todo: Add support
			}
			auto name = getFullId(_First());

			auto& params = _Last();
			for (auto& c : params->children) {
				Walk(c);
				if (IsConstant(c->type) || IsOperator(c->type)) {
					freeReg(c->regTarget);
					c->regTarget = getLastFree();
				}
				else {
					_Op(Copy);
					instruction.target = getLastFree();
					_In8 = c->regTarget;
					c->regTarget = instruction.target;
				}
			}

			for (auto& c : params->children) {
				freeReg(c->regTarget);
			}

			auto it = std::find(currentFunction->functionTableSymbols.begin(), currentFunction->functionTableSymbols.end(), name);
			size_t index = 0;

			if (it != currentFunction->functionTableSymbols.end()) {
				index = it - currentFunction->functionTableSymbols.begin();
			}
			else {
				currentFunction->functionTableSymbols.push_back(name);
				currentFunction->functionTable.push_back(nullptr);
			}

			_Op(CallFunction);
			_In8 = (uint8)index;
			if (params->children.size() != 0) {
				_In8_2 = (uint8)params->children.front()->regTarget;
			}
			_Out;
		);

	default:
		_Error("Unexpected token found");
		break;
	}
}

Symbol* ASTWalker::findSymbol(const std::string& name)
{
	if (namespaces.find(name) != namespaces.end()) return nullptr;
	if (auto s = currentNamespace->findSymbol(name); s != nullptr) return s;
	if (currentScope) if (auto s = currentScope->findSymbol(name); s != nullptr) return s;
	return nullptr;
}

void ASTWalker::handleFunction(Node* n, Function* f, Symbol* s)
{
	initRegs();
	currentFunction = f;

	for (auto& c : n->children) {
		switch (c->type)
		{
		case CallParams: {
			for (auto& v : c->children) {
				auto symParam = f->scope->findSymbol(std::get<0>(v->data));
				symParam->reg = getFirstFree();
			}
		} break;

		case Scope: {

			instructionList.reserve(c->children.size() * n->depth);
			currentScope = f->scope;
			for (auto& stmt : c->children) {
				try {
					Walk(stmt);
					if (IsConstant(stmt->type) || IsOperator(stmt->type)) freeReg(stmt->regTarget);
				}
				catch (...) {
					gError() << "Internal error occured!\n";
				}
			}

		} break;

		default:
			break;
		}
	}

	f->Bytecode.resize(instructionList.size());
	for (int i = 0; i < instructionList.size(); i++) {
		f->Bytecode[i] = instructionList[i].data;
	}

	f->RegisterCount = maxRegister + 1;
	s->resolved = true;
	instructionList.clear();

	currentFunction = nullptr;
	currentScope = nullptr;

	// Cleanup
	delete f->scope;
	f->scope = nullptr;
}
