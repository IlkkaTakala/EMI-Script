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
	void AddField(const TName& name, Variable var, const Symbol& flags);
	VariableType GetFieldType(const TName& name) const;

	VariableType Type;
private:
	friend class ObjectManager;

	ankerl::unordered_dense::map<TName, Symbol> FieldNames;
	std::vector<Variable> DefaultFields;
};

class ObjectManager
{

public:
	ObjectManager() {}
	VariableType AddType(const TName& name, const UserDefinedType& obj);
	void RemoveType(const TName& name);

	Variable Make(VariableType type) const ;

	bool GetType(UserDefinedType*& type, const TName& name);

	bool GetPropertyIndex(uint16_t& out, const TName& name, VariableType type);
	bool GetPropertySymbol(Symbol*& symbol, const TName& name, VariableType type);

private:
	ankerl::unordered_dense::map<VariableType, UserDefinedType> BaseTypes;
	ankerl::unordered_dense::map<TName, VariableType> NameToType;
	uint32_t TypeCounter = (uint32_t)VariableType::Object;
};

ObjectManager& GetManager();