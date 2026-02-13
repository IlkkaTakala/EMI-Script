#include "AST.h"
#include "Helpers.h"
#include "Objects/StringObject.h"
#include "Objects/ArrayObject.h"
#include "VM.h"
#include <charconv>

#ifdef DEBUG

#define X(x) { OpCodes::x, #x },
std::unordered_map<OpCodes, std::string> OpcodeNames = {
#include "Opcodes.h"
};
#endif // DEBUG

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
	auto var = CopyToVM(v);
	return var;
}

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
			Node* l = nullptr, * r = nullptr;
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
		}
		else {
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

ASTWalker::ASTWalker(VM* in_vm, Node* n, const std::string& file)
{
	Vm = in_vm;
	Root = n;
	CurrentFunction = nullptr;
	CurrentScope = nullptr;
	HasError = false;
	MaxRegister = 0;
	SearchPaths.resize(1);
	Filename = file;

	InitFunction = new ScriptFunction();
	InitFunction->FunctionScope = new ScopeType();

	HasDebug = true; // @todo: disable debugs

	for (int i = 0; i < (int)Token::Last; ++i) {
		TokenJumpTable[i] = &ASTWalker::handle_default;
	}

	TokenJumpTable[(int)Token::Scope] = &ASTWalker::handle_Scope;
	TokenJumpTable[(int)Token::TypeNumber] = &ASTWalker::handle_TypeNumber;
	TokenJumpTable[(int)Token::TypeBoolean] = &ASTWalker::handle_TypeBoolean;
	TokenJumpTable[(int)Token::TypeString] = &ASTWalker::handle_TypeString;
	TokenJumpTable[(int)Token::TypeArray] = &ASTWalker::handle_TypeArray;
	TokenJumpTable[(int)Token::TypeFunction] = &ASTWalker::handle_TypeFunction;
	TokenJumpTable[(int)Token::AnyType] = &ASTWalker::handle_AnyType;
	TokenJumpTable[(int)Token::Typename] = &ASTWalker::handle_Typename;
	TokenJumpTable[(int)Token::Number] = &ASTWalker::handle_Number;
	TokenJumpTable[(int)Token::Literal] = &ASTWalker::handle_Literal;
	TokenJumpTable[(int)Token::Null] = &ASTWalker::handle_Null;
	TokenJumpTable[(int)Token::Array] = &ASTWalker::handle_Array;
	TokenJumpTable[(int)Token::Indexer] = &ASTWalker::handle_Indexer;
	TokenJumpTable[(int)Token::True] = &ASTWalker::handle_True;
	TokenJumpTable[(int)Token::False] = &ASTWalker::handle_False;
	TokenJumpTable[(int)Token::Add] = &ASTWalker::handle_Add;
	TokenJumpTable[(int)Token::Sub] = &ASTWalker::handle_Sub;
	TokenJumpTable[(int)Token::Div] = &ASTWalker::handle_Div;
	TokenJumpTable[(int)Token::Mult] = &ASTWalker::handle_Mult;
	TokenJumpTable[(int)Token::AssignAdd] = &ASTWalker::handle_AssignAdd;
	TokenJumpTable[(int)Token::AssignSub] = &ASTWalker::handle_AssignSub;
	TokenJumpTable[(int)Token::AssignDiv] = &ASTWalker::handle_AssignDiv;
	TokenJumpTable[(int)Token::AssignMult] = &ASTWalker::handle_AssignMult;
	TokenJumpTable[(int)Token::Id] = &ASTWalker::handle_Id;
	TokenJumpTable[(int)Token::Property] = &ASTWalker::handle_Property;
	TokenJumpTable[(int)Token::Negate] = &ASTWalker::handle_Negate;
	TokenJumpTable[(int)Token::Not] = &ASTWalker::handle_Not;
	TokenJumpTable[(int)Token::Less] = &ASTWalker::handle_Less;
	TokenJumpTable[(int)Token::LessEqual] = &ASTWalker::handle_LessEqual;
	TokenJumpTable[(int)Token::Larger] = &ASTWalker::handle_Larger;
	TokenJumpTable[(int)Token::LargerEqual] = &ASTWalker::handle_LargerEqual;
	TokenJumpTable[(int)Token::Equal] = &ASTWalker::handle_Equal;
	TokenJumpTable[(int)Token::NotEqual] = &ASTWalker::handle_NotEqual;
	TokenJumpTable[(int)Token::And] = &ASTWalker::handle_And;
	TokenJumpTable[(int)Token::Or] = &ASTWalker::handle_Or;
	TokenJumpTable[(int)Token::Decrement] = &ASTWalker::handle_Increment;
	TokenJumpTable[(int)Token::Increment] = &ASTWalker::handle_Increment;
	TokenJumpTable[(int)Token::PreDecrement] = &ASTWalker::handle_PreIncrement;
	TokenJumpTable[(int)Token::PreIncrement] = &ASTWalker::handle_PreIncrement;
	TokenJumpTable[(int)Token::Assign] = &ASTWalker::handle_Assign;
	TokenJumpTable[(int)Token::Const] = &ASTWalker::handle_VarDeclare;
	TokenJumpTable[(int)Token::VarDeclare] = &ASTWalker::handle_VarDeclare;
	TokenJumpTable[(int)Token::ObjectInit] = &ASTWalker::handle_ObjectInit;
	TokenJumpTable[(int)Token::Return] = &ASTWalker::handle_Return;
	TokenJumpTable[(int)Token::Conditional] = &ASTWalker::handle_Conditional;
	TokenJumpTable[(int)Token::If] = &ASTWalker::handle_If;
	TokenJumpTable[(int)Token::For] = &ASTWalker::handle_For;
	TokenJumpTable[(int)Token::While] = &ASTWalker::handle_While;
	TokenJumpTable[(int)Token::Continue] = &ASTWalker::handle_Break;
	TokenJumpTable[(int)Token::Break] = &ASTWalker::handle_Break;
	TokenJumpTable[(int)Token::Else] = &ASTWalker::handle_Else;
	TokenJumpTable[(int)Token::FunctionCall] = &ASTWalker::handle_FunctionCall;
}

ASTWalker::~ASTWalker()
{
	delete InitFunction;

	for (auto& n : Root->children) {
		delete n->sym;
	}
	delete Root;

	for (auto& [name, sym] : Global.Table) {
		delete sym;
	}
	Global.Table.clear();
}

PathType getFullId(Node* n) {
	PathType full;
	full << std::get<0>(n->data).c_str();
	for (auto& c : n->children) {
		if (c->type != Token::Id) break;
		full << getFullId(c);
	}
	return full;
}

void ASTWalker::Run()
{
	std::vector<std::pair<Node*, ScriptFunction*>> functionList;
	std::vector<std::string> ImportList;
	CurrentFunction = InitFunction;
	for (auto& c : Root->children) {
		switch (c->type)
		{
		case Token::NamespaceDef:
		{
			PathType name = getFullId(c->children.front());
			if (name.toString() != "Global") {

				if (name.Get(name.Length() - 1).toString() == "Global") {
					SearchPaths[0] = PathType();
					name = name.PopLast();
				}

				if (SearchPaths[0].Length() + name.Length() > PathType::MaxLength() - 1) {
					HasError = true;
					gCompileError() << "Maximum namespace depth reached with " << name.Append(SearchPaths[0]) << "!";
				}

				PathType temp(name);
				while (temp.Length() > 0) {
					auto [n, s] = FindSymbol(temp);
					if (!s) {
						auto [nn, sym] = FindOrCreateSymbol(temp, SymbolType::Namespace);
						Namespace* space = new Namespace();
						space->Name = nn;
						sym->Space = space;
					}
					temp = temp.Pop();
				}
				SearchPaths[0] = name.Append(SearchPaths[0]);
			}
			else {
				SearchPaths[0] = PathType();
			}
		} break;

		case Token::UsingDef: {

			PathType name = getFullId(c->children.front());

			AllSearchPaths.emplace_back(c->line, name);
			SearchPaths.push_back(name);

		} break;

		case Token::ImportDef: {

			std::string path = std::get<std::string>(c->data);
			ImportList.push_back(path);

			Vm->LoadLibrary(path.c_str());

		} break;

		case Token::ObjectDef:
		{
			PathType data(std::get<0>(c->data).c_str());
			auto [name, s] = FindSymbol(data);
			if (!s) {
				auto symbol = new Symbol{};
				PathType addedName(data.Append(SearchPaths[0]));
				if (!Global.AddName(addedName, symbol)) {
					delete symbol;
					break;
				}
				symbol->setType(SymbolType::Object);
				auto object = new UserDefinedType{};
				symbol->UserObject = object;

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
								flags.VarType = var.getType();
							}
							else {
								gCompileError() << "Trying to initialize with wrong type: In object " << addedName << ", field " << std::get<0>(field->data);
							}
						}
						else if (field->children.back()->type != Token::OExpr && field->children.back()->type != Token::Null) {
							gCompileWarn() << "Cannot initialize fields with non-constant values. Reverting to default";
							var.setUndefined();
						}
					}
					else {
						gCompileError() << "Parse error in object " << addedName;
					}
					NameType fieldName(std::get<0>(field->data).c_str());
					object->AddField(fieldName, var, flags);
				}
			}
			else {
				gCompileError() << "Line " << c->line << ": Symbol '" << data << "' already defined";
			}
		} break;

		case Token::PublicFunctionDef:
		case Token::FunctionDef:
		{
			PathType data(std::get<0>(c->data).c_str());

			PathTypeQuery query(data, SearchPaths);
			auto [name, s] = Global.FindName(query);

			Symbol* symbol = nullptr;
			if (!name) {
				symbol = new Symbol{};
				Global.AddName(data.Append(SearchPaths[0]), symbol);
				symbol->setType(SymbolType::Function);
				symbol->VarType = VariableType::Function;
				symbol->Function = new FunctionTable();
			}
			else {
				symbol = s;
			}

			FunctionSymbol* functionSym = new FunctionSymbol();
			ScriptFunction* function = new ScriptFunction();
			functionSym->Type = FunctionType::User;
			functionSym->Local = function;

			function->Name = data.Append(SearchPaths[0]);
			function->IsPublic = c->type == Token::PublicFunctionDef || SearchPaths[0] == PathType();
			c->sym = new CompileSymbol();
			c->sym->Global = true;
			c->sym->Sym = symbol;
			functionList.emplace_back(c, function);
			function->FunctionScope = new ScopeType();
			CurrentFunction = function;

			for (auto& node : c->children) {
				switch (node->type)
				{
				case Token::Scope: break;
				case Token::CallParams: {
					function->ArgCount = (uint8_t)node->children.size();
					for (auto& v : node->children) {
						auto [paramName, symParam] = FindSymbol(std::get<0>(v->data).c_str());
						if (paramName) {
							gCompileError() << "Line " << node->line << ": Symbol '" << paramName << "' already defined";
							HasError = true;
							break;
						}
						CompileSymbol* sym = function->FunctionScope->addSymbol(std::get<0>(v->data).c_str());
						WalkLoad(v->children.front());
						sym->Sym = new Symbol();
						sym->Sym->setType(SymbolType::Variable);
						sym->Sym->VarType = v->children.front()->varType;
						if (sym->Sym->VarType != VariableType::Undefined) {
							sym->Sym->Flags = sym->Sym->Flags | SymbolFlags::Typed;
						}
						sym->Resolved = true;
						functionSym->Signature.Arguments.push_back(sym->Sym->VarType);
						functionSym->Signature.ArgumentNames.push_back(std::get<0>(v->data).c_str());
					}
				} break;
				default:
					WalkLoad(node);
					functionSym->Signature.Return = node->varType;
					break;
				}
			}

			symbol->Function->AddFunction(function->ArgCount, functionSym);

		} break;

		case Token::Static:
		case Token::Const:
		case Token::VarDeclare:
		{
			PathType data(std::get<0>(c->data).c_str());
			auto [name, s] = FindSymbol(data);
			if (!name) {
				auto symbol = new Symbol();
				Global.AddName(data.Append(SearchPaths[0]), symbol);
				switch (c->type)
				{
				case Token::Static: symbol->setType(SymbolType::Static); break;
				case Token::Const: symbol->setType(SymbolType::Static); break;
				case Token::VarDeclare: symbol->setType(SymbolType::Variable); break;
				default: break;
				}
				Variable* variable = new Variable();

				symbol->SimpleVariable = variable;

				switch (c->children[0]->type)
				{
				case Token::TypeNumber:
					symbol->VarType = VariableType::Number;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::TypeBoolean:
					symbol->VarType = VariableType::Boolean;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::TypeString:
					symbol->VarType = VariableType::String;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::Typename:
					symbol->VarType = VariableType::Object;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					// @todo: make objects;
					break;
				case Token::TypeArray:
					symbol->VarType = VariableType::Array;
					symbol->Flags = symbol->Flags | SymbolFlags::Typed;
					break;
				case Token::AnyType:
					if (c->children.size() == 2) {
						switch (c->children[1]->type)
						{
						case Token::Number:
							symbol->VarType = VariableType::Number;
							break;
						case Token::True:
						case Token::False:
							symbol->VarType = VariableType::Boolean;
							break;
						case Token::Literal:
							symbol->VarType = VariableType::String;
							break;
						case Token::Array:
							symbol->VarType = VariableType::Array;
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

	HandleInit();

	for (auto& [node, function] : functionList) {
		HandleFunction(node, function, node->sym);
		gCompileInfo() << "Generated function '" << function->Name << "', used " << MaxRegister + 1 << " registers and " << function->Bytecode.size() << " instructions\n";
	}
}

#ifdef DEBUG
void printInstruction(const Instruction& in) {
	gCompileLogger() << "Target: " << (int)in.target << ", In: " << (int)in.in1 << ", In2: " << (int)in.in2 << ", Param: " << (int)in.param << ", \t" << "Code: " << OpcodeNames[in.code] << '\n';
}
#endif // DEBUG

#define Op(op) n->instruction = InstructionList.size(); auto& instruction = InstructionList.emplace_back(); instruction.code = OpCodes::op; if (HasDebug) CurrentFunction->DebugLines[(int)n->instruction] = (int)n->line;
#define Walk for(auto& child : n->children) (this->*TokenJumpTable[(int)child->type])(child);
#define WalkOne(node) (this->*TokenJumpTable[(int)node->type])(node);
#define In16 instruction.param
#define In8 instruction.in1
#define In8_2 instruction.in2
#define Out n->regTarget = instruction.target = GetFirstFree()
#define FreeChildren for (auto& c : n->children) { if (!c->sym  || (c->sym && c->sym->NeedsLoading) ) FreeRegister(c->regTarget); }
#define NodeType n->varType 
#define SetOut(n) n->regTarget = InstructionList[n->instruction].target
#define Error(text) gCompileError() << Filename << ":" << n->line << ": " << text; HasError = true;
#define Warn(text) gWarn() << "Line " << n->line << ": " << text;
#define FreeConstant(n) if (!n->sym || (n->sym && n->sym->NeedsLoading) ) FreeRegister(n->regTarget);
#define EnsureOperands if (n->children.size() != 2) { Error("Invalid number of operands") return; }
#define GetFirstNode() Node* first = n->children.size() ? n->children.front() : nullptr;
#define GetLastNode() Node* last = n->children.size() ? n->children.back() : nullptr;

#define Operator(varTy, op, op2) \
case VariableType::varTy: {\
	if (last->varType != VariableType::Undefined && last->varType != VariableType::varTy) {\
		instruction.code = OpCodes::op2; \
	}\
	else { \
		instruction.code = OpCodes::op; \
	} \
	NodeType = VariableType::varTy;\
	In8 = first->regTarget;\
	In8_2 = last->regTarget;\
	FreeChildren;\
} break;

#define OperatorAssign Out;

#define Compare(op) \
Walk;\
if (first->varType != last->varType && first->varType != VariableType::Undefined && last->varType != VariableType::Undefined) {\
	Error("Can only compare similar types");\
}\
NodeType = VariableType::Boolean;\
Op(op);\
In8 = first->regTarget;\
In8_2 = last->regTarget;\
FreeChildren;\
Out;\

void ASTWalker::WalkLoad(Node* n)
{
	(this->*TokenJumpTable[(int)n->type])(n);
}

void ASTWalker::handle_default(Node* n)
{
	Error("No handler for token " << TokensToName[n->type]);
}

void ASTWalker::handle_Scope(Node* n) {
	auto& next = CurrentScope->children.emplace_back();
	next.parent = CurrentScope;
	CurrentScope = &next;
	auto regs = Registers;
	for (auto& c : n->children) {
		WalkOne(c);
		FreeConstant(c);
	}
	CurrentScope = CurrentScope->parent;
	Registers = regs;
}

void ASTWalker::handle_TypeNumber(Node* n) {
	NodeType = VariableType::Number;
}
void ASTWalker::handle_TypeBoolean(Node* n) {
	NodeType = VariableType::Boolean;
}
void ASTWalker::handle_TypeString(Node* n) {
	NodeType = VariableType::String;
}
void ASTWalker::handle_TypeArray(Node* n) {
	NodeType = VariableType::Array;
}
void ASTWalker::handle_TypeFunction(Node* n) {
	NodeType = VariableType::Function;
}
void ASTWalker::handle_AnyType(Node* n) {
	NodeType = VariableType::Undefined;
}
void ASTWalker::handle_Typename(Node* n) {
	PathType name(std::get<std::string>(n->data).c_str());
	auto it = std::find(CurrentFunction->TypeTableSymbols.begin(), CurrentFunction->TypeTableSymbols.end(), name);
	size_t index = 0;
	if (it == CurrentFunction->TypeTableSymbols.end()) {
		index = CurrentFunction->TypeTableSymbols.size();
		CurrentFunction->TypeTableSymbols.push_back({ name, SearchPaths });
	}
	else {
		index = it - CurrentFunction->TypeTableSymbols.begin();
	}
	NodeType = static_cast<VariableType>(static_cast<size_t>(VariableType::Object) + index);
}

void ASTWalker::handle_Number(Node* n) {
	Op(LoadNumber);
	auto res = CurrentFunction->NumberTable.emplace(std::get<double>(n->data));
	uint16_t idx = (uint16_t)std::distance(CurrentFunction->NumberTable.begin(), res.first);
	In16 = idx;
	Out;
	NodeType = VariableType::Number;
}

void ASTWalker::handle_Literal(Node* n) {
	Op(LoadString);
	auto res = StringList.emplace(std::get<std::string>(n->data));
	uint16_t idx = (uint16_t)std::distance(StringList.begin(), res.first);
	In16 = idx;
	Out;
	NodeType = VariableType::String;
}

void ASTWalker::handle_Null(Node* n) {
	Op(PushUndefined);
	Out;
}

void ASTWalker::handle_Array(Node* n) {
	{
		Op(PushArray);
		In16 = n->children.size();
		Out;
	}
	for (auto& c : n->children) {
		WalkOne(c);
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
}

void ASTWalker::handle_Indexer(Node* n) {
	GetFirstNode()
	GetLastNode()
	Walk;
	Op(LoadIndex);
	//n->sym = first->sym;
	In8 = first->regTarget;
	In8_2 = last->regTarget;
	FreeChildren;
	Out;
}

void ASTWalker::handle_True(Node* n) {
	Op(PushBoolean);
	Out;
	In8 = 1;
	NodeType = VariableType::Boolean;
}
void ASTWalker::handle_False(Node* n) {
	Op(PushBoolean);
	Out;
	In8 = 0;
	NodeType = VariableType::Boolean;
}

void ASTWalker::handle_Add(Node* n) {
	GetFirstNode()
	GetLastNode()
	EnsureOperands;
	Walk;
	Op(NumAdd);
	switch (first->varType)
	{
		Operator(Number, NumAdd, Add)
			Operator(String, StrAdd, Add)
	default:
		NodeType = VariableType::Undefined;
		instruction.code = OpCodes::Add;
		In8 = first->regTarget;
		In8_2 = last->regTarget;
		FreeChildren;
		break;
	}
	OperatorAssign;
}
void ASTWalker::handle_Sub(Node* n) {
	GetFirstNode()
	GetLastNode()
	EnsureOperands;
	Walk;
	Op(NumAdd);
	switch (first->varType)
	{
		Operator(Number, NumSub, Sub);
	default:
		NodeType = VariableType::Undefined;
		instruction.code = OpCodes::Sub;
		In8 = first->regTarget;
		In8_2 = last->regTarget;
		FreeChildren;
		break;
	}
	OperatorAssign;
}
void ASTWalker::handle_Div(Node* n) {
	GetFirstNode()
	GetLastNode()
	EnsureOperands;
	Walk;
	Op(NumAdd);
	switch (first->varType)
	{
		Operator(Number, NumDiv, Div);
	default:
		NodeType = VariableType::Undefined;
		instruction.code = OpCodes::Div;
		In8 = first->regTarget;
		In8_2 = last->regTarget;
		FreeChildren;
		break;
	}
	OperatorAssign;

}
void ASTWalker::handle_Mult(Node* n) {
	GetFirstNode()
	GetLastNode()
	EnsureOperands;
	Walk;
	Op(NumAdd);
	switch (first->varType)
	{
		Operator(Number, NumMul, Mul);
	default:
		NodeType = VariableType::Undefined;
		instruction.code = OpCodes::Mul;
		In8 = first->regTarget;
		In8_2 = last->regTarget;
		FreeChildren;
		break;
	}
	OperatorAssign;
}

void ASTWalker::handle_AssignAdd(Node* n) {
	GetLastNode()
	n->type = Token::Add;
	WalkOne(n);
	last = n;
	n->type = Token::Assign;
	helper_Assign(n);
}
void ASTWalker::handle_AssignSub(Node* n) {
	GetLastNode()
	n->type = Token::Sub;
	WalkOne(n);
	last = n;
	n->type = Token::Assign;
	helper_Assign(n);
}
void ASTWalker::handle_AssignDiv(Node* n) {
	GetLastNode()
		n->type = Token::Div;
	WalkOne(n);
	last = n;
	n->type = Token::Assign;
	helper_Assign(n);
}
void ASTWalker::handle_AssignMult(Node* n) {
	GetLastNode()
		n->type = Token::Mult;
	WalkOne(n);
	last = n;
	n->type = Token::Assign;
	helper_Assign(n);
}

void ASTWalker::handle_Id(Node* n) {
	Walk;
	CompileSymbol* symbol = nullptr;
	PathType data(std::get<std::string>(n->data).c_str());
	if (CurrentScope && n->children.empty()) {
		symbol = CurrentScope->FindSymbol(data);
	}
	if (!symbol) {
		if (n->children.size() != 1) {
			auto [fullname, globalSymbol] = FindSymbol(data);

			if (fullname) {
				data = fullname;
				symbol = FindOrCreateLocalSymbol(data);
				symbol->Sym = globalSymbol;
				symbol->Global = true;
				n->varType = globalSymbol->VarType;
				if (globalSymbol->Type == SymbolType::Variable
					|| globalSymbol->Type == SymbolType::Static
					|| globalSymbol->Type == SymbolType::Function)
					symbol->NeedsLoading = true;
			}
		}
		else {
			GetFirstNode()
			if (first->sym) {
				if (first->sym->Sym->Type == SymbolType::Namespace) {
					PathType full = data.Append(first->sym->Sym->Space->Name);
					auto [fullName, globalSymbol] = FindSymbol(full);
					if (fullName) {
						data = fullName;
						symbol = FindOrCreateLocalSymbol(full);
						n->varType = globalSymbol->VarType;
						symbol->Sym = globalSymbol;
						symbol->Global = true;
						symbol->EndLife = InstructionList.size();
						if (globalSymbol->Type == SymbolType::Variable
							|| globalSymbol->Type == SymbolType::Static
							|| globalSymbol->Type == SymbolType::Function)
							symbol->NeedsLoading = true;
					}
					else {
						gCompileWarn() << "Symbol not found during compile: " << data << ". Check script compile order.";
					}
				}
				else if (first->sym->Sym->Type == SymbolType::Variable) {
					Op(LoadProperty);
					In8 = first->regTarget;
					FreeChildren;
					Out;
					size_t index = 0;
					auto it = std::find(CurrentFunction->PropertyTableSymbols.begin(), CurrentFunction->PropertyTableSymbols.end(), data);
					if (it != CurrentFunction->PropertyTableSymbols.end()) {
						index = it - CurrentFunction->PropertyTableSymbols.begin();
					}
					else {
						index = CurrentFunction->PropertyTableSymbols.size();
						CurrentFunction->PropertyTableSymbols.push_back(data.GetFirst());
					}

					auto& arg = InstructionList.emplace_back();
					arg.code = OpCodes::Noop;
					arg.param = static_cast<uint16_t>(index);

					size_t typeIndex = (size_t)first->varType - (size_t)VariableType::Object;
					if (typeIndex < CurrentFunction->TypeTableSymbols.size()) {
						auto& objectName = CurrentFunction->TypeTableSymbols[typeIndex];
						if (auto [localObject, objectType] = FindSymbol(objectName); objectType && objectType->Type == SymbolType::Object) {
							n->varType = objectType->UserObject->GetFieldType(data.GetFirst());
						}
					}
					return;
				}
				else {
					Error(std::string("Unknown symbol: ") + data.toString());
				}
			}
			else {
				Error(std::string("Unknown symbol: ") + data.toString());
			}
		}
	}
	if (!symbol || (symbol && symbol->NeedsLoading)) {
		//auto data = getFullId(n);
		auto it = std::find(CurrentFunction->GlobalTableSymbols.begin(), CurrentFunction->GlobalTableSymbols.end(), data);
		size_t index = 0;

		if (it != CurrentFunction->GlobalTableSymbols.end()) {
			index = it - CurrentFunction->GlobalTableSymbols.begin();
		}
		else {
			index = CurrentFunction->GlobalTableSymbols.size();
			CurrentFunction->GlobalTableSymbols.push_back(data);
			CurrentFunction->GlobalTable.push_back(nullptr);
		}

		if (symbol) {
			NodeType = symbol->Sym->VarType;
			n->sym = symbol;
		}

		Op(LoadSymbol);
		In16 = index;
		Out;
	}
	else if (symbol) {
		n->regTarget = symbol->Register;
		symbol->EndLife = InstructionList.size();
		NodeType = symbol->Sym->VarType;
		n->sym = symbol;
	}
}

void ASTWalker::handle_Property(Node* n) {
	NameType data(std::get<std::string>(n->data).c_str());
	if (n->children.size() != 1) {
		Error("Invalid property access: " + data.toString());
		return;
	}
	Walk;
	GetFirstNode()
	if (first->varType > VariableType::Object || first->varType == VariableType::Undefined) {
		Op(LoadProperty);
		In8 = first->regTarget;
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

		size_t typeIndex = (size_t)first->varType - (size_t)VariableType::Object;
		if (typeIndex < CurrentFunction->TypeTableSymbols.size()) {
			auto& objectName = CurrentFunction->TypeTableSymbols[typeIndex];
			if (auto localObject = FindSymbol(objectName); localObject.second && localObject.second->Type == SymbolType::Object) {
				n->varType = localObject.second->UserObject->GetFieldType(data);
			}
		}
		return;
	}
	else {
		Error("Unknown symbol: " + data.toString());
	}
}

void ASTWalker::handle_Negate(Node* n) {
	GetFirstNode()
	auto& var = first->varType;
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
	In8_2 = first->regTarget;
	FreeChildren;
	FreeRegister(load.target);
	Out;
	NodeType = VariableType::Number;
}

void ASTWalker::handle_Not(Node* n) {
	GetFirstNode()
	Walk;
	Op(Not);
	In8 = first->regTarget;
	FreeChildren;
	Out;
	NodeType = VariableType::Boolean;
}

void ASTWalker::handle_Less(Node* n) {
	GetFirstNode()
	GetLastNode()
	Compare(Less)
}

void ASTWalker::handle_LessEqual(Node* n) {
	GetFirstNode()
	GetLastNode()
	Compare(LessEqual)
}

void ASTWalker::handle_Larger(Node* n) {
	GetFirstNode()
	GetLastNode()
	Compare(Greater)
}

void ASTWalker::handle_LargerEqual(Node* n) {
	GetFirstNode()
	GetLastNode()
	Compare(GreaterEqual)
}

void ASTWalker::handle_Equal(Node* n) {
	GetFirstNode()
	GetLastNode()
	Compare(Equal)
}

void ASTWalker::handle_NotEqual(Node* n) {
	GetFirstNode()
	GetLastNode()
	Compare(NotEqual)
}

void ASTWalker::handle_And(Node* n) {
	GetFirstNode()
	GetLastNode()
	Walk;
	NodeType = VariableType::Boolean;
	Op(And);
	In8 = first->regTarget;
	In8_2 = last->regTarget;
	FreeChildren;
	Out;
}
void ASTWalker::handle_Or(Node* n) {
	GetFirstNode()
	GetLastNode()
	Walk;
	NodeType = VariableType::Boolean;
	Op(Or);
	In8 = first->regTarget;
	In8_2 = last->regTarget;
	FreeChildren;
	Out;
}

void ASTWalker::handle_Increment(Node* n) {
	GetFirstNode()
	WalkOne(first);
	if (first->varType != VariableType::Number) {
		Error("Cannot increment");
	}
	Op(PostMod);
	In8 = first->regTarget;
	In8_2 = n->type == Token::Increment ? 0 : 1;
	Out;
	FreeChildren;
	In8 = WalkStore(first); // @todo: this is unsafe
	FreeChildren;
	n->sym = first->sym;
}

void ASTWalker::handle_PreIncrement(Node* n) {
	GetFirstNode()
	WalkOne(first);
	if (first->varType != VariableType::Number) {
		Error("Cannot increment");
	}
	Op(PreMod);
	In8 = first->regTarget;
	In8_2 = n->type == Token::PreIncrement ? 0 : 1;
	n->regTarget = instruction.target = WalkStore(first);
	FreeChildren;
	n->sym = first->sym;
}

void ASTWalker::handle_Assign(Node* n) {
	if (n->children.size() != 2) return;
	GetLastNode()
	WalkOne(last);
	helper_Assign(n);
}

void ASTWalker::helper_Assign(Node * n) {
	GetFirstNode()
	GetLastNode()
	auto& lhs = first;
	auto& rhs = last;

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
	if (lhs->sym && !isAssignable(lhs->sym->Sym->Flags)) {
		Error("Trying to assign to unassignable type");
		HasError = true;
	}
	if (rhs->varType != VariableType::Undefined && lhs->varType != rhs->varType && lhs->sym) {
		if (isTyped(lhs->sym->Sym->Flags) || lhs->data.index() != 0) {
			Error("Trying to assign unrelated types");
			HasError = true;
		}
		else if (lhs->varType == VariableType::Undefined) {
			lhs->sym->Sym->VarType = NodeType = lhs->varType = rhs->varType;
		}
	}
}

void ASTWalker::handle_VarDeclare(Node* n) {
	PathType data(std::get<std::string>(n->data).c_str());
	if (FindSymbol(data).first || CurrentScope->FindSymbol(data)) {
		Error("Symbol already defined: " + data.toString());
		return;
	}
	auto sym = CurrentScope->addSymbol(data);
	sym->Sym = new Symbol();
	sym->StartLife = InstructionList.size();
	sym->EndLife = InstructionList.size();
	sym->Sym->setType(n->type == Token::VarDeclare ? SymbolType::Variable : SymbolType::Static);
	sym->Resolved = true;
	sym->NeedsLoading = false;
	n->sym = sym;
	if (n->children.size() > 0) {
		GetFirstNode()
		Walk;
		sym->Sym->VarType = NodeType = first->varType;
		if (sym->Sym->VarType != VariableType::Undefined) {
			sym->Sym->Flags = sym->Sym->Flags | SymbolFlags::Typed;
		}
		if (n->children.size() == 2) {
			GetLastNode()
			if (last->varType == NodeType || NodeType == VariableType::Undefined) {
				sym->Sym->VarType = NodeType = last->varType;
				if (!last->sym) {
					sym->Register = n->regTarget = last->regTarget;
				}
				else {
					Op(Copy);
					In8 = last->regTarget;
					sym->Register = Out;
				}
				return;
			}
			Error("Cannot assign unrelated types");
		}
	}
	if (sym->Sym->VarType >= VariableType::Object) {
		Op(PushObjectDefault);
		In16 = (uint16_t)sym->Sym->VarType - (uint16_t)VariableType::Object;
		sym->Register = Out;
	}
	else {
		Op(PushTypeDefault);
		In16 = (uint16_t)sym->Sym->VarType;
		sym->Register = Out;
	}
}

void ASTWalker::handle_ObjectInit(Node* n) {
	uint8_t lastReg = 255;
	GetFirstNode()
	GetLastNode()
	auto name = getFullId(first);
	for (auto& c : last->children) {
		WalkOne(c);
		if (!c->sym || c->regTarget == lastReg + 1) {
			FreeRegister(c->regTarget);
			lastReg = SetOut(c) = GetLastFree();
		}
		else {
			Op(Copy);
			instruction.target = GetLastFree();
			In8 = c->regTarget;
			lastReg = c->regTarget = instruction.target;
		}
	}

	for (auto& c : last->children) {
		FreeRegister(c->regTarget);
	}

	PathTypeQuery query = { name, SearchPaths };
	auto it = std::find(CurrentFunction->TypeTableSymbols.begin(), CurrentFunction->TypeTableSymbols.end(), query);
	size_t index = 0;

	if (it != CurrentFunction->TypeTableSymbols.end()) {
		index = it - CurrentFunction->TypeTableSymbols.begin();
	}
	else {
		index = CurrentFunction->TypeTableSymbols.size();
		CurrentFunction->TypeTableSymbols.push_back(query);
	}
	Op(InitObject);

	if (last->children.size() != 0) {
		In8 = (uint8_t)last->children.front()->regTarget;
	}
	else {
		In8 = GetLastFree();
	}
	In8_2 = last->children.size();
	Out;

	auto& arg = InstructionList.emplace_back();
	arg.code = OpCodes::Noop;
	arg.param = (uint16_t)index;

}

void ASTWalker::handle_Return(Node* n) {
	Walk;
	Op(Return);
	if (!n->children.empty()) {
		GetFirstNode()
		instruction.target = first->regTarget;
		In8 = 1;
	}
}

void ASTWalker::handle_Conditional(Node* n) {
	if (n->children.size() != 3) {
		Error("Invalid ternary operator");
		return;
	}
	WalkOne(n->children[0]);
	GetFirstNode()
	FreeConstant(first);

	auto& trueInst = InstructionList.emplace_back();
	trueInst.code = OpCodes::JumpNeg;;
	trueInst.target = first->regTarget;
	trueInst.param = 0;
	auto falseStart = InstructionList.size();

	WalkOne(n->children[1]);
	FreeConstant(n->children[1]);

	auto& falseInst = InstructionList.emplace_back();
	falseInst.code = OpCodes::JumpForward;;
	falseInst.target = first->regTarget;
	falseInst.param = 0;
	auto trueStart = InstructionList.size();


	auto falseTarget = InstructionList.size();
	WalkOne(n->children[2]);
	FreeConstant(n->children[2]);
	auto trueTarget = InstructionList.size();

	trueInst.param = falseTarget - falseStart;
	falseInst.param = trueTarget - trueStart;

	n->instruction = SetOut(n->children[2]) = SetOut(n->children[1]) = GetFirstFree();

	if (n->children[1]->varType == n->children[2]->varType) {
		NodeType = n->children[1]->varType;
	}

}

void ASTWalker::handle_If(Node* n) {
	if (n->children.size() != 3) {
		Error("Incorrect if-block");
	}

	auto& next = CurrentScope->children.emplace_back();
	next.parent = CurrentScope;
	CurrentScope = &next;
	auto regs = Registers;
	GetFirstNode()
	WalkOne(first);
	Op(JumpNeg);
	n->regTarget = instruction.target = first->regTarget;
	FreeConstant(n);

	auto oldSize = InstructionList.size();
	auto it = n->children[1];
	WalkOne(it);
	FreeConstant(it);

	size_t elseloc = InstructionList.size();
	auto& elseinst = InstructionList.emplace_back();
	elseinst.code = OpCodes::JumpForward;

	InstructionList[n->instruction].param = InstructionList.size() - oldSize;
	oldSize = InstructionList.size();

	GetLastNode()
	WalkOne(last);
	FreeConstant(last);

	InstructionList[elseloc].param = InstructionList.size() - oldSize;

	CurrentScope = CurrentScope->parent;
	Registers = regs;
}

void ASTWalker::handle_For(Node* n) {
	if (n->children.size() == 5) {
		auto& next = CurrentScope->children.emplace_back();
		next.parent = CurrentScope;
		CurrentScope = &next;
		auto regs = Registers;
		GetFirstNode()
		WalkOne(first);

		auto start = InstructionList.size();
		auto it = n->children[1];
		WalkOne(it);
		FreeConstant(it);

		Op(JumpNeg);
		n->regTarget = instruction.target = it->regTarget;
		auto jumpStart = InstructionList.size();

		auto target = n->children[3];
		WalkOne(target);
		FreeConstant(target);

		size_t loopend = InstructionList.size();
		it = n->children[2];
		WalkOne(it);
		FreeConstant(it);

		auto& elseinst = InstructionList.emplace_back();
		elseinst.code = OpCodes::JumpBackward;
		elseinst.param = InstructionList.size() - start;

		GetLastNode()
		size_t elsestart = InstructionList.size();
		WalkOne(last);
		FreeConstant(last);

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

		GetFirstNode()
		WalkOne(first);

		auto sym = CurrentScope->addSymbol("_index_");
		sym->Sym = new Symbol();
		sym->Register = GetFirstFree();
		sym->Sym->setType(SymbolType::Static);
		sym->Sym->VarType = VariableType::Number;
		{
			Op(LoadImmediate);
			In16 = (uint16_t)-1;
			n->regTarget = instruction.target = sym->Register;
		}

		PathType data(std::get<std::string>(n->data).c_str());
		bool isVar = data;

		auto start = InstructionList.size();
		uint8_t regtarget = 0;
		if (isVar) {
			auto var = CurrentScope->addSymbol(data);
			var->Sym = new Symbol();
			var->Register = GetFirstFree();
			var->Sym->setType(SymbolType::Variable);
			if (first->varType == VariableType::Number) var->Sym->VarType = VariableType::Number;
			{
				Op(RangeForVar);
				In8 = sym->Register;
				In8_2 = first->regTarget;
				Out;
				regtarget = instruction.target;
			}
			{
				Op(Noop);
				In8 = var->Register;
			}
		}
		else {
			{
				Op(RangeFor);
				In8 = sym->Register;
				In8_2 = first->regTarget;
				Out;
				regtarget = instruction.target;
			}
		}

		Op(JumpNeg);
		n->regTarget = instruction.target = regtarget;
		auto jumpStart = InstructionList.size();

		auto target = n->children[1];
		WalkOne(target);
		FreeConstant(target);

		size_t loopend = InstructionList.size();

		auto& elseinst = InstructionList.emplace_back();
		elseinst.code = OpCodes::JumpBackward;
		elseinst.param = InstructionList.size() - start;

		size_t elsestart = InstructionList.size();
		GetLastNode()
		WalkOne(last);
		FreeConstant(last);

		InstructionList[n->instruction].param = InstructionList.size() - jumpStart;

		PlaceBreaks(n, loopend, elsestart);

		CurrentScope = CurrentScope->parent;
		Registers = regs;
	}
	else {
		Error("Incorrect for-block");
	}
}

void ASTWalker::handle_While(Node* n) {
	if (n->children.size() != 2) {
		Error("Incorrect while-block");
	}

	auto& next = CurrentScope->children.emplace_back();
	next.parent = CurrentScope;
	CurrentScope = &next;
	auto regs = Registers;

	auto start = InstructionList.size();
	GetFirstNode()
	WalkOne(first);
	Op(JumpNeg);
	auto jumpStart = InstructionList.size();
	n->regTarget = instruction.target = first->regTarget;

	auto it = n->children[1];
	WalkOne(it);
	FreeConstant(it);

	auto& elseinst = InstructionList.emplace_back();
	elseinst.code = OpCodes::JumpBackward;
	elseinst.param = InstructionList.size() - start;

	InstructionList[n->instruction].param = InstructionList.size() - jumpStart;

	PlaceBreaks(n, start, InstructionList.size());

	CurrentScope = CurrentScope->parent;
	Registers = regs;
}

void ASTWalker::handle_Break(Node* n) {
	Op(Jump);
}

void ASTWalker::handle_Else(Node* n) {
	Walk;
}

void ASTWalker::handle_FunctionCall(Node* n) {
	int type = 0;
	GetFirstNode()
	WalkOne(first);
	auto name = getFullId(first);
	if (!first->sym) {
		if (first->varType == VariableType::Function || first->varType == VariableType::Undefined) {
			type = 3;
			goto function;
		}
		goto function;
	}
	else if (first->sym->Sym->Type != SymbolType::Function && first->sym->Sym->Type == SymbolType::Variable) {
		type = 3;
		goto function;
	}
	else if (first->sym->Sym->Type == SymbolType::Function) {
		auto fn = first->sym->Sym->Function;

		if (first->instruction != 0 && InstructionList[first->instruction].code == OpCodes::LoadSymbol) {
			InstructionList[first->instruction].data = 0;
		}

		GetLastNode()
		auto fnsym = fn->GetFirstFitting((int)last->children.size());

		if (fnsym)
			NodeType = fnsym->Signature.Return;
	}
	else {
		Error("Cannot call unknown symbol, something went wrong");
		return;
	}
function:

	uint8_t lastReg = 255;
	GetLastNode()
	auto& params = last;
	for (auto& c : params->children) {
		WalkOne(c);
		if (!c->sym || c->regTarget == lastReg + 1) {
			FreeRegister(c->regTarget);
			lastReg = SetOut(c) = GetLastFree();
		}
		else {
			Op(Copy);
			instruction.target = GetLastFree();
			In8 = c->regTarget;
			lastReg = c->regTarget = instruction.target;
		}
	}

	for (auto& c : params->children) {
		FreeRegister(c->regTarget);
	}
	FreeConstant(first);

	auto it = std::find(CurrentFunction->FunctionTableSymbols.begin(), CurrentFunction->FunctionTableSymbols.end(), name);
	size_t index = 0;

	if (it != CurrentFunction->FunctionTableSymbols.end()) {
		index = it - CurrentFunction->FunctionTableSymbols.begin();
	}
	else {
		index = CurrentFunction->FunctionTableSymbols.size();
		PathTypeQuery query(name, SearchPaths);
		CurrentFunction->FunctionTableSymbols.push_back(query);
	}

	Op(CallFunction);

	switch (type)
	{
	case 0: instruction.code = OpCodes::CallFunction; break;
	case 3: instruction.code = OpCodes::CallSymbol; break;
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
	case 3: arg.target = first->regTarget; break;
	case 4: arg.data = (uint32_t)index; break;
	}
}

uint8_t ASTWalker::WalkStore(Node* n) {
	auto first = n->children.size() ? n->children.front() : nullptr;
	auto last = n->children.size() ? n->children.back() : nullptr;
	switch (n->type) {
	case Token::Id: {
			Walk;
			CompileSymbol* symbol = nullptr;
			PathType data(std::get<std::string>(n->data).c_str());
			if (CurrentScope && n->children.empty()) {
				symbol = CurrentScope->FindSymbol(data);
			}
			if (!symbol) {
				if (n->children.size() != 1) {
					auto [fullname, globalSymbol] = FindSymbol(data);

					if (fullname) {
						data = fullname;
						symbol = FindOrCreateLocalSymbol(data);
						symbol->Sym = globalSymbol;
						symbol->Global = true;
						n->varType = globalSymbol->VarType;
						if (globalSymbol->Type == SymbolType::Variable
							|| globalSymbol->Type == SymbolType::Static)
							symbol->NeedsLoading = true;
					}
				}
				else {
					if (first->sym) {
						if (first->sym->Sym->Type == SymbolType::Namespace) {
							PathType full = data.Append(first->sym->Sym->Space->Name);
							auto [fullName, globalSymbol] = FindSymbol(full);
							if (fullName) {
								symbol = FindOrCreateLocalSymbol(full);
								symbol->Sym = globalSymbol;
								symbol->Global = true;
								symbol->EndLife = InstructionList.size();
								n->varType = globalSymbol->VarType;
								if (globalSymbol->Type == SymbolType::Variable
									|| globalSymbol->Type == SymbolType::Static
									|| globalSymbol->Type == SymbolType::Function)
									symbol->NeedsLoading = true;
							}
						}
						else if (first->sym->Sym->Type == SymbolType::Variable) {

							size_t typeIndex = (size_t)first->varType - (size_t)VariableType::Object;
							if (typeIndex < CurrentFunction->TypeTableSymbols.size()) {
								auto& objectName = CurrentFunction->TypeTableSymbols[typeIndex];
								if (auto it = FindSymbol(objectName); it.second && it.second->Type == SymbolType::Object) {
									n->varType = it.second->UserObject->GetFieldType(data.GetFirst());
								}
							}

							// @todo: Maybe type checking here?

							Op(StoreProperty);
							In8 = first->regTarget;
							Out;
							size_t index = 0;
							auto it = std::find(CurrentFunction->PropertyTableSymbols.begin(), CurrentFunction->PropertyTableSymbols.end(), data);
							if (it != CurrentFunction->PropertyTableSymbols.end()) {
								index = it - CurrentFunction->PropertyTableSymbols.begin();
							}
							else {
								index = CurrentFunction->PropertyTableSymbols.size();
								CurrentFunction->PropertyTableSymbols.push_back(data.GetFirst());
							}

							auto& arg = InstructionList.emplace_back();
							arg.code = OpCodes::Noop;
							arg.param = static_cast<uint16_t>(index);

							return n->regTarget;
						}
						else {
							Error("Unknown symbol: " + data.toString());
						}
					}
					else {
						Error("Unknown symbol: " + data.toString());
					}
				}
			}
			if (!symbol || (symbol && symbol->NeedsLoading)) {
				auto it = std::find(CurrentFunction->GlobalTableSymbols.begin(), CurrentFunction->GlobalTableSymbols.end(), data);
				size_t index = 0;

				if (it != CurrentFunction->GlobalTableSymbols.end()) {
					index = it - CurrentFunction->GlobalTableSymbols.begin();
				}
				else {
					index = CurrentFunction->GlobalTableSymbols.size();
					CurrentFunction->GlobalTableSymbols.push_back(data);
				}

				if (symbol) {
					NodeType = symbol->Sym->VarType;
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
				NodeType = symbol->Sym->VarType;
				n->sym = symbol;
				return n->regTarget;
			}
		}

		case Token::Indexer: {
			Walk;
			Op(StoreIndex);
			//n->sym = first->sym;
			In8 = first->regTarget;
			In8_2 = last->regTarget;
			FreeConstant(last);
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
		WalkOne(n);
		return n->regTarget;
	}
	}
	return n->regTarget;
}

CompileSymbol* ASTWalker::FindLocalSymbol(const PathType& name)
{
	CompileSymbol* symbol = nullptr;
	if (CurrentScope) {
		symbol = CurrentScope->FindSymbol(name);
	}
	return symbol;
}

CompileSymbol* ASTWalker::FindOrCreateLocalSymbol(const PathType& name)
{
	auto symbol = FindLocalSymbol(name);

	if (!symbol && CurrentScope) {
		symbol = CurrentScope->addSymbol(name);
		symbol->EndLife = symbol->StartLife = InstructionList.size();
	}

	return symbol;
}

std::pair<PathType, Symbol*> ASTWalker::FindSymbol(const PathTypeQuery& name)
{
	if (!name) return { {}, nullptr };
	PathTypeQuery query = name;
	query.Paths().insert(query.Paths().end(), SearchPaths.begin(), SearchPaths.end());

	auto res = Global.FindName(query);

	if (!res.first) {
		res = Vm->FindSymbol(query);
	}
	return res;
}

std::pair<PathType, Symbol*> ASTWalker::FindSymbol(const PathType& name)
{
	if (!name) return { name, nullptr };

	PathTypeQuery query(name, SearchPaths);
	auto res = Global.FindName(query);

	if (!res.first) {
		res = Vm->FindSymbol(query);
	}
	return res;
}

std::pair<PathType, Symbol*> ASTWalker::FindOrCreateSymbol(const PathType& name, SymbolType type)
{
	auto res = FindSymbol(name);
	if (res.first) return res;

	auto symbol = new Symbol();
	symbol->setType(type);

	PathType newName(name.Append(SearchPaths[0]));
	if (Global.AddName(newName, symbol)) {
		return { newName, symbol };
	}
	delete symbol;
	return { {}, nullptr };
}

void ASTWalker::HandleFunction(Node* n, ScriptFunction* f, CompileSymbol* s)
{
	InitRegisters();
	CurrentFunction = f;
	SearchPaths.resize(1);
	SearchPaths[0] = f->Name.Get(1);
	for (auto& [line, name] : AllSearchPaths) {
		if (line < n->line) {
			SearchPaths.push_back(name);
		}
	}

	for (auto& c : n->children) {
		switch (c->type)
		{
		case Token::CallParams: {
			for (auto& v : c->children) {
				auto symParam = f->FunctionScope->FindSymbol(std::get<0>(v->data).c_str());
				if (symParam) symParam->Register = GetFirstFree();
			}
		} break;

		case Token::Scope: {

			InstructionList.reserve(c->children.size() * n->depth);
			CurrentScope = f->FunctionScope;
			for (auto& stmt : c->children) {
				try {
					WalkOne(stmt);
					FreeConstant(stmt);
				}
				catch (...) {
					gCompileError() << "Internal error occured!";
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
	gCompileDebug() << "Function " << f->Name.toString();
	gCompileLogger() << "\n----------------------------------\n";
	for (auto& in : InstructionList) {
		printInstruction(in);
	}
#endif // DEBUG

	f->FunctionTable.resize(f->FunctionTableSymbols.size(), nullptr);
	f->PropertyTable.resize(f->PropertyTableSymbols.size(), -1);
	f->TypeTable.resize(f->TypeTableSymbols.size(), VariableType::Undefined);
	f->GlobalTable.resize(f->GlobalTableSymbols.size(), nullptr);

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

void ASTWalker::HandleInit()
{
	InitRegisters();
	CurrentFunction = InitFunction;
	CurrentScope = CurrentFunction->FunctionScope;
	SearchPaths.resize(1);
	SearchPaths[0] = PathType();

	for (auto& node : Root->children) {
		switch (node->type)
		{
		case Token::NamespaceDef: {
			SearchPaths[0] = getFullId(node);
		} break;
		case Token::UsingDef: {
			SearchPaths.push_back(getFullId(node));
		} break;

		case Token::PublicFunctionDef:
		case Token::FunctionDef:
		case Token::ObjectDef:
			break;

		case Token::Const:
		case Token::VarDeclare:
		case Token::Static: {
			auto old = node->children[0];
			Token type = old->type;
			auto data = old->data;
			old->type = Token::Id;
			old->data = node->data;
			node->type = Token::Assign;
			auto [path, symbol] = FindSymbol(NameType(std::get<std::string>(old->data).c_str()));
			bool assignable = true;
			if (symbol) {
				assignable = isAssignable(symbol->Flags);
				symbol->Flags = symbol->Flags | SymbolFlags::Assignable;
			}
			WalkOne(node);
			FreeConstant(node);
			if (symbol && !assignable) symbol->Flags = symbol->Flags ^ SymbolFlags::Assignable;
			old->type = type;
			old->data = data;
			node->sym = nullptr;
		} break;

		default:
			WalkOne(node);
			FreeConstant(node);
			break;
		}
	}
	Instruction op;
	op.code = OpCodes::Return;
	op.in1 = 0;
	InstructionList.emplace_back(op);

	InitFunction->Bytecode.resize(InstructionList.size());
	for (size_t i = 0; i < InstructionList.size(); i++) {
		InitFunction->Bytecode[i] = InstructionList[i].data;
	}

#ifdef DEBUG
	gCompileDebug() << "Init Function";
	gCompileLogger() << "\n----------------------------------\n";
	for (auto& in : InstructionList) {
		printInstruction(in);
	}
#endif // DEBUG

	InitFunction->FunctionTable.resize(InitFunction->FunctionTableSymbols.size(), nullptr);
	InitFunction->PropertyTable.resize(InitFunction->PropertyTableSymbols.size(), -1);
	InitFunction->TypeTable.resize(InitFunction->TypeTableSymbols.size(), VariableType::Undefined);
	InitFunction->GlobalTable.resize(InitFunction->GlobalTableSymbols.size(), nullptr);

	InitFunction->StringTable.reserve(StringList.size());
	for (auto& str : StringList) {
		InitFunction->StringTable.emplace_back(String::GetAllocator()->Make(str.c_str()));
	}
	StringList.clear();

	InitFunction->RegisterCount = MaxRegister + 1;
	InstructionList.clear();

	CurrentFunction = nullptr;
	CurrentScope = nullptr;

	// Cleanup
	delete InitFunction->FunctionScope;
	InitFunction->FunctionScope = nullptr;
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
