#include "UserObject.h"

VariableType ObjectManager::AddType(const std::string& name, const UserDefinedType& obj)
{
	if (auto it = NameToType.find(name); it == NameToType.end()) {
		auto nextType = (VariableType)++TypeCounter;
		auto res = BaseTypes.emplace(nextType, obj);
		NameToType[name] = nextType;
		return res.first->second.Type = nextType;
	}
	else {
		return it->second;
	}
}

void ObjectManager::RemoveType(const std::string& name)
{
	auto type = NameToType[name];
	BaseTypes.erase(type);
	NameToType.erase(name);
}

Variable ObjectManager::Make(VariableType type) const
{
	if (auto it = BaseTypes.find(type); it != BaseTypes.end()) {

		UserObject* object = UserObject::GetAllocator()->Make(type, (uint16_t)it->second.DefaultFields.size());
		object->RefCount++;

		uint16_t idx = 0;
		for (auto& field : it->second.DefaultFields) {
			(*object)[idx] = field;
			idx++;
		}

		object->RefCount--;

		return object;
	}
	return Variable();
}

bool ObjectManager::GetType(UserDefinedType*& type, const std::string& name)
{
	auto it = NameToType.find(name);
	if (it == NameToType.end()) {
		return false;
	}
	type = &BaseTypes[it->second];
	return true;
}

bool ObjectManager::GetPropertyIndex(uint16_t& out, const std::string& name, VariableType type)
{
	if (auto it = BaseTypes.find(type); it != BaseTypes.end()) {
		auto& fields = it->second.FieldNames;
		auto field = fields.find(name);
		if (field == fields.end()) {
			return false;
		}

		auto index = std::distance(fields.begin(), field);

		out = static_cast<uint16_t>(index);
		return true;
	}
	return false;
}

bool ObjectManager::GetPropertySymbol(Symbol*& symbol, const std::string& name, VariableType type)
{
	if (auto it = BaseTypes.find(type); it != BaseTypes.end()) {
		auto& fields = it->second.FieldNames;
		auto field = fields.find(name);
		if (field == fields.end()) {
			return false;
		}

		symbol = &field->second;
		return true;
	}

	return false;
}

void UserDefinedType::AddField(const std::string& name, Variable var, const Symbol& flags)
{
	DefaultFields.push_back(var);
	FieldNames.emplace(name, flags); // @todo: might not work
}

VariableType UserDefinedType::GetFieldType(const std::string& name) const
{
	if (auto it = FieldNames.find(name); it != FieldNames.end()) {
		return it->second.VarType;
	}
	return VariableType::Undefined;
}

UserObject::UserObject(VariableType type, uint16_t count)
{
	Type = type;
	Data = new Variable[count];
	DataCount = count;
}

UserObject::~UserObject()
{
	delete[] Data;
	Data = nullptr;
	DataCount = 0;
}

void UserObject::Clear()
{
	for (int i = 0; i < DataCount; ++i) {
		Data[i].setUndefined();
	}
}

Allocator<UserObject>* UserObject::GetAllocator()
{
	static Allocator<UserObject> alloc;
	return &alloc;
}

ObjectManager& GetManager()
{
	static ObjectManager manager;
	return manager;
}
