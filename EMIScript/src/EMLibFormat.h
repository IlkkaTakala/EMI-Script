#pragma once
#include "Namespace.h"

namespace Library 
{

#ifdef _MSC_VER
#pragma pack(push, 1)
#define PACKED
#else
#define PACKED __attribute__((packed))
#endif
class EMLibFormat
{
	short Version; //00 00 00

	// Symbols
		// Name
		// Type
		// 

} PACKED;

bool Decode(std::istream& instream, SymbolTable& table);

bool Encode(const SymbolTable& table, std::ostream& outstream);

#ifdef _MSC_VER
#pragma pack(pop)
#endif
#undef PACKED

}