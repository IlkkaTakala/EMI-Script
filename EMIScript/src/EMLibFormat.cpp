#include "EMLibFormat.h"
#include "Function.h"
#include "Objects/UserObject.h"
#include "Objects/StringObject.h"

template<typename T>
void WriteValue(std::ostream& out, T t) {
	out.write((char*)&t, sizeof(T));
}
template<typename T>
void ReadValue(std::istream& out, T& t) {
	out.read((char*)&t, sizeof(T));
}
template<typename T>
void ReadValue(std::istream& out, T& t, size_t count) {
	out.read((char*)&t, count);
}

void WriteString(std::ostream& out, const std::string& str) {
	uint16_t data = (uint16_t)str.size();
	WriteValue(out, data);
	out.write(str.c_str(), str.size());
}
std::string ReadString(std::istream& in) {
	uint16_t size = 0;
	ReadValue(in, size);
	std::string str;
	str.resize(size + 1);
	in.read(str.data(), size);
	return str;
}

template <class A>
void WriteArray(std::ostream& out, const A& arr, std::function<void(std::ostream&, const typename A::value_type&)> pred) {
	uint16_t size = (uint16_t)arr.size();
	WriteValue(out, size);
	for (const auto& item : arr) {
		pred(out, item);
	}
}

template <class A>
void WriteArray(std::ostream& out, const A& arr) {
	uint16_t datasize = uint16_t(arr.size() * sizeof(typename A::value_type));
	WriteValue(out, datasize);
	out.write(reinterpret_cast<const char*>(arr.data()), datasize);
}

template <class A>
void ReadArray(std::istream& in, A& arr, std::function<void(std::istream&, typename A::value_type&)> pred) {
	uint16_t datasize;
	ReadValue(in, datasize);
	arr.resize(datasize);
	for (auto& item : arr) {
		pred(in, item);
	}
}

template <class A>
void ReadArray(std::istream& in, A& arr) {
	uint16_t datasize;
	ReadValue(in, datasize);
	arr.resize(datasize / sizeof(typename A::value_type));
	in.read(reinterpret_cast<char*>(arr.data()), datasize);
}

void WriteFunction(std::ostream& out, const Function* fnd) {
	
	WriteValue(out, (uint8_t)fnd->ArgCount);
	WriteValue(out, (uint8_t)fnd->RegisterCount);
	WriteValue(out, (bool)fnd->IsPublic);

	WriteArray(out, fnd->StringTable, [](std::ostream& out, const Variable& var) {
		WriteString(out, var.as<String>()->data());
		});

	WriteArray(out, fnd->NumberTable.values());
	WriteArray(out, fnd->DebugLines.values());
	WriteArray(out, fnd->Types);

	WriteArray(out, fnd->FunctionTableSymbols, [](std::ostream& out, const TNameQuery& name) {
		WriteString(out, name.GetTarget().toString());
		WriteArray(out, name.GetPaths(), [](std::ostream& out, const TName& path) { WriteString(out, path.toString()); });
		});
	WriteArray(out, fnd->PropertyTableSymbols, [](std::ostream& out, const TName& name) {
		WriteString(out, name.toString());
		});
	WriteArray(out, fnd->TypeTableSymbols, [](std::ostream& out, const TNameQuery& name) {
		WriteString(out, name.GetTarget().toString());
		WriteArray(out, name.GetPaths(), [](std::ostream& out, const TName& path) { WriteString(out, path.toString()); });
		});
	WriteArray(out, fnd->GlobalTableSymbols, [](std::ostream& out, const TNameQuery& name) {
		WriteString(out, name.GetTarget().toString());
		WriteArray(out, name.GetPaths(), [](std::ostream& out, const TName& path) { WriteString(out, path.toString()); });
		});

	WriteArray(out, fnd->Bytecode);
}

void ReadFunction(std::istream& instream, Function* fnd) {
	ReadValue(instream, fnd->ArgCount);
	ReadValue(instream, fnd->RegisterCount);
	ReadValue(instream, fnd->IsPublic);

	ReadArray(instream, fnd->StringTable, [](std::istream& in, Variable& var) {
		std::string str = ReadString(in);
		var = String::GetAllocator()->Make(str.c_str());
		});

	std::vector<double> nums;
	ReadArray(instream, nums);
	fnd->NumberTable.insert(nums.begin(), nums.end());

	std::vector<std::pair<int, int>> debug;
	ReadArray(instream, debug);
	fnd->DebugLines.insert(debug.begin(), debug.end());

	ReadArray(instream, fnd->Types);

	ReadArray(instream, fnd->FunctionTableSymbols, [](std::istream& in, TNameQuery& name) {
		TNameQuery query(toName(ReadString(in).c_str()));
		ReadArray(in, query.Paths(), [](std::istream& in, TName& path) { path = toName(ReadString(in).c_str()); });
		name = query;
		});
	ReadArray(instream, fnd->PropertyTableSymbols, [](std::istream& in, TName& name) {
		name = toName(ReadString(in).c_str());
		});
	ReadArray(instream, fnd->TypeTableSymbols, [](std::istream& in, TNameQuery& name) {
		TNameQuery query(toName(ReadString(in).c_str()));
		ReadArray(in, query.Paths(), [](std::istream& in, TName& path) { path = toName(ReadString(in).c_str()); });
		name = query;
		});
	ReadArray(instream, fnd->GlobalTableSymbols, [](std::istream& in, TNameQuery& name) {
		TNameQuery query(toName(ReadString(in).c_str()));
		ReadArray(in, query.Paths(), [](std::istream& in, TName& path) { path = toName(ReadString(in).c_str()); });
		name = query;
		});

	fnd->FunctionTable.resize(fnd->FunctionTableSymbols.size(), nullptr);
	fnd->IntrinsicTable.resize(fnd->FunctionTableSymbols.size(), nullptr);
	fnd->ExternalTable.resize(fnd->FunctionTableSymbols.size(), nullptr);
	fnd->PropertyTable.resize(fnd->PropertyTableSymbols.size(), -1);
	fnd->TypeTable.resize(fnd->TypeTableSymbols.size(), VariableType::Undefined);
	fnd->GlobalTable.resize(fnd->GlobalTableSymbols.size(), nullptr);

	ReadArray(instream, fnd->Bytecode);
}

