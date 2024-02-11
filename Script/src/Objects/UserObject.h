#pragma once
#include "Defines.h"
#include "BaseObject.h"
#include "Symbol.h"
#include <vector>
#include "ankerl/unordered_dense.h"

class ObjectManager;




class UserDefined : public Object
{
public:
	UserDefined() {
		Type = VariableType::Object;
	}
	void AddField(const std::string& name, Variable var, const Symbol& flags);
private:
	friend class ObjectManager;

	ankerl::unordered_dense::map<std::string, Symbol> fieldNames;
	std::vector<Variable> fields;
};

class ObjectManager
{

public:
	ObjectManager() {}
	VariableType AddType(const std::string& name, const UserDefined& obj);
	void RemoveType(const std::string& name);

private:
	ankerl::unordered_dense::map<std::string, UserDefined> baseTypes;
	uint32 typeCounter = (uint32)VariableType::Object;
};