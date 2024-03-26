#include "AST.h"
#include "TypeConverters.h"
#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "VM.h"
#include <charconv>

constexpr unsigned char print_first[] = { 195, 196, 196, 0 };
constexpr unsigned char print_last[] = { 192, 196, 196, 0 };
constexpr unsigned char print_add[] = { 179, 32, 32, 32, 0 };

#ifdef DEBUG
#define X(x) { OpCodes::x, #x },
std::unordered_map<OpCodes, std::string> OpcodeNames = {
#include "Opcodes.h"
};
#endif // DEBUG

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

Variable Node::ToVariable() const
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
	return (t >= Token::Equal && t <= Token::Div) || (t >= Token::FunctionCall && t <= Token::Property);
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
				i = n->children.erase(i);
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
			case Token::FunctionCall: return true;
			case Token::Indexer: return true;
			case Token::Property: return true;
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
	case Token::Number: {
		n = 0.f;
		auto& var = std::get<double>(n);
		#ifndef _MSC_VER
		char* end;
		double value = std::strtod(h.data.data(), &end);
		if (end != h.data.data() + h.data.size()) {
			var = 0.0;
		} else {
			var = value;
		}
		#else 
		std::from_chars(h.data.data(), h.data.data() + h.data.size(), var);
		#endif
	} break;
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
	Vm = in_vm;
	Root = n;
	auto it = Namespaces.emplace("Global", Namespace{});
	it.first->second.Name = "Global";
	CurrentNamespace = &it.first->second;
	CurrentNamespace->Sym = new Symbol();
	CurrentNamespace->Sym->setType(SymbolType::Namespace);
	CurrentFunction = nullptr;
	CurrentScope = nullptr;
	HasError = false;
	MaxRegister = 0;

	HasDebug = true; // @todo: disable debugs
}