bool Library::Decode(std::istream& instream, SymbolTable& table, Function*& init)
{
	char identifier[4] = { 0 };
	uint8_t format;
	uint16_t version = 0;
	ReadValue(instream, identifier, 3);
	ReadValue(instream, format);
	ReadValue(instream, version);

	if (strncmp(identifier, "EMI", 3) == 0 && version > EMI_VERSION || format > FORMAT_VERSION) {
		gCompileError() << "Not EMI library file or version is too new";
		return false;
	}

	switch (format)
	{
	case 1: {
		uint16_t datasize;
		ReadValue(instream, datasize);

		for (size_t i = 0; i < datasize; i++) {
			// Writing symbol
			auto symbol = new Symbol();

			TName name = toName(ReadString(instream).c_str());
			uint8_t data_u8;
			uint16_t data_u16;

			ReadValue(instream, data_u8);
			symbol->Type = (SymbolType)data_u8;
			ReadValue(instream, data_u16);
			symbol->Flags = (SymbolFlags)data_u16;
			ReadValue(instream, data_u16);
			symbol->VarType = (VariableType)data_u16;

			switch (symbol->Type)
			{
			case SymbolType::Namespace: {
				symbol->Data = new Namespace{ name };
			} break;
			case SymbolType::Object: {
				auto ob = new UserDefinedType();
				ReadValue(instream, ob->Type);
				std::vector<int> data;
				ReadArray(instream, data, [ob](std::istream& in, int&) {
					auto name = toName(ReadString(in).c_str());
					Symbol flags;
					ReadValue(in, flags.Flags);
					ReadValue(in, flags.VarType);
					ob->AddField(name, {}, flags);
				});
				symbol->Data = ob;
			} break;
			case SymbolType::Variable: {
				symbol->Data = new Variable();
			} break;
			case SymbolType::Function: {
				auto fn = new FunctionSymbol();
				symbol->Data = fn;

				ReadValue(instream, data_u8);
				fn->Type = (FunctionType)data_u8;
				ReadValue(instream, data_u16);
				fn->Return = (VariableType)data_u16;
				ReadArray(instream, fn->Arguments, [](std::istream& in, VariableType type) {
					uint16_t data_u16;
					ReadValue(in, data_u16);
					type = (VariableType)data_u16;
				});

				switch (fn->Type)
				{
				case FunctionType::User: {
					auto fnd = new Function();
					fn->DirectPtr = fnd;

					ReadFunction(instream, fnd);

				} break;
				default:
					break;
				}
			} break;
			default:
				break;
			}

			table.AddName(name, symbol);
		}

		init = new Function();

		ReadFunction(instream, init);

	} break;
	default:
		gCompileError() << "Invalid file format";
		return false;
	}

	return true;
}

bool Library::Encode(const SymbolTable& table, std::ostream& outstream, Function* init)
{
	outstream << "EMI";
	WriteValue(outstream, FORMAT_VERSION);
	WriteValue(outstream, EMI_VERSION);

	SymbolTable outtable;
	for (auto& [name, symbol] : table.Table) {
		if (!symbol) continue;
		if (symbol->Builtin) continue;

		outtable.AddName(name, symbol);
	}

	WriteArray(outstream, outtable.Table, [](std::ostream& out, const std::pair<TName, Symbol*>& pair) {
		// Writing symbol
		auto symbol = pair.second;


		WriteString(out, pair.first.toString());

		WriteValue(out, (uint8_t)symbol->Type);
		WriteValue(out, (uint16_t)symbol->Flags);
		WriteValue(out, (uint16_t)symbol->VarType);

		if (symbol->Data) {
			switch (symbol->Type)
			{
			case SymbolType::Namespace: {
				// Might need namespace
			} break;
			case SymbolType::Object: {
				auto ob = static_cast<UserDefinedType*>(symbol->Data);
				WriteValue(out, ob->Type);
				WriteArray(out, ob->GetFields().values(), [](std::ostream& out, const auto& data) {
					WriteString(out, data.first.toString());
					WriteValue(out, data.second.Flags);
					WriteValue(out, data.second.VarType);
				});
			} break;
			case SymbolType::Function: {
				auto fn = static_cast<FunctionSymbol*>(symbol->Data);
				WriteValue(out, (uint8_t)fn->Type);
				WriteValue(out, (uint16_t)fn->Return);
				WriteArray(out, fn->Arguments, [](std::ostream& out, VariableType type) {
					WriteValue(out, (uint16_t)type);
					});

				switch (fn->Type)
				{
				case FunctionType::User: {
					auto fnd = reinterpret_cast<Function*>(fn->DirectPtr);
					WriteFunction(out, fnd);
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

	WriteFunction(outstream, init);

	return true;
}
