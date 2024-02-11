#include "UserObject.h"

VariableType ObjectManager::AddType(const std::string& name, const UserDefined& obj)
{
	if (auto it = baseTypes.find(name); it == baseTypes.end()) {
		auto res = baseTypes.emplace(name, obj);
		return res.first->second.Type = (VariableType)++typeCounter;
	}
	return VariableType::Object;
}

void ObjectManager::RemoveType(const std::string& name)
{
	baseTypes.erase(name);
}

void UserDefined::AddField(const std::string& name, Variable var, const Symbol& flags)
{
	fields.push_back(var);
	fieldNames.emplace(name, flags);
}
