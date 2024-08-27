#include "EMLibFormat.h"

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

bool Library::Decode(std::istream& instream, SymbolTable& table)
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
    for (auto& [name, symbol] : table.Table) {
        WriteArray(outstream, table.Table, [](std::ostream& out, std::pair<TName, Symbol*> pair) {
            WriteString(out, pair.first.toString());
        });
    }

    return false;
}
