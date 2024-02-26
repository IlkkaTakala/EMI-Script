#include "AST.h"
#include "NativeFuncs.h"
#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "VM.h"

constexpr unsigned char print_first[] = { 195, 196, 196, 0 };
constexpr unsigned char print_last[] = { 192, 196, 196, 0 };
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
	InternalValue v;
	switch (data.index())
	{
	case 0: v = std::get<0>(data).c_str(); break;
	case 1: v = std::get<1>(data); break;
	case 2: v = std::get<2>(data); break;
	default:
		break;
	}
	auto var = moveOwnershipToVM(v);
	return var;
}
#ifdef DEBUG
void Node::print(const std::string& prefix, bool isLast)
{
	gLogger() << prefix;
	gLogger() << (isLast ? (char*)print_last : (char*)print_first);
	gLogger() << TokensToName[type] << ": " << VariantToStr(this) << '\n';

	for (auto& node : children) {
		node->print(prefix + (isLast ? "    " : (char*)print_add), node == children.back());
	}
}
#endif

inline bool IsControl(Token t) {
	return t == Token::Return || t == Token::Break || t == Token::Continue;
}

inline bool IsConstant(Token t) {
	return t >= Token::Literal && t <= Token::False;
}

inline bool IsOperator(Token t) {
	return t >= Token::Equal && t <= Token::Div || t == Token::FunctionCall;
}

void OpAdd(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Literal: {
		n->data = VariantToStr(l) + VariantToStr(r);
	} break;
	case Token::Number: {
		n->data = VariantToFloat(l) + VariantToFloat(r);
	} break;
	case Token::False:
	case Token::True: {
		n->data = VariantToBool(l) || VariantToBool(r);
		n->type = std::get<bool>(n->data) ? Token::True : Token::False;
	} break;
	default:
		break;
	}
}

void OpSub(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Literal: {
		n->data = VariantToStr(l);// -VariantToStr(r); // @todo: Remove substrings?
	} break;
	case Token::Number: {
		n->data = VariantToFloat(l) - VariantToFloat(r);
	} break;
	case Token::False:
	case Token::True: {
		n->data = VariantToBool(l) && VariantToBool(r);
		n->type = std::get<bool>(n->data) ? Token::True : Token::False;
	} break;
	default:
		break;
	}
}

void OpNeg(Node* n, Node* l) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Number: {
		n->data = -VariantToFloat(l);
	} break;
	case Token::False: 
	case Token::True: {
		n->data = !VariantToBool(l);
		n->type = std::get<bool>(n->data) ? Token::True : Token::False;
	} break;
	default:
		break;
	}
}

void OpNot(Node* n, Node* l) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Number: {
		n->data = !VariantToBool(l);
	} break;
	case Token::False: 
	case Token::True: {
		n->data = !VariantToBool(l);
	} break;
	default:
		break;
	}
	n->type = std::get<bool>(n->data) ? Token::True : Token::False;
}

void OpMult(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Literal: {
		n->data = VariantToStr(l);// -VariantToStr(r); // TODO Remove substrings?
	} break;
	case Token::Number: {
		n->data = VariantToFloat(l) * VariantToFloat(r);
	} break;
	case Token::False: 
	case Token::True: {
		n->data = VariantToBool(l) || VariantToBool(r);
		n->type = std::get<bool>(n->data) ? Token::True : Token::False;
	} break;
	default:
		break;
	}
}

void OpDiv(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Literal: {
		n->data = VariantToStr(l);// -VariantToStr(r); // TODO Remove substrings?
	} break;
	case Token::Number: {
		n->data = VariantToFloat(l) / VariantToFloat(r);
	} break;
	case Token::False: 
	case Token::True: {
		n->data = VariantToBool(l) || VariantToBool(r);
		n->type = std::get<bool>(n->data) ? Token::True : Token::False;
	} break;
	default:
		break;
	}
}

void OpEqual(Node* n, Node* l, Node* r) {
	n->type = l->type;
	switch (l->type)
	{
	case Token::Literal: {
		n->data = VariantToStr(l) == VariantToStr(r); // TODO Remove substrings?
	} break;
	case Token::Number: {
		n->data = VariantToFloat(l) == VariantToFloat(r);
	} break;
	case Token::False: 
	case Token::True: {
		n->data = VariantToBool(l) == VariantToBool(r);
	} break;
	default:
		break;
	}
	n->type = std::get<bool>(n->data) ? Token::True : Token::False;
}

void OpNotEqual(Node* n, Node* l, Node* r) {
	switch (l->type)
	{
	case Token::Literal: {
		n->data = VariantToStr(l) != VariantToStr(r); // TODO Remove substrings?
	} break;
	case Token::Number: {
		n->data = VariantToFloat(l) != VariantToFloat(r);
	} break;
	case Token::False: 
	case Token::True: {
		n->data = VariantToBool(l) != VariantToBool(r);
	} break;
	default:
		break;
	}
	n->type = std::get<bool>(n->data) ? Token::True : Token::False;
}

