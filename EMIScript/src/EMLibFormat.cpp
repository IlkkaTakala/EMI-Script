#include "EMLibFormat.h"
#include "Function.h"
#include "Objects/UserObject.h"
#include "Objects/StringObject.h"

void WriteString(std::ostream& out, const std::string& str) {
	out << (uint16_t)str.size();
	out.write(str.c_str(), str.size());
}
std::string ReadString(std::istream& in) {
	uint16_t size = 0;
	in >> size;
	std::string str;
	str.resize(size + 1);
	in.read(str.data(), size);
	return str;
}

template <class A>
void WriteArray(std::ostream& out, const A& arr, std::function<void(std::ostream&, const typename A::value_type&)> pred) {
	out << (uint16_t)arr.size();
	for (const auto& item : arr) {
		pred(out, item);
	}
}

template <class A>
void WriteArray(std::ostream& out, const A& arr) {
	auto datasize = arr.size() * sizeof(typename A::value_type);
	out << (uint16_t)datasize;
	out.write(reinterpret_cast<const char*>(arr.data()), datasize);
}

bool Library::Decode(std::istream& instream, SymbolTable&)
{
	uint8_t format;
	char identifier[3];
	uint16_t version = 0;
	instream >> identifier;
	instream >> format;
	instream >> version;

	if (strncmp(identifier, "EMI", 3) == 0 && version > EMI_VERSION || format > FORMAT_VERSION) {
		gError() << "Not EMI library file or version is too new";
		return false;
	}

	switch (format)
	{
	case 1: {

	} break;
	default:
		gError() << "Invalid file format";
		return false;
	}

	return false;
}

bool Library::Encode(const SymbolTable& table, std::ostream& outstream)
{
	outstream << "EMI";
	outstream << FORMAT_VERSION;
	outstream << EMI_VERSION;

	outstream << (uint16_t)table.Table.size();
	WriteArray(outstream, table.Table, [](std::ostream& out, const std::pair<TName, Symbol*>& pair) {
		// Writing symbol
		auto symbol = pair.second;

		if (symbol->Builtin) return;

		WriteString(out, pair.first.toString());

		out << (uint8_t)symbol->Type;
		out << (uint16_t)symbol->Flags;
		out << (uint16_t)symbol->VarType;

		if (symbol->Data) {
			switch (symbol->Type)
			{
			case SymbolType::Namespace: {
				// Might need namespace
			} break;
			case SymbolType::Object: {

			} break;
			case SymbolType::Function: {
				auto fn = static_cast<FunctionSymbol*>(symbol->Data);
				out << (uint8_t)fn->Type;
				out << (uint16_t)fn->Return;
				WriteArray(out, fn->Arguments, [](std::ostream& out, VariableType type) {
					out << (uint16_t)type;
				});

				switch (fn->Type)
				{
				case FunctionType::User: {
					auto fnd = reinterpret_cast<Function*>(fn->DirectPtr);

					out << (uint8_t)fnd->ArgCount;
					out << (uint8_t)fnd->RegisterCount;
					out << (bool)fnd->IsPublic;

					WriteArray(out, fnd->StringTable, [](std::ostream& out, const Variable& var) {
						WriteString(out, var.as<String>()->data());
					});

					WriteArray(out, fnd->NumberTable.values());
					WriteArray(out, fnd->JumpTable.values());
					WriteArray(out, fnd->DebugLines.values());
					WriteArray(out, fnd->Types);

					WriteArray(out, fnd->FunctionTableSymbols, [](std::ostream& out, const TNameQuery& name){
						WriteString(out, name.GetTarget().toString());
						WriteArray(out, name.GetPaths(), [](std::ostream& out, const TName& path){ WriteString(out, path.toString()); });
					});
					WriteArray(out, fnd->PropertyTableSymbols, [](std::ostream& out, const TName& name){
						WriteString(out, name.toString());
					});
					WriteArray(out, fnd->TypeTableSymbols, [](std::ostream& out, const TNameQuery& name){
						WriteString(out, name.GetTarget().toString());
						WriteArray(out, name.GetPaths(), [](std::ostream& out, const TName& path){ WriteString(out, path.toString()); });
					});
					WriteArray(out, fnd->GlobalTableSymbols, [](std::ostream& out, const TNameQuery& name){
						WriteString(out, name.GetTarget().toString());
						WriteArray(out, name.GetPaths(), [](std::ostream& out, const TName& path){ WriteString(out, path.toString()); });
					});
					
					WriteArray(out, fnd->Bytecode);
				
				} break;
				default:
					break;
				}

			} break;
			default:
				break;
			}
		}
	});

	return true;
}
