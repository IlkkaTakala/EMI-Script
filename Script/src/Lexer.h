#pragma once
#include <string>
#include <string_view>
#include <fstream>
#include <ankerl/unordered_dense.h>

#define X(name) name,
enum Token
{
#include "Lexemes.h"
};
#undef X 

extern ankerl::unordered_dense::map<Token, const char*> TokensToName;

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
};

class Lexer
{
public:
	Lexer(const std::string& file);
	~Lexer();

	void Reset();
	bool IsValid() const { return Valid; }
	Token GetNext(std::string_view& Data);
	const Context& GetContext() { return Current; }

private:

	Token Analyse(std::string_view& Data);

	std::string FileData;
	Context Current;
	bool Valid;
};