bool Optimize(Node*& n)
{
	switch (n->type)
	{
	case Token::Conditional: {
		if (n->children.size() == 3) {
			if (IsConstant(n->children.front()->type)) {
				auto old = n;
				n = *std::next(old->children.begin(), VariantToBool(n->children.front()) ? 1 : 2);
				std::erase(old->children, n);
				delete old;
			}
		}
	} break;

	case Token::Stmt: {
		if (n->children.empty()) break;
		if (IsConstant(n->children.front()->type)) {
			delete n;
			n = nullptr;
			return false;
		}
	} break;

	case Token::Scope: {
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

	case Token::Static: {
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
			case Token::Negate: OpNeg(n, l); break;
			case Token::Not: OpNot(n, l); break;
			case Token::Add: OpAdd(n, l, r); break;
			case Token::Sub: OpSub(n, l, r); break;
			case Token::Mult: OpMult(n, l, r); break;
			case Token::Div: OpDiv(n, l, r); break;
			case Token::Equal: OpEqual(n, l, r); break;
			case Token::NotEqual: OpNotEqual(n, l, r); break;
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
	case Token::Pipe: {
		Node* previous = nullptr;
		for (auto& node : n->children) {
			if (node->type == Token::FunctionCall && previous) {
				node->children[1]->children.insert(node->children[1]->children.begin(), previous);
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

void TypeConverter(NodeDataType& n, const TokenHolder& h)
{
	switch (h.token)
	{
	case Token::Number:
		n = 0.f;
		std::from_chars(h.data.data(), h.data.data() + h.data.size(), std::get<double>(n));
		break;
	case Token::True:
		n = true;
		break;
	case Token::False:
		n = false;
		break;
	default:
		std::get<std::string>(n) = h.data;
		break;
	}
}

ASTWalker::ASTWalker(VM* in_vm, Node* n)
{
	vm = in_vm;
	root = n;
	auto it = namespaces.emplace("Global", Namespace{});
	it.first->second.Name = "Global";
	currentNamespace = &it.first->second;
	currentNamespace->Sym = new Symbol();
	currentNamespace->Sym->setType(SymbolType::Namespace);
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

std::string getFullId(Node* n) {
	std::string full;
	for (auto& c : n->children) {
		if (c->type != Token::Id) break;
		full += getFullId(c) + ".";
	}
	full += std::get<0>(n->data);
	return full;
}

void ASTWalker::Run()
{
	std::vector<std::pair<Node*, Function*>> functionList;
	for (auto& c : root->children) {
		switch (c->type)
		{
		case Token::NamespaceDef:
		{
			currentNamespace = &namespaces[std::get<0>(c->data)];
			currentNamespace->Name = std::get<0>(c->data);
			auto sym = new Symbol();
			sym->setType(SymbolType::Namespace);
			sym->resolved = true;
			currentNamespace->Sym = sym;
		} break;
		
		case Token::ObjectDef:
		{
			auto& data = std::get<0>(c->data);
			bool isNameSpace;
			auto s = findSymbol(data, "", isNameSpace);
			if (!s && !isNameSpace) {
				auto it = currentNamespace->symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				symbol->setType(SymbolType::Object);
				auto& object = currentNamespace->objects.emplace(data, UserDefinedType{}).first->second;
				
				for (auto& field : c->children) {
					Symbol flags;
					flags.flags = SymbolFlags::None;
					Variable var;

					if (field->children.size() == 2) {
						if (field->children.front()->type != Token::AnyType) {
							flags.flags = SymbolFlags::Typed;
						}
						VariableType type = VariableType::Undefined;
						switch (field->children.front()->type)
						{
							case Token::TypeNumber: {
								type = VariableType::Number;
							}break;
							case Token::TypeBoolean: {
								type = VariableType::Boolean;
							}break;
							case Token::TypeString: {
								type = VariableType::String;
							}break;
							case Token::TypeArray: {
								type = VariableType::Array;
							}break;
							case Token::TypeFunction: {
								type = VariableType::Function;
							}break;
							case Token::AnyType: {
								type = VariableType::Undefined;
							}break;
							case Token::Typename: {
								type = VariableType::Object;
							}break;
						}
						flags.varType = type;
						if (IsConstant(field->children.back()->type)) {
							if (TypesMatch(field->children.front()->type, field->children.back()->type)) {
								var = field->children.back()->toVariable(); // @todo: Add implicit conversions
							}
							else {
								gError() << "Trying to initialize with wrong type: In object " << data << ", field " << std::get<0>(field->data) << '\n';
							}
						}
						else if (field->children.back()->type == Token::OExpr) {
							var = GetTypeDefault(type);
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
		
		case Token::PublicFunctionDef:
		case Token::FunctionDef:
		{
			auto& data = std::get<0>(c->data);
			bool isNameSpace;
			auto s = findSymbol(data, "", isNameSpace);
			if (!s && !isNameSpace) {
				auto it = currentNamespace->symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				symbol->setType(SymbolType::Function);
				symbol->varType = VariableType::Function;
				symbol->needsLoading = true;
				Function* function = new Function();
				function->Name = data;
				function->IsPublic = c->type == Token::PublicFunctionDef;
				currentNamespace->functions.emplace(function->Name.c_str(), function).first->second;
				c->sym = symbol;
				function->Namespace = currentNamespace->Name;
				function->NamespaceHash = ankerl::unordered_dense::hash<std::basic_string<char>>()(currentNamespace->Name);
				functionList.emplace_back(c, function);
				function->Types.resize(1);
				function->FunctionScope = new Scoped();
				currentFunction = function;

				for (auto& node : c->children) {
					switch (node->type)
					{
					case Token::Scope: break;
					case Token::CallParams: {
						function->ArgCount = (uint8)node->children.size();
						for (auto& v : node->children) {
							auto symParam = findSymbol(std::get<0>(v->data), "", isNameSpace);
							if (symParam || isNameSpace) {
								gError() << "Line " << node->line << ": Symbol '" << std::get<0>(v->data) << "' already defined\n";
								HasError = true;
								break;
							}
							symParam = function->FunctionScope->addSymbol(std::get<0>(v->data));
							WalkLoad(v->children.front());
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
						WalkLoad(node);
						function->Types[0] = symbol->varType;
						break;
					}
				}

			}
			else {
				gError() << "Line " << c->line << ": Symbol '" << data << "' already defined\n";
			}
		} break;

		case Token::Static:
		case Token::Const:
		case Token::VarDeclare:
		{
			auto& data = std::get<0>(c->data);
			bool isNameSpace;
			auto s = findSymbol(data, "", isNameSpace);
			if (!s && !isNameSpace) {
				auto name = currentNamespace->Name + '.' + data;
				auto it = currentNamespace->symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				switch (c->type)
				{
				case Token::Static: symbol->setType(SymbolType::Static); break;
				case Token::Const: symbol->setType(SymbolType::Static); break;
				case Token::VarDeclare: symbol->setType(SymbolType::Variable); break;
				}
				auto& variable = currentNamespace->variables.emplace(data, Variable{}).first->second;

				symbol->startLife = (size_t)-1;
				symbol->needsLoading = true;

				switch (c->children[0]->type)
				{
				case Token::TypeNumber:
					symbol->varType = VariableType::Number;
					variable = VariantToFloat(c->children[1]);
					symbol->flags = symbol->flags | SymbolFlags::Typed;
					break;
				case Token::TypeString:
					symbol->varType = VariableType::String;
					variable = String::GetAllocator()->Make(VariantToStr(c->children[1]).c_str());
					symbol->flags = symbol->flags | SymbolFlags::Typed;
					break;
				case Token::Typename:
					symbol->varType = VariableType::Object;
					symbol->flags = symbol->flags | SymbolFlags::Typed;
					// @todo: make objects;
					break;
				case Token::TypeArray:
					symbol->varType = VariableType::Array;
					variable = Array::GetAllocator()->Make(c->children[1]->children.size());
					// @todo: Initialize global array
					symbol->flags = symbol->flags | SymbolFlags::Typed;
					break;
				case Token::AnyType:
					switch (c->children[1]->type)
					{
					case Token::Number:
						symbol->varType = VariableType::Number;
						variable = VariantToFloat(c->children[1]);
						break;
					case Token::Literal:
						symbol->varType = VariableType::String;
						variable = String::GetAllocator()->Make(VariantToStr(c->children[1]).c_str());
						break;
					case Token::Array:
						symbol->varType = VariableType::Array;
						variable = Array::GetAllocator()->Make(c->children[1]->children.size());
						break;
					default:
						symbol->varType = VariableType::Undefined;
						break;
					}
					break;
				}
			}
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


#define _Op(op) n->instruction = instructionList.size(); auto& instruction = instructionList.emplace_back(); instruction.code = OpCodes::op; 
#define _Walk for(auto& child : n->children) WalkLoad(child);
#define _In16 instruction.param
#define _In8 instruction.in1
#define _In8_2 instruction.in2
#define _Out n->regTarget = instruction.target = getFirstFree()
#define _FreeChildren for (auto& c : n->children) { if (IsConstant(c->type) || IsOperator(c->type) || (c->sym && c->sym->needsLoading) ) freeReg(c->regTarget); }
#define _Type n->varType 
#define _SetOut(n) n->regTarget = instructionList[n->instruction].target
#define _First() first_child
#define _Last() last_child
#define _Error(text) gError() << "Line " << n->line << ": " << text << "\n"; HasError = true;
#define _Warn(text) gWarn() << "Line " << n->line << ": " << text << "\n";
#define _FreeConstant(n) if (IsConstant(n->type) || IsOperator(n->type) || (n->sym && n->sym->needsLoading) ) freeReg(n->regTarget);
#define _EnsureOperands if (n->children.size() != 2) { _Error("Invalid number of operands") break; }

#define _Operator(varTy, op, op2) \
case VariableType::varTy: {\
	if (_Last()->varType != VariableType::Undefined && _Last()->varType != VariableType::varTy) {\
		instruction.code = OpCodes::op2; \
	}\
	else { \
		instruction.code = OpCodes::op; \
	} \
	_Type = VariableType::varTy;\
	_In8 = _First()->regTarget;\
	_In8_2 = _Last()->regTarget;\
	_FreeChildren;\
} break;

#define _OperatorAssign _Out;

#define _Compare(op) \
_Walk;\
if (_First()->varType != _Last()->varType && _First()->varType != VariableType::Undefined && _Last()->varType != VariableType::Undefined) {\
	_Error("Can only compare similar types");\
}\
_Type = VariableType::Boolean;\
_Op(op);\
_In8 = _First()->regTarget;\
_In8_2 = _Last()->regTarget;\
_FreeChildren;\
_Out;\

void ASTWalker::WalkLoad(Node* n)
{
	Function* f = currentFunction;
	if (HasDebug) f->DebugLines[(int)n->line] = (int)instructionList.size();
	Node* first_child = n->children.size() ? n->children.front() : nullptr;
	Node* last_child = n->children.size() ? n->children.back() : nullptr;
	switch (n->type) {
	case Token::Scope: {
		auto& next = currentScope->children.emplace_back();
		next.parent = currentScope;
		currentScope = &next;
		auto regs = registers;
		for (auto& c : n->children) {
			WalkLoad(c);
			_FreeConstant(c);
		}
		currentScope = currentScope->parent;
		registers = regs;
	} break;

	case Token::TypeNumber: {
		_Type = VariableType::Number;
	}break;
	case Token::TypeBoolean: {
		_Type = VariableType::Boolean;
	}break;
	case Token::TypeString: {
		_Type = VariableType::String;
	}break;
	case Token::TypeArray: {
		_Type = VariableType::Array;
	}break;
	case Token::TypeFunction: {
		_Type = VariableType::Function;
	}break;
	case Token::AnyType: {
		_Type = VariableType::Undefined;
	}break;
	case Token::Typename: {
		auto& name = std::get<std::string>(n->data);
		auto it = std::find(currentFunction->TypeTableSymbols.begin(), currentFunction->TypeTableSymbols.end(), name);
		size_t index = 0;
		if (it == currentFunction->TypeTableSymbols.end()) {
			index = currentFunction->TypeTableSymbols.size();
			currentFunction->TypeTableSymbols.push_back(name);
		}
		else {
			index = it - currentFunction->TypeTableSymbols.begin();
		}
		_Type = static_cast<VariableType>(static_cast<size_t>(VariableType::Object) + index);
	}break;

	case Token::Number: {
		_Op(LoadNumber);
		auto res = f->NumberTable.emplace(std::get<double>(n->data));
		uint16 idx = (uint16)std::distance(f->NumberTable.begin(), res.first);
		_In16 = idx;
		_Out;
		_Type = VariableType::Number;
	}break;

	case Token::Literal: {
		_Op(LoadString);
		auto res = stringList.emplace(std::get<std::string>(n->data));
		uint16 idx = (uint16)std::distance(stringList.begin(), res.first);
		_In16 = idx;
		_Out;
		_Type = VariableType::String;
	}break;

	case Token::Array: {
		{
			_Op(PushArray);
			_In16 = n->children.size();
			_Out;
		}
		for (auto& c : n->children) {
			WalkLoad(c);
			_Op(PushIndex);
			instruction.target = n->regTarget;
			_In8 = c->regTarget;
			_FreeConstant(c);
		}
		_FreeChildren;
		_Type = VariableType::Array;
	}break;

	case Token::Indexer: {
		_Walk;
		_Op(LoadIndex);
		//n->sym = _First()->sym;
		_In8 = _First()->regTarget;
		_In8_2 = _Last()->regTarget;
		_FreeConstant(_Last());
		_Out;
	}break;

	case Token::True: {
		_Op(PushBoolean);
		_Out;
		_In8 = 1;
		_Type = VariableType::Boolean;
	}break;
	case Token::False: {
		_Op(PushBoolean);
		_Out;
		_In8 = 0;
		_Type = VariableType::Boolean;
	}break;

	case Token::Add: {
		_EnsureOperands;
		_Walk;
		_Op(NumAdd);
		switch (_First()->varType)
		{
			_Operator(Number, NumAdd, Add)
				_Operator(String, StrAdd, Add)
		default:
			_Type = VariableType::Undefined;
			instruction.code = OpCodes::Add;
			_In8 = _First()->regTarget;
			_In8_2 = _Last()->regTarget;
			_FreeChildren;
			break;
		}
		_OperatorAssign;
	}break;
	case Token::Sub: {
		_EnsureOperands;
		_Walk;
		_Op(NumAdd);
		switch (_First()->varType)
		{
			_Operator(Number, NumSub, Sub);
		default:
			_Type = VariableType::Undefined;
			instruction.code = OpCodes::Sub;
			_In8 = _First()->regTarget;
			_In8_2 = _Last()->regTarget;
			_FreeChildren;
			break;
		}
		_OperatorAssign;

	}break;
	case Token::Div: {
		_EnsureOperands;
		_Walk;
		_Op(NumAdd);
		switch (_First()->varType)
		{
			_Operator(Number, NumDiv, Div);
		default:
			_Type = VariableType::Undefined;
			instruction.code = OpCodes::Div;
			_In8 = _First()->regTarget;
			_In8_2 = _Last()->regTarget;
			_FreeChildren;
			break;
		}
		_OperatorAssign;

	}break;
	case Token::Mult: {
		_EnsureOperands;
		_Walk;
		_Op(NumAdd);
		switch (_First()->varType)
		{
			_Operator(Number, NumMul, Mul);
		default:
			_Type = VariableType::Undefined;
			instruction.code = OpCodes::Mul;
			_In8 = _First()->regTarget;
			_In8_2 = _Last()->regTarget;
			_FreeChildren;
			break;
		}
		_OperatorAssign;

	}break;

	case Token::AssignAdd: {
		n->type = Token::Add;
		WalkLoad(n);
		_Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;
	case Token::AssignSub: {
		n->type = Token::Sub;
		WalkLoad(n);
		_Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;
	case Token::AssignDiv: {
		n->type = Token::Div;
		WalkLoad(n);
		_Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;
	case Token::AssignMult: {
		n->type = Token::Mult;
		WalkLoad(n);
		_Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;

	case Token::Id: {
		_Walk;
		bool isNameSpace = false;
		Symbol* symbol = nullptr;
		auto& data = std::get<std::string>(n->data);
		if (n->children.size() != 1) {
			symbol = findSymbol(data, "", isNameSpace);
		}
		else {
			if (_First()->sym) {
				if (_First()->sym->type == SymbolType::Namespace) {
					symbol = findSymbol(data, std::get<std::string>(_First()->data), isNameSpace);
				}
				else if (_First()->sym->type == SymbolType::Variable) {
					_Op(LoadProperty);
					_In8 = _First()->regTarget;
					_Out;
					size_t index = 0;
					auto it = std::find(currentFunction->PropertyTableSymbols.begin(), currentFunction->PropertyTableSymbols.end(), data);
					if (it != currentFunction->PropertyTableSymbols.end()) {
						index = it - currentFunction->PropertyTableSymbols.begin();
					}
					else {
						index = currentFunction->PropertyTableSymbols.size();
						currentFunction->PropertyTableSymbols.push_back(data);
					}

					auto& arg = instructionList.emplace_back();
					arg.code = OpCodes::Noop;
					arg.param = static_cast<uint16>(index);

					UserDefinedType* typeProperty = nullptr;
					int typeIndex = (int)_First()->varType - (int)VariableType::Object;
					if (typeIndex >= 0 && typeIndex < currentFunction->TypeTableSymbols.size()) {
						auto& objectName = currentFunction->TypeTableSymbols[typeIndex];
						if (GetManager().GetType(typeProperty, objectName)) {
							n->varType = typeProperty->GetFieldType(data);
						}
						else if (auto localObject = currentNamespace->objects.find(objectName); localObject != currentNamespace->objects.end()) {
							n->varType = localObject->second.GetFieldType(data);
						}
					}
					break;
				}
				else {
					_Error("Unknown symbol: " + data);
				}
			}
			else {
				_Error("Unknown symbol: " + data);
			}
		}
		if (!symbol || (symbol && symbol->needsLoading)) {
			auto name = getFullId(n);
			auto it = std::find(currentFunction->GlobalTableSymbols.begin(), currentFunction->GlobalTableSymbols.end(), name);
			size_t index = 0;

			if (it != currentFunction->GlobalTableSymbols.end()) {
				index = it - currentFunction->GlobalTableSymbols.begin();
			}
			else {
				index = currentFunction->GlobalTableSymbols.size();
				currentFunction->GlobalTableSymbols.push_back(name);
				currentFunction->GlobalTable.push_back(nullptr);
			}

			if (symbol) {
				_Type = symbol->varType;
				n->sym = symbol;
			}

			_Op(LoadSymbol);
			_In16 = index;
			_Out;
		}
		else if (symbol) {
			n->regTarget = symbol->reg;
			symbol->endLife = instructionList.size();
			_Type = symbol->varType;
			n->sym = symbol;
		}
	}break;

	case Token::Property: {
		auto& data = std::get<std::string>(n->data);
		if (n->children.size() != 1) {
			_Error("Invalid property access: " + data);
			break;
		}
		_Walk;
		if (_First()->varType > VariableType::Object || _First()->varType == VariableType::Undefined) {
			_Op(LoadProperty);
			_In8 = _First()->regTarget;
			_Out;
			size_t index = 0;
			auto it = std::find(currentFunction->PropertyTableSymbols.begin(), currentFunction->PropertyTableSymbols.end(), data);
			if (it != currentFunction->PropertyTableSymbols.end()) {
				index = it - currentFunction->PropertyTableSymbols.begin();
			}
			else {
				index = currentFunction->PropertyTableSymbols.size();
				currentFunction->PropertyTableSymbols.push_back(data);
			}

			auto& arg = instructionList.emplace_back();
			arg.code = OpCodes::Noop;
			arg.param = static_cast<uint16>(index);

			UserDefinedType* typeProperty = nullptr;
			int typeIndex = (int)_First()->varType - (int)VariableType::Object;
			if (typeIndex >= 0 && typeIndex < currentFunction->TypeTableSymbols.size()) {
				auto& objectName = currentFunction->TypeTableSymbols[typeIndex];
				if (GetManager().GetType(typeProperty, objectName)) {
					n->varType = typeProperty->GetFieldType(data);
				}
				else if (auto localObject = currentNamespace->objects.find(objectName); localObject != currentNamespace->objects.end()) {
					n->varType = localObject->second.GetFieldType(data);
				}
			}
			break;
		}
		else {
			_Error("Unknown symbol: " + data);
		}
	} break;

	case Token::Negate: {
		auto& var = _First()->varType;
		if (var != VariableType::Number && var != VariableType::Undefined) {
			_Error("Cannot negate non number types");
		}
		auto& load = instructionList.emplace_back();
		load.code = OpCodes::LoadImmediate;
		load.target = getFirstFree();
		load.param = 0;

		_Op(NumSub);
		_Walk;
		_In8 = load.target;
		_In8_2 = _First()->regTarget;
		_FreeChildren;
		freeReg(load.target);
		_Out;
		_Type = VariableType::Number;
	}break;

	case Token::Not: {
		_Op(Not);
		_Walk;
		_In8 = _First()->regTarget;
		_FreeChildren;
		_Out;
		_Type = VariableType::Boolean;
	}break;

	case Token::Less: {
		_Compare(Less)
	}break;

	case Token::LessEqual: {
		_Compare(LessEqual)
	}break;

	case Token::Larger: {
		_Compare(Greater)
	}break;

	case Token::LargerEqual: {
		_Compare(GreaterEqual)
	}break;

	case Token::Equal: {
		_Compare(Equal)
	}break;

	case Token::NotEqual: {
		_Compare(NotEqual)
	}break;

	case Token::And: {
		_Walk;
		_Type = VariableType::Boolean;
		_Op(And);
		_In8 = _First()->regTarget;
		_In8_2 = _Last()->regTarget;
		_FreeChildren;
		_Out;
	} break;
	case Token::Or: {
		_Walk;
		_Type = VariableType::Boolean;
		_Op(Or);
		_In8 = _First()->regTarget;
		_In8_2 = _Last()->regTarget;
		_FreeChildren;
		_Out;
	} break;

	case Token::Decrement:
	case Token::Increment: {
		WalkLoad(_First());
		if (_First()->varType != VariableType::Number) {
			_Error("Cannot increment");
		}
		_Op(PostMod);
		_In8 = _First()->regTarget;
		_In8_2 = n->type == Token::Increment ? 0 : 1;
		_Out;
		_FreeChildren;
		WalkStore(_First(), _In8);
		_FreeChildren;

	}break;

	case Token::PreDecrement:
	case Token::PreIncrement: {
		WalkLoad(_First());
		if (_First()->varType != VariableType::Number) {
			_Error("Cannot increment");
		}
		_Op(PreMod);
		_In8 = _First()->regTarget;
		_In8_2 = n->type == Token::PreIncrement ? 0 : 1;
		n->regTarget = instruction.target = _First()->regTarget;
	}break;

	case Token::Assign: {
		if (n->children.size() != 2) break;
		WalkLoad(_Last());
	assign:
		auto& lhs = _First();
		auto& rhs = _Last();

		if (IsConstant(rhs->type) || IsOperator(rhs->type)) {
			WalkStore(_First(), rhs->regTarget);
			n->regTarget = _SetOut(rhs) = lhs->regTarget;
			_FreeConstant(rhs);
		}
		else {
			if (rhs->sym && rhs->sym->needsLoading) freeReg(rhs->regTarget);
			_Op(Copy);
			_In8 = rhs->regTarget;
			n->regTarget = instruction.target = lhs->regTarget;
			WalkStore(_First(), rhs->regTarget);
		}

		if (lhs->sym && !isAssignable(lhs->sym->flags)) {
			_Error("Trying to assign to unassignable type");
			HasError = true;
		}
		if (rhs->varType != VariableType::Undefined && lhs->varType != rhs->varType && lhs->sym) {
			if (isTyped(lhs->sym->flags) || lhs->data.index() != 0) {
				_Error("Trying to assign unrelated types");
				HasError = true;
			}
			else if (lhs->varType == VariableType::Undefined) {
				lhs->sym->varType = _Type = lhs->varType = rhs->varType;
			}
		}
	}break;

	case Token::VarDeclare: {
		auto sym = currentScope->addSymbol(std::get<std::string>(n->data));
		sym->startLife = instructionList.size();
		sym->endLife = instructionList.size();
		sym->setType(SymbolType::Variable);
		sym->resolved = true;
		sym->needsLoading = false;
		if (n->children.size() > 0) {
			_Walk;
			sym->varType = _Type = _First()->varType;
			if (sym->varType != VariableType::Undefined) {
				sym->flags = sym->flags | SymbolFlags::Typed;
			}
			if (n->children.size() == 2) {
				if (_Last()->varType == _Type || _Type == VariableType::Undefined || _Last()->varType == VariableType::Undefined) {
					sym->varType = _Type = _Last()->varType;
					auto type = _Last()->type;
					if (IsConstant(type) || IsOperator(type)) {
						sym->reg = n->regTarget = _Last()->regTarget;
					}
					else {
						_Op(Copy);
						_In8 = _Last()->regTarget;
						sym->reg = _Out;
					}
					break;
				}
				_Error("Cannot assign unrelated types");
			}
		}
		if (sym->varType >= VariableType::Object) {
			_Op(PushObjectDefault);
			_In16 = (uint16)sym->varType - (uint16)VariableType::Object;
			sym->reg = _Out;
		}
		else {
			_Op(PushTypeDefault);
			_In16 = (uint16)sym->varType;
			sym->reg = _Out;
		}
	}break;

	case Token::Return: {
		_Walk;
		_Op(Return);
		if (!n->children.empty()) {
			instruction.target = _First()->regTarget;
			_In8 = 1;
		}
	} break;

	case Token::Conditional: {
		if (n->children.size() != 3) {
			_Error("Invalid ternary operator");
			break;
		}
		WalkLoad(n->children[0]);
		_FreeConstant(_First());

		auto& trueInst = instructionList.emplace_back(); 
		trueInst.code = OpCodes::JumpNeg;;
		trueInst.target = _First()->regTarget;
		trueInst.param = 0;
		auto falseStart = instructionList.size();

		WalkLoad(n->children[1]);
		_FreeConstant(n->children[1]);

		auto& falseInst = instructionList.emplace_back(); 
		falseInst.code = OpCodes::JumpForward;;
		falseInst.target = _First()->regTarget;
		falseInst.param = 0;
		auto trueStart = instructionList.size();


		auto falseTarget = instructionList.size();
		WalkLoad(n->children[2]);
		_FreeConstant(n->children[2]);
		auto trueTarget = instructionList.size();

		trueInst.param = falseTarget - falseStart;
		falseInst.param = trueTarget - trueStart;

		n->instruction = _SetOut(n->children[2]) = _SetOut(n->children[1]) = getFirstFree();

		if (n->children[1]->varType == n->children[2]->varType) {
			_Type = n->children[1]->varType;
		}

	} break;

	case Token::If: {
		if (n->children.size() != 3) {
			_Error("Incorrect if-block");
		}

		auto& next = currentScope->children.emplace_back();
		next.parent = currentScope;
		currentScope = &next;
		auto regs = registers;
		WalkLoad(_First());
		_Op(JumpNeg);
		n->regTarget = instruction.target = _First()->regTarget;
		_FreeConstant(n);

		auto oldSize = instructionList.size();
		auto it = n->children[1];
		WalkLoad(it);
		_FreeConstant(it);

		auto& elseinst = instructionList.emplace_back();
		elseinst.code = OpCodes::JumpForward;

		_In16 = instructionList.size() - oldSize;
		oldSize = instructionList.size();

		WalkLoad(_Last());
		_FreeConstant(_Last());

		elseinst.param = instructionList.size() - oldSize;

		currentScope = currentScope->parent;
		registers = regs;
	}break;

	case Token::For: {
		if (n->children.size() == 5) {
			auto& next = currentScope->children.emplace_back();
			next.parent = currentScope;
			currentScope = &next;
			auto regs = registers;
			WalkLoad(_First());

			auto start = instructionList.size();
			auto it = n->children[1];
			WalkLoad(it);
			_FreeConstant(it);

			_Op(JumpNeg);
			n->regTarget = instruction.target = it->regTarget;
			auto jumpStart = instructionList.size();

			auto target = n->children[3];
			WalkLoad(target);
			_FreeConstant(target);

			size_t loopend = instructionList.size();
			it = n->children[2];
			WalkLoad(it);
			_FreeConstant(it);

			auto& elseinst = instructionList.emplace_back();
			elseinst.code = OpCodes::JumpBackward;
			elseinst.param = instructionList.size() - start;

			size_t elsestart = instructionList.size();
			WalkLoad(_Last());
			_FreeConstant(_Last());

			instructionList[n->instruction].param = instructionList.size() - jumpStart;

			placeBreaks(n, loopend, elsestart);

			currentScope = currentScope->parent;
			registers = regs;
		}
		else if (n->children.size() == 3) {
			auto& next = currentScope->children.emplace_back();
			next.parent = currentScope;
			currentScope = &next;
			auto regs = registers;

			WalkLoad(_First());
			_FreeConstant(_First());

			auto sym = currentScope->addSymbol("_index_");
			sym->reg = getFirstFree();
			sym->setType(SymbolType::Static);
			sym->varType = VariableType::Number;
			{
				_Op(LoadImmediate);
				_In16 = (uint16)-1;
				sym->reg = _Out;
			}

			auto& data = std::get<std::string>(n->data);
			bool isVar = !data.empty();

			auto start = instructionList.size();
			uint8 regtarget = 0;
			if (isVar) {
				auto var = currentScope->addSymbol(data);
				var->reg = getFirstFree();
				var->setType(SymbolType::Variable);
				if (_First()->varType == VariableType::Number) var->varType = VariableType::Number;
				{
					_Op(RangeForVar);
					_In8 = sym->reg;
					_In8_2 = _First()->regTarget;
					_Out;
					regtarget = instruction.target;
				}
				{
					_Op(Noop);
					_In8 = var->reg;
				}
			} else {
				{
					_Op(RangeFor);
					_In8 = sym->reg;
					_In8_2 = _First()->regTarget;
					_Out;
					regtarget = instruction.target;
				}
			}

			_Op(JumpNeg);
			n->regTarget = instruction.target = regtarget;
			auto jumpStart = instructionList.size();

			auto target = n->children[1];
			WalkLoad(target);
			_FreeConstant(target);

			size_t loopend = instructionList.size();

			auto& elseinst = instructionList.emplace_back();
			elseinst.code = OpCodes::JumpBackward;
			elseinst.param = instructionList.size() - start;

			size_t elsestart = instructionList.size();
			WalkLoad(_Last());
			_FreeConstant(_Last());

			instructionList[n->instruction].param = instructionList.size() - jumpStart;

			placeBreaks(n, loopend, elsestart);

			currentScope = currentScope->parent;
			registers = regs;
		}
		else {
			_Error("Incorrect for-block");
		}
	}break;

	case Token::While: {
		if (n->children.size() != 2) {
			_Error("Incorrect while-block");
		}

		auto& next = currentScope->children.emplace_back();
		next.parent = currentScope;
		currentScope = &next;
		auto regs = registers;

		auto start = instructionList.size();
		WalkLoad(_First());
		_Op(JumpNeg);
		auto jumpStart = instructionList.size();
		n->regTarget = instruction.target = _First()->regTarget;

		auto it = n->children[1];
		WalkLoad(it);
		_FreeConstant(it);

		auto& elseinst = instructionList.emplace_back();
		elseinst.code = OpCodes::JumpBackward;
		elseinst.param = instructionList.size() - start;

		instructionList[n->instruction].param = instructionList.size() - jumpStart;

		placeBreaks(n, start, instructionList.size());

		currentScope = currentScope->parent;
		registers = regs;
	}break;

	case Token::Continue: 
	case Token::Break: {
		_Op(Jump);
	} break;

	case Token::Else: {
		_Walk;
	}break;

	case Token::FunctionCall: {
		int Type = 0;
		WalkLoad(_First());
		auto name = getFullId(_First());
		if (!_First()->sym) {

			if (auto it = HostFunctions().find(name); it != HostFunctions().end()) {
				_Type = TypeFromValue(it->second->return_type);
				Type = 1;
				goto function;
			}
			if (IntrinsicFunctions.find(name) != IntrinsicFunctions.end()) {
				_Type = IntrinsicFunctionTypes[name][0];
				Type = 2;
				goto function;
			}
			Type = 4;
			goto function;
		}
		else if (_First()->sym->type != SymbolType::Function && _First()->sym->type == SymbolType::Variable) {
			Type = 3;
			goto function;
		}
		else {
			if (_First()->sym->type == SymbolType::Function) {
			// @todo: Get return type for user functions

			}
		}
		function:

		uint8 last = 255;
		auto& params = _Last();
		for (auto& c : params->children) {
			WalkLoad(c);
			if (IsConstant(c->type) || IsOperator(c->type) || c->regTarget == last + 1) {
				freeReg(c->regTarget);
				last = _SetOut(c) = getLastFree();
			}
			else {
				_Op(Copy);
				instruction.target = getLastFree();
				_In8 = c->regTarget;
				last = c->regTarget = instruction.target;
			}
		}

		for (auto& c : params->children) {
			freeReg(c->regTarget);
		}
		_FreeConstant(_First());

		auto it = std::find(currentFunction->FunctionTableSymbols.begin(), currentFunction->FunctionTableSymbols.end(), name);
		size_t index = 0;

		if (it != currentFunction->FunctionTableSymbols.end()) {
			index = it - currentFunction->FunctionTableSymbols.begin();
		}
		else {
			index = currentFunction->FunctionTableSymbols.size();
			currentFunction->FunctionTableSymbols.push_back(name);
		}

		_Op(CallFunction);
		
		switch (Type)
		{
		case 0: instruction.code = OpCodes::CallFunction; break;
		case 1: instruction.code = OpCodes::CallExternal; break;
		case 2: instruction.code = OpCodes::CallInternal; break;
		case 3: instruction.code = OpCodes::CallSymbol; break;
		case 4: instruction.code = OpCodes::Call; break;
		}

		if (params->children.size() != 0) {
			_In8 = (uint8)params->children.front()->regTarget;
		}
		else {
			_In8 = getLastFree();
		}
		_In8_2 = params->children.size();
		_Out;

		auto& arg = instructionList.emplace_back();
		arg.code = OpCodes::Noop;

		switch (Type)
		{
		case 0: arg.data = (uint32)index; break;
		case 1: arg.data = (uint32)index; break;
		case 2: arg.data = (uint32)index; break;
		case 3: arg.target = _First()->regTarget; break;
		case 4: arg.data = (uint32)index; break;
		}
	}break;

	default:
		_Error("Unexpected token found");
		break;
	}
}

void ASTWalker::WalkStore(Node* n, uint8 reg) {
	Function* f = currentFunction;
	if (HasDebug) f->DebugLines[(int)n->line] = (int)instructionList.size();
	auto first_child = n->children.size() ? n->children.front() : nullptr;
	auto last_child = n->children.size() ? n->children.back() : nullptr;
	switch (n->type) {
	case Token::Id: {
		_Walk;
		bool isNameSpace = false;
		Symbol* symbol = nullptr;
		auto& data = std::get<std::string>(n->data);
		if (n->children.size() != 1) {
			symbol = findSymbol(data, "", isNameSpace);
		}
		else {
			if (_First()->sym) {
				if (_First()->sym->type == SymbolType::Namespace) {
					symbol = findSymbol(data, std::get<std::string>(_First()->data), isNameSpace);
				}
				else if (_First()->sym->type == SymbolType::Variable) {

					UserDefinedType* typeProperty = nullptr;
					int typeIndex = (int)_First()->varType - (int)VariableType::Object;
					if (typeIndex >= 0 && typeIndex < currentFunction->TypeTableSymbols.size()) {
						auto& objectName = currentFunction->TypeTableSymbols[typeIndex];
						if (GetManager().GetType(typeProperty, objectName)) {
							n->varType = typeProperty->GetFieldType(data);
						}
						else if (auto localObject = currentNamespace->objects.find(objectName); localObject != currentNamespace->objects.end()) {
							n->varType = localObject->second.GetFieldType(data);
						}
					}

					// @todo: Maybe type checking here?

					_Op(StoreProperty);
					_In8 = _First()->regTarget;
					_SetOut(n) = reg;
					size_t index = 0;
					auto it = std::find(currentFunction->PropertyTableSymbols.begin(), currentFunction->PropertyTableSymbols.end(), data);
					if (it != currentFunction->PropertyTableSymbols.end()) {
						index = it - currentFunction->PropertyTableSymbols.begin();
					}
					else {
						index = currentFunction->PropertyTableSymbols.size();
						currentFunction->PropertyTableSymbols.push_back(data);
					}

					auto& arg = instructionList.emplace_back();
					arg.code = OpCodes::Noop;
					arg.param = static_cast<uint16>(index);

					break;
				}
				else {
					_Error("Unknown symbol: " + data);
				}
			}
			else {
				_Error("Unknown symbol: " + data);
			}
		}
		if (!symbol || (symbol && symbol->needsLoading)) {
			auto name = getFullId(n);
			auto it = std::find(currentFunction->GlobalTableSymbols.begin(), currentFunction->GlobalTableSymbols.end(), name);
			size_t index = 0;

			if (it != currentFunction->GlobalTableSymbols.end()) {
				index = it - currentFunction->GlobalTableSymbols.begin();
			}
			else {
				index = currentFunction->GlobalTableSymbols.size();
				currentFunction->GlobalTableSymbols.push_back(name);
				currentFunction->GlobalTable.push_back(nullptr);
			}

			if (symbol) {
				_Type = symbol->varType;
				n->sym = symbol;
			}

			_Op(StoreSymbol);
			_In16 = index;
			_SetOut(n) = reg;
		}
		else if (symbol) {
			n->regTarget = symbol->reg;
			symbol->endLife = instructionList.size();
			_Type = symbol->varType;
			n->sym = symbol;
		}
	} break;

	case Token::Indexer: {
		_Walk;
		_Op(StoreIndex);
		//n->sym = _First()->sym;
		_In8 = _First()->regTarget;
		_In8_2 = _Last()->regTarget;
		_FreeConstant(_Last());
		_SetOut(n) = reg;
	}break;

	case Token::Number:
	case Token::Literal:
	case Token::True:
	case Token::False:
	case Token::TypeBoolean:
	case Token::TypeString:
	case Token::TypeFunction:
		_Error("Invalid left-hand side"); // @todo: Add rest invalid sides;
		break;


	default: {
		WalkLoad(n);
	}
	}
}

Symbol* ASTWalker::findSymbol(const std::string& name, const std::string& space, bool& isNamespace)
{
	if (auto it = namespaces.find(name); it != namespaces.end()) {
		isNamespace = true;
		return it->second.Sym;
	}
	if (space.size() == 0) {
		if (auto s = currentNamespace->findSymbol(name); s != nullptr) return s;
		if (auto s = namespaces["Global"].findSymbol(name); s != nullptr) return s;
		if (auto s = vm->FindSymbol(name, currentNamespace->Name, isNamespace); s != nullptr) return s;
		if (currentScope) if (auto s = currentScope->findSymbol(name); s != nullptr) return s;
		return nullptr;
	}
	else {
		if (auto it = namespaces.find(space); it != namespaces.end()) {
			if (auto s = it->second.findSymbol(name); s != nullptr) return s;
			if (auto s = vm->FindSymbol(name, space, isNamespace); s != nullptr) return s;
		}
		return nullptr;
	}
}

void ASTWalker::handleFunction(Node* n, Function* f, Symbol* s)
{
	initRegs();
	currentFunction = f;
	currentNamespace = &namespaces[f->Namespace];

	for (auto& c : n->children) {
		switch (c->type)
		{
		case Token::CallParams: {
			for (auto& v : c->children) {
				auto symParam = f->FunctionScope->findSymbol(std::get<0>(v->data));
				if (symParam) symParam->reg = getFirstFree();
			}
		} break;

		case Token::Scope: {

			instructionList.reserve(c->children.size() * n->depth);
			currentScope = f->FunctionScope;
			for (auto& stmt : c->children) {
				try {
					WalkLoad(stmt);
					if (IsConstant(stmt->type) || IsOperator(stmt->type) || (stmt->sym && stmt->sym->needsLoading)) 
						freeReg(stmt->regTarget);;
				}
				catch (...) {
					gError() << "Internal error occured!\n";
				}
			}
			Instruction op;
			op.code = OpCodes::Return;
			op.in1 = 0;
			instructionList.emplace_back(op);

		} break;

		default:
			break;
		}
	}

	f->Bytecode.resize(instructionList.size());
	for (int i = 0; i < instructionList.size(); i++) {
		f->Bytecode[i] = instructionList[i].data;
	}

	f->FunctionTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->ExternalTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->IntrinsicTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->PropertyTable.resize(f->PropertyTableSymbols.size(), -1);
	f->TypeTable.resize(f->TypeTableSymbols.size(), VariableType::Undefined);


	f->StringTable.reserve(stringList.size());
	for (auto& str : stringList) {
		f->StringTable.emplace_back(String::GetAllocator()->Make(str.c_str()));
	}
	stringList.clear();

	f->RegisterCount = maxRegister + 1;
	s->resolved = true;
	instructionList.clear();

	currentFunction = nullptr;
	currentScope = nullptr;

	// Cleanup
	delete f->FunctionScope;
	f->FunctionScope = nullptr;
}

void ASTWalker::placeBreaks(Node* n, size_t start, size_t end)
{
	for (auto& node : n->children) {
		switch (node->type)
		{
		case Token::Break: {
			instructionList[node->instruction].param = (uint16)end;
		} break;
		case Token::Continue: {
			instructionList[node->instruction].param = (uint16)start;
		} break;
		default:
			break;
		}
		placeBreaks(node, start, end);
	}
}
