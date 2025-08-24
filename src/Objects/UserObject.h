#pragma once
#include "ankerl/unordered_dense.h"
#include "BaseObject.h"
#include "Defines.h"
#include "Symbol.h"
#include <vector>

class ObjectManager;

class UserObject : public Object
{
public:
	UserObject() {
		Data = nullptr;
		DataCount = 0;
		Type = VariableType::Object;
	}

	UserObject(VariableType type, uint16_t count);
	UserObject(const UserObject& object);
	~UserObject();

	void Clear();

	static Allocator<UserObject>* GetAllocator();

	uint16_t size() const { return DataCount; }

	Variable& operator[](uint16_t index) {
		if (index < DataCount)
			return Data[index];
		return Data[0];
	}

private:

	Variable* Data;
	uint16_t DataCount;
};


class UserDefinedType
{
public:
	UserDefinedType() {
		Type = VariableType::Object;
	}
	void AddField(const NameType& name, Variable var, const Symbol& flags);
	VariableType GetFieldType(const NameType& name) const;
	auto& GetFields() { return FieldNames; }
	auto& GetDefaults() { return DefaultFields; }

	VariableType Type;
private:
	friend class ObjectManager;

	ankerl::unordered_dense::map<NameType, Symbol> FieldNames;
	std::vector<Variable> DefaultFields;
	std::vector<VariableType> DefaultTypes;
};

class ObjectManager
{

public:
	ObjectManager() {}
	VariableType AddType(const PathType& name, const UserDefinedType& obj);
	void RemoveType(const PathType& name);

	Variable Make(VariableType type) const ;

	bool GetType(UserDefinedType*& type, const PathType& name);

	bool GetPropertyIndex(uint16_t& out, const NameType& name, VariableType type);
	bool GetPropertySymbol(Symbol*& symbol, const NameType& name, VariableType type);

private:
	ankerl::unordered_dense::map<VariableType, UserDefinedType> BaseTypes;
	ankerl::unordered_dense::map<PathType, VariableType> NameToType;
	uint32_t TypeCounter = (uint32_t)VariableType::Object;
};

ObjectManager& GetManager();