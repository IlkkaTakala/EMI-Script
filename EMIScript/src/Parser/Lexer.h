#pragma once
#include <string>
#include <string_view>
#include <fstream>
#include <ankerl/unordered_dense.h>
#include "Defines.h"

#define X(name) name,
enum class Token : uint16
{
#include "Lexemes.h"
};
#undef X 

#ifdef DEBUG
extern ankerl::unordered_dense::map<Token, const char*> TokensToName;
#endif // DEBUG

struct Context
{
	const char* Ptr;
	const char* Last;
	size_t Column{ 0 };
	size_t Row{ 1 };
	bool Valid;

	Token Previous;

	bool Advance() {
		switch (*Ptr) {
		case '\n':
			Row++;
			Column = 0;
			break;
		default:
			Column++;
			break;
		}
		Ptr++;

		Valid = Ptr < Last;
		return Valid;
	}

	void Back() {
		Ptr--;
		if (Column > 0) {
			Column--;
		}
		else {
			Row--;
			Column = 0;
		}
	}
};

class Lexer
{
public:
	Lexer(const char* data, size_t size);
	~Lexer();

	void Reset();
	bool IsValid() const { return Valid; }
	Token GetNext(std::string_view& Data);
	const Context& GetContext() { return Current; }

private:

	Token Analyse(std::string_view& Data);

	const char* FileData;
	size_t Size;
	Context Current;
	bool InString;
	bool InQuote;
	bool Valid;
};