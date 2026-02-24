#pragma once
#include "Parser/ParseTable.h"

constexpr size_t CreateTime = 134161757836156961u; 


int GetIndex(int state, int token);
extern ActionNode* ParseTable;
extern RuleTable_t RuleTable;
extern std::vector<RuleData> Data;