ASTWalker::~ASTWalker()
{
	delete Root;
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
	for (auto& c : Root->children) {
		switch (c->type)
		{
		case Token::NamespaceDef:
		{
			CurrentNamespace = &Namespaces[std::get<0>(c->data)];
			CurrentNamespace->Name = std::get<0>(c->data);
			auto sym = new Symbol();
			sym->setType(SymbolType::Namespace);
			sym->Resolved = true;
			CurrentNamespace->Sym = sym;
		} break;
		
		case Token::ObjectDef:
		{
			auto& data = std::get<0>(c->data);
			bool isNameSpace;
			auto s = FindSymbol(data, "", isNameSpace);
			if (!s && !isNameSpace) {
				auto it = CurrentNamespace->Symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				symbol->setType(SymbolType::Object);
				auto& object = CurrentNamespace->Objects.emplace(data, UserDefinedType{}).first->second;
				
				for (auto& field : c->children) {
					Symbol flags;
					flags.Flags = SymbolFlags::None;
					Variable var;

					if (field->children.size() == 2) {
						if (field->children.front()->type != Token::AnyType) {
							flags.Flags = SymbolFlags::Typed;
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
							default: break;
						}
						flags.VarType = type;
						if (IsConstant(field->children.back()->type)) {
							if (TypesMatch(field->children.front()->type, field->children.back()->type)) {
								var = field->children.back()->ToVariable(); // @todo: Add implicit conversions
							}
							else {
								gError() << "Trying to initialize with wrong type: In object " << data << ", field " << std::get<0>(field->data) << '\n';
							}
						}
						else {
							var = GetTypeDefault(type);
						}
					} 
					else {
						gError() << "Parse error in object " << data << '\n';
					}
					
					object.AddField(std::get<0>(field->data), var, flags);
				}

				symbol->Resolved = true;
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
			auto s = FindSymbol(data, "", isNameSpace);
			if (!s && !isNameSpace) {
				auto it = CurrentNamespace->Symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				symbol->setType(SymbolType::Function);
				symbol->VarType = VariableType::Function;
				symbol->NeedsLoading = true;
				Function* function = new Function();
				function->Name = data;
				function->IsPublic = c->type == Token::PublicFunctionDef || CurrentNamespace->Name == "Global";
				CurrentNamespace->Functions.emplace(function->Name.c_str(), function).first->second;
				c->sym = symbol;
				function->Namespace = CurrentNamespace->Name;
				function->NamespaceHash = ankerl::unordered_dense::hash<std::basic_string<char>>()(CurrentNamespace->Name);
				functionList.emplace_back(c, function);
				function->Types.resize(1);
				function->FunctionScope = new Scoped();
				CurrentFunction = function;

				for (auto& node : c->children) {
					switch (node->type)
					{
					case Token::Scope: break;
					case Token::CallParams: {
						function->ArgCount = (uint8_t)node->children.size();
						for (auto& v : node->children) {
							auto symParam = FindSymbol(std::get<0>(v->data), "", isNameSpace);
							if (symParam || isNameSpace) {
								gError() << "Line " << node->line << ": Symbol '" << std::get<0>(v->data) << "' already defined\n";
								HasError = true;
								break;
							}
							symParam = function->FunctionScope->addSymbol(std::get<0>(v->data));
							WalkLoad(v->children.front());
							symParam->setType(SymbolType::Variable);
							symParam->VarType = v->children.front()->varType;
							if (symParam->VarType != VariableType::Undefined) {
								symParam->Flags = symParam->Flags | SymbolFlags::Typed;
							}
							symParam->Resolved = true;
							function->Types.push_back(symParam->VarType);
						}
					} break;
					default:
						WalkLoad(node);
						function->Types[0] = symbol->VarType;
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
			auto s = FindSymbol(data, "", isNameSpace);
			if (!s && !isNameSpace) {
				auto name = CurrentNamespace->Name + '.' + data;
				auto it = CurrentNamespace->Symbols.emplace(data, new Symbol{});
				auto& symbol = it.first->second;
				switch (c->type)
				{
				case Token::Static: symbol->setType(SymbolType::Static); break;
				case Token::Const: symbol->setType(SymbolType::Static); break;
				case Token::VarDeclare: symbol->setType(SymbolType::Variable); break;
				default: break;
				}
				auto& variable = CurrentNamespace->Variables.emplace(data, Variable{}).first->second;

				symbol->StartLife = (size_t)-1;
				symbol->NeedsLoading = true;

				switch (c->children[0]->type)
				{
				case Token::TypeNumber:
					symbol->VarType = VariableType::Number;
					if (c->children.size() == 2) variable = VariantToFloat(c->children[1]);
					else variable = 0.0;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::TypeBoolean:
					symbol->VarType = VariableType::Boolean;
					if (c->children.size() == 2) variable = VariantToBool(c->children[1]);
					else variable = false;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::TypeString:
					symbol->VarType = VariableType::String;
					if (c->children.size() == 2) variable = String::GetAllocator()->Make(VariantToStr(c->children[1]).c_str());
					else String::GetAllocator()->Make("");
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::Typename:
					symbol->VarType = VariableType::Object;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					// @todo: make objects;
					break;
				case Token::TypeArray:
					symbol->VarType = VariableType::Array;
					if (c->children.size() == 2) variable = Array::GetAllocator()->Make(c->children[1]->children.size());
					else variable = Array::GetAllocator()->Make(0);
					// @todo: Initialize global array
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::AnyType:
					if (c->children.size() == 2) {
						switch (c->children[1]->type)
						{
						case Token::Number:
							symbol->VarType = VariableType::Number;
							if (c->children.size() == 2) variable = VariantToFloat(c->children[1]);
							else variable = 0.0;
							break;
						case Token::True:
						case Token::False:
							symbol->VarType = VariableType::Boolean;
							if (c->children.size() == 2) variable = VariantToBool(c->children[1]);
							else variable = c->children[1]->type == Token::True;
							break;
						case Token::Literal:
							symbol->VarType = VariableType::String;
							if (c->children.size() == 2) variable = String::GetAllocator()->Make(VariantToStr(c->children[1]).c_str());
							else variable = String::GetAllocator()->Make("");
							break;
						case Token::Array:
							symbol->VarType = VariableType::Array;
							if (c->children.size() == 2) variable = Array::GetAllocator()->Make(c->children[1]->children.size());
							else variable = Array::GetAllocator()->Make(0);
							break;
						default:
							symbol->VarType = VariableType::Undefined;
							break;
						}
					}
					break;
				default: break;
				}
			}
		} break;

		default:
			break;
		}
	}

	for (auto& [node, function] : functionList) {
		HandleFunction(node, function, node->sym);
		gDebug() << "Generated function '" << function->Name << "', used " << MaxRegister + 1 << " registers and " << function->Bytecode.size() << " instructions\n";

	}
}

#ifdef DEBUG
void printInstruction(const Instruction& in) {
	gLogger() << "Target: " << (int)in.target << ", In: " << (int)in.in1 << ", In2: " << (int)in.in2 << ", Param: " << (int)in.param << ", \t" << "Code: " << OpcodeNames[in.code] << '\n';
}
#endif // DEBUG

#define Op(op) n->instruction = InstructionList.size(); auto& instruction = InstructionList.emplace_back(); instruction.code = OpCodes::op; 
#define Walk for(auto& child : n->children) WalkLoad(child);
#define In16 instruction.param
#define In8 instruction.in1
#define In8_2 instruction.in2
#define Out n->regTarget = instruction.target = GetFirstFree()
#define FreeChildren for (auto& c : n->children) { if (!c->sym  || (c->sym && c->sym->NeedsLoading) ) FreeRegister(c->regTarget); }
#define NodeType n->varType 
#define SetOut(n) n->regTarget = InstructionList[n->instruction].target
#define First() first_child
#define Last() last_child
#define Error(text) gError() << "Line " << n->line << ": " << text << "\n"; HasError = true;
#define Warn(text) gWarn() << "Line " << n->line << ": " << text << "\n";
#define FreeConstant(n) if (!n->sym || (n->sym && n->sym->NeedsLoading) ) FreeRegister(n->regTarget);
#define EnsureOperands if (n->children.size() != 2) { Error("Invalid number of operands") break; }

#define Operator(varTy, op, op2) \
case VariableType::varTy: {\
	if (Last()->varType != VariableType::Undefined && Last()->varType != VariableType::varTy) {\
		instruction.code = OpCodes::op2; \
	}\
	else { \
		instruction.code = OpCodes::op; \
	} \
	NodeType = VariableType::varTy;\
	In8 = First()->regTarget;\
	In8_2 = Last()->regTarget;\
	FreeChildren;\
} break;

#define OperatorAssign Out;

#define Compare(op) \
Walk;\
if (First()->varType != Last()->varType && First()->varType != VariableType::Undefined && Last()->varType != VariableType::Undefined) {\
	Error("Can only compare similar types");\
}\
NodeType = VariableType::Boolean;\
Op(op);\
In8 = First()->regTarget;\
In8_2 = Last()->regTarget;\
FreeChildren;\
Out;\

void ASTWalker::WalkLoad(Node* n)
{
	Function* f = CurrentFunction;
	if (HasDebug) f->DebugLines[(int)n->line] = (int)InstructionList.size();
	Node* first_child = n->children.size() ? n->children.front() : nullptr;
	Node* last_child = n->children.size() ? n->children.back() : nullptr;
	switch (n->type) {
	case Token::Scope: {
		auto& next = CurrentScope->children.emplace_back();
		next.parent = CurrentScope;
		CurrentScope = &next;
		auto regs = Registers;
		for (auto& c : n->children) {
			WalkLoad(c);
			FreeConstant(c);
		}
		CurrentScope = CurrentScope->parent;
		Registers = regs;
	} break;

	case Token::TypeNumber: {
		NodeType = VariableType::Number;
	}break;
	case Token::TypeBoolean: {
		NodeType = VariableType::Boolean;
	}break;
	case Token::TypeString: {
		NodeType = VariableType::String;
	}break;
	case Token::TypeArray: {
		NodeType = VariableType::Array;
	}break;
	case Token::TypeFunction: {
		NodeType = VariableType::Function;
	}break;
	case Token::AnyType: {
		NodeType = VariableType::Undefined;
	}break;
	case Token::Typename: {
		auto& name = std::get<std::string>(n->data);
		auto it = std::find(CurrentFunction->TypeTableSymbols.begin(), CurrentFunction->TypeTableSymbols.end(), name);
		size_t index = 0;
		if (it == CurrentFunction->TypeTableSymbols.end()) {
			index = CurrentFunction->TypeTableSymbols.size();
			CurrentFunction->TypeTableSymbols.push_back(name);
		}
		else {
			index = it - CurrentFunction->TypeTableSymbols.begin();
		}
		NodeType = static_cast<VariableType>(static_cast<size_t>(VariableType::Object) + index);
	}break;

	case Token::Number: {
		Op(LoadNumber);
		auto res = f->NumberTable.emplace(std::get<double>(n->data));
		uint16_t idx = (uint16_t)std::distance(f->NumberTable.begin(), res.first);
		In16 = idx;
		Out;
		NodeType = VariableType::Number;
	}break;

	case Token::Literal: {
		Op(LoadString);
		auto res = StringList.emplace(std::get<std::string>(n->data));
		uint16_t idx = (uint16_t)std::distance(StringList.begin(), res.first);
		In16 = idx;
		Out;
		NodeType = VariableType::String;
	}break;

	case Token::Null: {
		Op(PushUndefined);
		Out;
	} break;

	case Token::Array: {
		{
			Op(PushArray);
			In16 = n->children.size();
			Out;
		}
		for (auto& c : n->children) {
			WalkLoad(c);
			Op(PushIndex);
			instruction.target = n->regTarget;
			In8 = c->regTarget;
			FreeConstant(c);
		}
		FreeChildren;
		{
			Op(Copy);
			In8 = n->regTarget;
			Out;
		}
		NodeType = VariableType::Array;
	}break;

	case Token::Indexer: {
		Walk;
		Op(LoadIndex);
		//n->sym = First()->sym;
		In8 = First()->regTarget;
		In8_2 = Last()->regTarget;
		FreeChildren;
		Out;
	}break;

	case Token::True: {
		Op(PushBoolean);
		Out;
		In8 = 1;
		NodeType = VariableType::Boolean;
	}break;
	case Token::False: {
		Op(PushBoolean);
		Out;
		In8 = 0;
		NodeType = VariableType::Boolean;
	}break;

	case Token::Add: {
		EnsureOperands;
		Walk;
		Op(NumAdd);
		switch (First()->varType)
		{
			Operator(Number, NumAdd, Add)
				Operator(String, StrAdd, Add)
		default:
			NodeType = VariableType::Undefined;
			instruction.code = OpCodes::Add;
			In8 = First()->regTarget;
			In8_2 = Last()->regTarget;
			FreeChildren;
			break;
		}
		OperatorAssign;
	}break;
	case Token::Sub: {
		EnsureOperands;
		Walk;
		Op(NumAdd);
		switch (First()->varType)
		{
			Operator(Number, NumSub, Sub);
		default:
			NodeType = VariableType::Undefined;
			instruction.code = OpCodes::Sub;
			In8 = First()->regTarget;
			In8_2 = Last()->regTarget;
			FreeChildren;
			break;
		}
		OperatorAssign;

	}break;
	case Token::Div: {
		EnsureOperands;
		Walk;
		Op(NumAdd);
		switch (First()->varType)
		{
			Operator(Number, NumDiv, Div);
		default:
			NodeType = VariableType::Undefined;
			instruction.code = OpCodes::Div;
			In8 = First()->regTarget;
			In8_2 = Last()->regTarget;
			FreeChildren;
			break;
		}
		OperatorAssign;

	}break;
	case Token::Mult: {
		EnsureOperands;
		Walk;
		Op(NumAdd);
		switch (First()->varType)
		{
			Operator(Number, NumMul, Mul);
		default:
			NodeType = VariableType::Undefined;
			instruction.code = OpCodes::Mul;
			In8 = First()->regTarget;
			In8_2 = Last()->regTarget;
			FreeChildren;
			break;
		}
		OperatorAssign;

	}break;

	case Token::AssignAdd: {
		n->type = Token::Add;
		WalkLoad(n);
		Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;
	case Token::AssignSub: {
		n->type = Token::Sub;
		WalkLoad(n);
		Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;
	case Token::AssignDiv: {
		n->type = Token::Div;
		WalkLoad(n);
		Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;
	case Token::AssignMult: {
		n->type = Token::Mult;
		WalkLoad(n);
		Last() = n;
		n->type = Token::Assign;
		goto assign;
	} break;

	case Token::Id: {
		Walk;
		bool isNameSpace = false;
		Symbol* symbol = nullptr;
		auto& data = std::get<std::string>(n->data);
		if (n->children.size() != 1) {
			symbol = FindSymbol(data, "", isNameSpace);
		}
		else {
			if (First()->sym) {
				if (First()->sym->Type == SymbolType::Namespace) {
					symbol = FindSymbol(data, std::get<std::string>(First()->data), isNameSpace);
				}
				else if (First()->sym->Type == SymbolType::Variable) {
					Op(LoadProperty);
					In8 = First()->regTarget;
					FreeChildren;
					Out;
					size_t index = 0;
					auto it = std::find(CurrentFunction->PropertyTableSymbols.begin(), CurrentFunction->PropertyTableSymbols.end(), data);
					if (it != CurrentFunction->PropertyTableSymbols.end()) {
						index = it - CurrentFunction->PropertyTableSymbols.begin();
					}
					else {
						index = CurrentFunction->PropertyTableSymbols.size();
						CurrentFunction->PropertyTableSymbols.push_back(data);
					}

					auto& arg = InstructionList.emplace_back();
					arg.code = OpCodes::Noop;
					arg.param = static_cast<uint16_t>(index);

					UserDefinedType* typeProperty = nullptr;
					size_t typeIndex = (size_t)First()->varType - (size_t)VariableType::Object;
					if (typeIndex < CurrentFunction->TypeTableSymbols.size()) {
						auto& objectName = CurrentFunction->TypeTableSymbols[typeIndex];
						if (GetManager().GetType(typeProperty, objectName)) {
							n->varType = typeProperty->GetFieldType(data);
						}
						else if (auto localObject = CurrentNamespace->Objects.find(objectName); localObject != CurrentNamespace->Objects.end()) {
							n->varType = localObject->second.GetFieldType(data);
						}
					}
					break;
				}
				else {
					Error("Unknown symbol: " + data);
				}
			}
			else {
				Error("Unknown symbol: " + data);
			}
		}
		if (!symbol || (symbol && symbol->NeedsLoading)) {
			auto name = getFullId(n);
			auto it = std::find(CurrentFunction->GlobalTableSymbols.begin(), CurrentFunction->GlobalTableSymbols.end(), name);
			size_t index = 0;

			if (it != CurrentFunction->GlobalTableSymbols.end()) {
				index = it - CurrentFunction->GlobalTableSymbols.begin();
			}
			else {
				index = CurrentFunction->GlobalTableSymbols.size();
				CurrentFunction->GlobalTableSymbols.push_back(name);
				CurrentFunction->GlobalTable.push_back(nullptr);
			}

			if (symbol) {
				NodeType = symbol->VarType;
				n->sym = symbol;
			}

			Op(LoadSymbol);
			In16 = index;
			Out;
		}
		else if (symbol) {
			n->regTarget = symbol->Register;
			symbol->EndLife = InstructionList.size();
			NodeType = symbol->VarType;
			n->sym = symbol;
		}
	}break;

	case Token::Property: {
		auto& data = std::get<std::string>(n->data);
		if (n->children.size() != 1) {
			Error("Invalid property access: " + data);
			break;
		}
		Walk;
		if (First()->varType > VariableType::Object || First()->varType == VariableType::Undefined) {
			Op(LoadProperty);
			In8 = First()->regTarget;
			Out;
			size_t index = 0;
			auto it = std::find(CurrentFunction->PropertyTableSymbols.begin(), CurrentFunction->PropertyTableSymbols.end(), data);
			if (it != CurrentFunction->PropertyTableSymbols.end()) {
				index = it - CurrentFunction->PropertyTableSymbols.begin();
			}
			else {
				index = CurrentFunction->PropertyTableSymbols.size();
				CurrentFunction->PropertyTableSymbols.push_back(data);
			}

			auto& arg = InstructionList.emplace_back();
			arg.code = OpCodes::Noop;
			arg.param = static_cast<uint16_t>(index);

			UserDefinedType* typeProperty = nullptr;
			size_t typeIndex = (size_t)First()->varType - (size_t)VariableType::Object;
			if (typeIndex < CurrentFunction->TypeTableSymbols.size()) {
				auto& objectName = CurrentFunction->TypeTableSymbols[typeIndex];
				if (GetManager().GetType(typeProperty, objectName)) {
					n->varType = typeProperty->GetFieldType(data);
				}
				else if (auto localObject = CurrentNamespace->Objects.find(objectName); localObject != CurrentNamespace->Objects.end()) {
					n->varType = localObject->second.GetFieldType(data);
				}
			}
			break;
		}
		else {
			Error("Unknown symbol: " + data);
		}
	} break;

	case Token::Negate: {
		auto& var = First()->varType;
		if (var != VariableType::Number && var != VariableType::Undefined) {
			Error("Cannot negate non number types");
		}
		auto& load = InstructionList.emplace_back();
		load.code = OpCodes::LoadImmediate;
		load.target = GetFirstFree();
		load.param = 0;

		Op(NumSub);
		Walk;
		In8 = load.target;
		In8_2 = First()->regTarget;
		FreeChildren;
		FreeRegister(load.target);
		Out;
		NodeType = VariableType::Number;
	}break;

	case Token::Not: {
		Walk;
		Op(Not);
		In8 = First()->regTarget;
		FreeChildren;
		Out;
		NodeType = VariableType::Boolean;
	}break;

	case Token::Less: {
		Compare(Less)
	}break;

	case Token::LessEqual: {
		Compare(LessEqual)
	}break;

	case Token::Larger: {
		Compare(Greater)
	}break;

	case Token::LargerEqual: {
		Compare(GreaterEqual)
	}break;

	case Token::Equal: {
		Compare(Equal)
	}break;

	case Token::NotEqual: {
		Compare(NotEqual)
	}break;

	case Token::And: {
		Walk;
		NodeType = VariableType::Boolean;
		Op(And);
		In8 = First()->regTarget;
		In8_2 = Last()->regTarget;
		FreeChildren;
		Out;
	} break;
	case Token::Or: {
		Walk;
		NodeType = VariableType::Boolean;
		Op(Or);
		In8 = First()->regTarget;
		In8_2 = Last()->regTarget;
		FreeChildren;
		Out;
	} break;

	case Token::Decrement:
	case Token::Increment: {
		WalkLoad(First());
		if (First()->varType != VariableType::Number) {
			Error("Cannot increment");
		}
		Op(PostMod);
		In8 = First()->regTarget;
		In8_2 = n->type == Token::Increment ? 0 : 1;
		Out;
		FreeChildren;
		In8 = WalkStore(First()); // @todo: this is unsafe
		FreeChildren;
		n->sym = First()->sym;
	}break;

	case Token::PreDecrement:
	case Token::PreIncrement: {
		WalkLoad(First());
		if (First()->varType != VariableType::Number) {
			Error("Cannot increment");
		}
		Op(PreMod);
		In8 = First()->regTarget;
		In8_2 = n->type == Token::PreIncrement ? 0 : 1;
		n->regTarget = instruction.target = WalkStore(First());
		FreeChildren;
		n->sym = First()->sym;
	}break;

	case Token::Assign: {
		if (n->children.size() != 2) break;
		WalkLoad(Last());
	assign:
		auto& lhs = First();
		auto& rhs = Last();

		if (!rhs->sym || (rhs->sym && rhs->sym->NeedsLoading)) {
			WalkStore(lhs);
			FreeConstant(rhs);
			n->regTarget = SetOut(rhs) = lhs->regTarget;
		}
		else {
			if (rhs->sym && rhs->sym->NeedsLoading) FreeRegister(rhs->regTarget);
			Op(Copy);
			In8 = rhs->regTarget;
			SetOut(n) = WalkStore(lhs);
		}
		n->sym = lhs->sym;
		if (lhs->sym && !isAssignable(lhs->sym->Flags)) {
			Error("Trying to assign to unassignable type");
			HasError = true;
		}
		if (rhs->varType != VariableType::Undefined && lhs->varType != rhs->varType && lhs->sym) {
			if (isTyped(lhs->sym->Flags) || lhs->data.index() != 0) {
				Error("Trying to assign unrelated types");
				HasError = true;
			}
			else if (lhs->varType == VariableType::Undefined) {
				lhs->sym->VarType = NodeType = lhs->varType = rhs->varType;
			}
		}
	}break;

	case Token::Const:
	case Token::VarDeclare: {
		bool isNamespace;
		if (FindSymbol(std::get<std::string>(n->data), "", isNamespace)) {
			Error("Symbol already defined: " + std::get<std::string>(n->data));
			break;
		}
		auto sym = CurrentScope->addSymbol(std::get<std::string>(n->data));
		sym->StartLife = InstructionList.size();
		sym->EndLife = InstructionList.size();
		sym->setType(n->type == Token::VarDeclare ? SymbolType::Variable : SymbolType::Static);
		sym->Resolved = true;
		sym->NeedsLoading = false;
		n->sym = sym;
		if (n->children.size() > 0) {
			Walk;
			sym->VarType = NodeType = First()->varType;
			if (sym->VarType != VariableType::Undefined) {
				sym->Flags = sym->Flags | SymbolFlags::Typed;
			}
			if (n->children.size() == 2) {
				if (Last()->varType == NodeType || NodeType == VariableType::Undefined || Last()->varType == VariableType::Undefined) {
					sym->VarType = NodeType = Last()->varType;
					if (!Last()->sym) {
						sym->Register = n->regTarget = Last()->regTarget;
					}
					else {
						Op(Copy);
						In8 = Last()->regTarget;
						sym->Register = Out;
					}
					break;
				}
				Error("Cannot assign unrelated types");
			}
		}
		if (sym->VarType >= VariableType::Object) {
			Op(PushObjectDefault);
			In16 = (uint16_t)sym->VarType - (uint16_t)VariableType::Object;
			sym->Register = Out;
		}
		else {
			Op(PushTypeDefault);
			In16 = (uint16_t)sym->VarType;
			sym->Register = Out;
		}
	}break;

	case Token::ObjectInit: {
		uint8_t last = 255;
		auto name = getFullId(First());
		for (auto& c : Last()->children) {
			WalkLoad(c);
			if (!c->sym || c->regTarget == last + 1) {
				FreeRegister(c->regTarget);
				last = SetOut(c) = GetLastFree();
			}
			else {
				Op(Copy);
				instruction.target = GetLastFree();
				In8 = c->regTarget;
				last = c->regTarget = instruction.target;
			}
		}

		for (auto& c : Last()->children) {
			FreeRegister(c->regTarget);
		}

		auto it = std::find(CurrentFunction->TypeTableSymbols.begin(), CurrentFunction->TypeTableSymbols.end(), name);
		size_t index = 0;

		if (it != CurrentFunction->TypeTableSymbols.end()) {
			index = it - CurrentFunction->TypeTableSymbols.begin();
		}
		else {
			index = CurrentFunction->TypeTableSymbols.size();
			CurrentFunction->TypeTableSymbols.push_back(name);
		}
		Op(InitObject);

		if (Last()->children.size() != 0) {
			In8 = (uint8_t)Last()->children.front()->regTarget;
		}
		else {
			In8 = GetLastFree();
		}
		In8_2 = Last()->children.size();
		Out;

		auto& arg = InstructionList.emplace_back();
		arg.code = OpCodes::Noop;
		arg.param = (uint16_t)index;

	} break;

	case Token::Return: {
		Walk;
		Op(Return);
		if (!n->children.empty()) {
			instruction.target = First()->regTarget;
			In8 = 1;
		}
	} break;

	case Token::Conditional: {
		if (n->children.size() != 3) {
			Error("Invalid ternary operator");
			break;
		}
		WalkLoad(n->children[0]);
		FreeConstant(First());

		auto& trueInst = InstructionList.emplace_back(); 
		trueInst.code = OpCodes::JumpNeg;;
		trueInst.target = First()->regTarget;
		trueInst.param = 0;
		auto falseStart = InstructionList.size();

		WalkLoad(n->children[1]);
		FreeConstant(n->children[1]);

		auto& falseInst = InstructionList.emplace_back(); 
		falseInst.code = OpCodes::JumpForward;;
		falseInst.target = First()->regTarget;
		falseInst.param = 0;
		auto trueStart = InstructionList.size();


		auto falseTarget = InstructionList.size();
		WalkLoad(n->children[2]);
		FreeConstant(n->children[2]);
		auto trueTarget = InstructionList.size();

		trueInst.param = falseTarget - falseStart;
		falseInst.param = trueTarget - trueStart;

		n->instruction = SetOut(n->children[2]) = SetOut(n->children[1]) = GetFirstFree();

		if (n->children[1]->varType == n->children[2]->varType) {
			NodeType = n->children[1]->varType;
		}

	} break;

	case Token::If: {
		if (n->children.size() != 3) {
			Error("Incorrect if-block");
		}

		auto& next = CurrentScope->children.emplace_back();
		next.parent = CurrentScope;
		CurrentScope = &next;
		auto regs = Registers;
		WalkLoad(First());
		Op(JumpNeg);
		n->regTarget = instruction.target = First()->regTarget;
		FreeConstant(n);

		auto oldSize = InstructionList.size();
		auto it = n->children[1];
		WalkLoad(it);
		FreeConstant(it);

		size_t elseloc = InstructionList.size();
		auto& elseinst = InstructionList.emplace_back();
		elseinst.code = OpCodes::JumpForward;

		InstructionList[n->instruction].param = InstructionList.size() - oldSize;
		oldSize = InstructionList.size();

		WalkLoad(Last());
		FreeConstant(Last());

		InstructionList[elseloc].param = InstructionList.size() - oldSize;

		CurrentScope = CurrentScope->parent;
		Registers = regs;
	}break;

	case Token::For: {
		if (n->children.size() == 5) {
			auto& next = CurrentScope->children.emplace_back();
			next.parent = CurrentScope;
			CurrentScope = &next;
			auto regs = Registers;
			WalkLoad(First());

			auto start = InstructionList.size();
			auto it = n->children[1];
			WalkLoad(it);
			FreeConstant(it);

			Op(JumpNeg);
			n->regTarget = instruction.target = it->regTarget;
			auto jumpStart = InstructionList.size();

			auto target = n->children[3];
			WalkLoad(target);
			FreeConstant(target);

			size_t loopend = InstructionList.size();
			it = n->children[2];
			WalkLoad(it);
			FreeConstant(it);

			auto& elseinst = InstructionList.emplace_back();
			elseinst.code = OpCodes::JumpBackward;
			elseinst.param = InstructionList.size() - start;

			size_t elsestart = InstructionList.size();
			WalkLoad(Last());
			FreeConstant(Last());

			InstructionList[n->instruction].param = InstructionList.size() - jumpStart;

			PlaceBreaks(n, loopend, elsestart);

			CurrentScope = CurrentScope->parent;
			Registers = regs;
		}
		else if (n->children.size() == 3) {
			auto& next = CurrentScope->children.emplace_back();
			next.parent = CurrentScope;
			CurrentScope = &next;
			auto regs = Registers;

			WalkLoad(First());

			auto sym = CurrentScope->addSymbol("_index_");
			sym->Register = GetFirstFree();
			sym->setType(SymbolType::Static);
			sym->VarType = VariableType::Number;
			{
				Op(LoadImmediate);
				In16 = (uint16_t)-1;
				n->regTarget = instruction.target = sym->Register;
			}

			auto& data = std::get<std::string>(n->data);
			bool isVar = !data.empty();

			auto start = InstructionList.size();
			uint8_t regtarget = 0;
			if (isVar) {
				auto var = CurrentScope->addSymbol(data);
				var->Register = GetFirstFree();
				var->setType(SymbolType::Variable);
				if (First()->varType == VariableType::Number) var->VarType = VariableType::Number;
				{
					Op(RangeForVar);
					In8 = sym->Register;
					In8_2 = First()->regTarget;
					Out;
					regtarget = instruction.target;
				}
				{
					Op(Noop);
					In8 = var->Register;
				}
			} else {
				{
					Op(RangeFor);
					In8 = sym->Register;
					In8_2 = First()->regTarget;
					Out;
					regtarget = instruction.target;
				}
			}

			Op(JumpNeg);
			n->regTarget = instruction.target = regtarget;
			auto jumpStart = InstructionList.size();

			auto target = n->children[1];
			WalkLoad(target);
			FreeConstant(target);

			size_t loopend = InstructionList.size();

			auto& elseinst = InstructionList.emplace_back();
			elseinst.code = OpCodes::JumpBackward;
			elseinst.param = InstructionList.size() - start;

			size_t elsestart = InstructionList.size();
			WalkLoad(Last());
			FreeConstant(Last());

			InstructionList[n->instruction].param = InstructionList.size() - jumpStart;

			PlaceBreaks(n, loopend, elsestart);

			CurrentScope = CurrentScope->parent;
			Registers = regs;
		}
		else {
			Error("Incorrect for-block");
		}
	}break;

	case Token::While: {
		if (n->children.size() != 2) {
			Error("Incorrect while-block");
		}

		auto& next = CurrentScope->children.emplace_back();
		next.parent = CurrentScope;
		CurrentScope = &next;
		auto regs = Registers;

		auto start = InstructionList.size();
		WalkLoad(First());
		Op(JumpNeg);
		auto jumpStart = InstructionList.size();
		n->regTarget = instruction.target = First()->regTarget;

		auto it = n->children[1];
		WalkLoad(it);
		FreeConstant(it);

		auto& elseinst = InstructionList.emplace_back();
		elseinst.code = OpCodes::JumpBackward;
		elseinst.param = InstructionList.size() - start;

		InstructionList[n->instruction].param = InstructionList.size() - jumpStart;

		PlaceBreaks(n, start, InstructionList.size());

		CurrentScope = CurrentScope->parent;
		Registers = regs;
	}break;

	case Token::Continue: 
	case Token::Break: {
		Op(Jump);
	} break;

	case Token::Else: {
		Walk;
	}break;

	case Token::FunctionCall: {
		int type = 0;
		WalkLoad(First());
		auto name = getFullId(First());
		if (!First()->sym) {

			if (auto it = HostFunctions().find(name); it != HostFunctions().end()) {
				NodeType = TypeFromValue(it->second->return_type);
				type = 1;
				goto function;
			}
			if (IntrinsicFunctions.find(name) != IntrinsicFunctions.end()) {
				NodeType = IntrinsicFunctionTypes[name][0];
				type = 2;
				goto function;
			}
			type = 3; // @todo: This should be 4, fix 
			goto function;
		}
		else if (First()->sym->Type != SymbolType::Function && First()->sym->Type == SymbolType::Variable) {
			type = 3;
			goto function;
		}
		else {
			if (First()->sym->Type == SymbolType::Function) {
			// @todo: Get return type for user functions

			}
		}
		function:

		uint8_t last = 255;
		auto& params = Last();
		for (auto& c : params->children) {
			WalkLoad(c);
			if (!c->sym || c->regTarget == last + 1) {
				FreeRegister(c->regTarget);
				last = SetOut(c) = GetLastFree();
			}
			else {
				Op(Copy);
				instruction.target = GetLastFree();
				In8 = c->regTarget;
				last = c->regTarget = instruction.target;
			}
		}

		for (auto& c : params->children) {
			FreeRegister(c->regTarget);
		}
		FreeConstant(First());

		auto it = std::find(CurrentFunction->FunctionTableSymbols.begin(), CurrentFunction->FunctionTableSymbols.end(), name);
		size_t index = 0;

		if (it != CurrentFunction->FunctionTableSymbols.end()) {
			index = it - CurrentFunction->FunctionTableSymbols.begin();
		}
		else {
			index = CurrentFunction->FunctionTableSymbols.size();
			CurrentFunction->FunctionTableSymbols.push_back(name);
		}

		Op(CallFunction);
		
		switch (type)
		{
		case 0: instruction.code = OpCodes::CallFunction; break;
		case 1: instruction.code = OpCodes::CallExternal; break;
		case 2: instruction.code = OpCodes::CallInternal; break;
		case 3: instruction.code = OpCodes::CallSymbol; break;
		case 4: instruction.code = OpCodes::Call; break;
		}

		if (params->children.size() != 0) {
			In8 = (uint8_t)params->children.front()->regTarget;
		}
		else {
			In8 = GetLastFree();
		}
		In8_2 = params->children.size();
		Out;

		auto& arg = InstructionList.emplace_back();
		arg.code = OpCodes::Noop;

		switch (type)
		{
		case 0: arg.data = (uint32_t)index; break;
		case 1: arg.data = (uint32_t)index; break;
		case 2: arg.data = (uint32_t)index; break;
		case 3: arg.target = First()->regTarget; break;
		case 4: arg.data = (uint32_t)index; break;
		}
	}break;

	default:
		Error("Unexpected token found");
		break;
	}
}

uint8_t ASTWalker::WalkStore(Node* n) {
	Function* f = CurrentFunction;
	if (HasDebug) f->DebugLines[(int)n->line] = (int)InstructionList.size();
	auto first_child = n->children.size() ? n->children.front() : nullptr;
	auto last_child = n->children.size() ? n->children.back() : nullptr;
	switch (n->type) {
	case Token::Id: {
		Walk;
		bool isNameSpace = false;
		Symbol* symbol = nullptr;
		auto& data = std::get<std::string>(n->data);
		if (n->children.size() != 1) {
			symbol = FindSymbol(data, "", isNameSpace);
		}
		else {
			if (First()->sym) {
				if (First()->sym->Type == SymbolType::Namespace) {
					symbol = FindSymbol(data, std::get<std::string>(First()->data), isNameSpace);
				}
				else if (First()->sym->Type == SymbolType::Variable) {

					UserDefinedType* typeProperty = nullptr;
					size_t typeIndex = (size_t)First()->varType - (size_t)VariableType::Object;
					if (typeIndex < CurrentFunction->TypeTableSymbols.size()) {
						auto& objectName = CurrentFunction->TypeTableSymbols[typeIndex];
						if (GetManager().GetType(typeProperty, objectName)) {
							n->varType = typeProperty->GetFieldType(data);
						}
						else if (auto localObject = CurrentNamespace->Objects.find(objectName); localObject != CurrentNamespace->Objects.end()) {
							n->varType = localObject->second.GetFieldType(data);
						}
					}

					// @todo: Maybe type checking here?

					Op(StoreProperty);
					In8 = First()->regTarget;
					Out;
					size_t index = 0;
					auto it = std::find(CurrentFunction->PropertyTableSymbols.begin(), CurrentFunction->PropertyTableSymbols.end(), data);
					if (it != CurrentFunction->PropertyTableSymbols.end()) {
						index = it - CurrentFunction->PropertyTableSymbols.begin();
					}
					else {
						index = CurrentFunction->PropertyTableSymbols.size();
						CurrentFunction->PropertyTableSymbols.push_back(data);
					}

					auto& arg = InstructionList.emplace_back();
					arg.code = OpCodes::Noop;
					arg.param = static_cast<uint16_t>(index);

					return n->regTarget;
				}
				else {
					Error("Unknown symbol: " + data);
				}
			}
			else {
				Error("Unknown symbol: " + data);
			}
		}
		if (!symbol || (symbol && symbol->NeedsLoading)) {
			auto name = getFullId(n);
			auto it = std::find(CurrentFunction->GlobalTableSymbols.begin(), CurrentFunction->GlobalTableSymbols.end(), name);
			size_t index = 0;

			if (it != CurrentFunction->GlobalTableSymbols.end()) {
				index = it - CurrentFunction->GlobalTableSymbols.begin();
			}
			else {
				index = CurrentFunction->GlobalTableSymbols.size();
				CurrentFunction->GlobalTableSymbols.push_back(name);
				CurrentFunction->GlobalTable.push_back(nullptr);
			}

			if (symbol) {
				NodeType = symbol->VarType;
				n->sym = symbol;
			}

			Op(StoreSymbol);
			In16 = index;
			Out;
			return n->regTarget;
		}
		else if (symbol) {
			n->regTarget = symbol->Register;
			symbol->EndLife = InstructionList.size();
			NodeType = symbol->VarType;
			n->sym = symbol;
			return n->regTarget;
		}
	} break;

	case Token::Indexer: {
		Walk;
		Op(StoreIndex);
		//n->sym = First()->sym;
		In8 = First()->regTarget;
		In8_2 = Last()->regTarget;
		FreeConstant(Last());
		Out;
		return n->regTarget;
	}break;

	case Token::Number:
	case Token::Literal:
	case Token::True:
	case Token::False:
	case Token::TypeBoolean:
	case Token::TypeString:
	case Token::TypeFunction:
		Error("Invalid left-hand side"); // @todo: Add rest invalid sides;
		break;


	default: {
		WalkLoad(n);
		return n->regTarget;
	}
	}
	return n->regTarget;
}

Symbol* ASTWalker::FindSymbol(const std::string& name, const std::string& space, bool& isNamespace)
{
	if (auto it = Namespaces.find(name); it != Namespaces.end()) {
		isNamespace = true;
		return it->second.Sym;
	}
	if (space.size() == 0) {
		if (auto s = CurrentNamespace->FindSymbol(name); s != nullptr) return s;
		if (auto s = Namespaces["Global"].FindSymbol(name); s != nullptr) return s;
		if (auto s = Vm->FindSymbol(name, CurrentNamespace->Name, isNamespace); s != nullptr) return s;
		if (CurrentScope) if (auto s = CurrentScope->FindSymbol(name); s != nullptr) return s;
		return nullptr;
	}
	else {
		if (auto it = Namespaces.find(space); it != Namespaces.end()) {
			if (auto s = it->second.FindSymbol(name); s != nullptr) return s;
			if (auto s = Vm->FindSymbol(name, space, isNamespace); s != nullptr) return s;
		}
		return nullptr;
	}
}

void ASTWalker::HandleFunction(Node* n, Function* f, Symbol* s)
{
	InitRegisters();
	CurrentFunction = f;
	CurrentNamespace = &Namespaces[f->Namespace];

	for (auto& c : n->children) {
		switch (c->type)
		{
		case Token::CallParams: {
			for (auto& v : c->children) {
				auto symParam = f->FunctionScope->FindSymbol(std::get<0>(v->data));
				if (symParam) symParam->Register = GetFirstFree();
			}
		} break;

		case Token::Scope: {

			InstructionList.reserve(c->children.size() * n->depth);
			CurrentScope = f->FunctionScope;
			for (auto& stmt : c->children) {
				try {
					WalkLoad(stmt);
					FreeConstant(stmt);
				}
				catch (...) {
					gError() << "Internal error occured!\n";
				}
			}
			Instruction op;
			op.code = OpCodes::Return;
			op.in1 = 0;
			InstructionList.emplace_back(op);

		} break;

		default:
			break;
		}
	}

	f->Bytecode.resize(InstructionList.size());
	for (size_t i = 0; i < InstructionList.size(); i++) {
		f->Bytecode[i] = InstructionList[i].data;
	}

#ifdef DEBUG
	gDebug() << "Function " << f->Name << '\n';
	gLogger() << "----------------------------------\n";
	for (auto& in : InstructionList) {
		printInstruction(in);
	}
	gLogger() << "\n\n";
#endif // DEBUG

	f->FunctionTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->ExternalTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->IntrinsicTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->PropertyTable.resize(f->PropertyTableSymbols.size(), -1);
	f->TypeTable.resize(f->TypeTableSymbols.size(), VariableType::Undefined);


	f->StringTable.reserve(StringList.size());
	for (auto& str : StringList) {
		f->StringTable.emplace_back(String::GetAllocator()->Make(str.c_str()));
	}
	StringList.clear();

	f->RegisterCount = MaxRegister + 1;
	s->Resolved = true;
	InstructionList.clear();

	CurrentFunction = nullptr;
	CurrentScope = nullptr;

	// Cleanup
	delete f->FunctionScope;
	f->FunctionScope = nullptr;
}

void ASTWalker::PlaceBreaks(Node* n, size_t start, size_t end)
{
	for (auto& node : n->children) {
		switch (node->type)
		{
		case Token::Break: {
			InstructionList[node->instruction].param = (uint16_t)end;
		} break;
		case Token::Continue: {
			InstructionList[node->instruction].param = (uint16_t)start;
		} break;
		case Token::For:
		case Token::While: break;
		default:
			PlaceBreaks(node, start, end);
			break;
		}
	}
}
