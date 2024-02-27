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
	}

	UserObject(VariableType type, uint16 count);
	~UserObject();

	void Clear();

	static Allocator<UserObject>* GetAllocator();

	uint16 size() const { return DataCount; }

	Variable& operator[](uint16 index) {
		return Data[index];
	}

private:

	Variable* Data;
	uint16 DataCount;
};


class UserDefinedType : public Object
{
public:
	UserDefinedType() {
		Type = VariableType::Object;
	}
	void AddField(const std::string& name, Variable var, const Symbol& flags);
	VariableType GetFieldType(const std::string& name) const;

private:
	friend class ObjectManager;

	ankerl::unordered_dense::map<std::string, Symbol> FieldNames;
	std::vector<Variable> DefaultFields;
};

class ObjectManager
{

public:
	ObjectManager() {}
	VariableType AddType(const std::string& name, const UserDefinedType& obj);
	void RemoveType(const std::string& name);

	Variable Make(VariableType type) const ;
	Variable Make(const std::string& type) const;

	bool GetType(UserDefinedType*& type, const std::string& name);

	bool GetPropertyIndex(uint16& out, const std::string& name, VariableType type);
	bool GetPropertySymbol(Symbol*& symbol, const std::string& name, VariableType type);

private:
	ankerl::unordered_dense::map<VariableType, UserDefinedType> BaseTypes;
	ankerl::unordered_dense::map<std::string, VariableType> NameToType;
	uint32 TypeCounter = (uint32)VariableType::Object;
};

ObjectManager& GetManager();