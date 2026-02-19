#pragma once
#include "Parser/ParseTable.h"

constexpr size_t CreateTime = 134006849785933059u; 


int GetIndex(int state, int token);
extern ActionNode* ParseTable;
extern RuleTable_t RuleTable;
extern std::vector<RuleData> Data;
