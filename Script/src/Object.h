#pragma once
#include "Defines.h"
#include "Variable.h"
#include <vector>
#include "ankerl/unordered_dense.h"
#include "Symbol.h"

enum class ObjectType : uint32
{
	None,
	String,
	Array,

	UserDefined
};

class ObjectManager;

class Object
{
public:
	ObjectType getType() const { return Type; }
	Object() : Type(ObjectType::None), RefCount(0) {};
	virtual ~Object() {

	}

public:
	ObjectType Type;
	int RefCount;
};

class UserDefined : public Object
{
public:
	UserDefined() {
		Type = ObjectType::UserDefined;
	}
	void AddField(const std::string& name, Variable var, Symbol flags) {
		fields.push_back(var);
		fieldNames.emplace(name, flags);
	}
private:
	friend class ObjectManager;

	ankerl::unordered_dense::map<std::string, Symbol> fieldNames;
	std::vector<Variable> fields;
};

class ObjectManager
{

public:
	ObjectManager() {}

	ObjectType AddType(const std::string& name, const UserDefined& obj) {
		if (auto it = baseTypes.find(name); it == baseTypes.end()) {
			auto res = baseTypes.emplace(name, obj);
			return res.first->second.Type = (ObjectType)++typeCounter;
		}
		return ObjectType::UserDefined;
	}

	void RemoveType(const std::string& name) {
		baseTypes.erase(name);
	}

	ankerl::unordered_dense::map<std::string, UserDefined> baseTypes;

private:
	uint32 typeCounter = (uint32)ObjectType::UserDefined;
};