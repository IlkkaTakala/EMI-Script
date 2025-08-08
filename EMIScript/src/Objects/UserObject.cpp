#include "UserObject.h"
#include "Helpers.h"

VariableType ObjectManager::AddType(const PathType& name, const UserDefinedType& obj)
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

void ObjectManager::RemoveType(const PathType& name)
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
		for (int i = 0; i < it->second.DefaultFields.size(); ++i) {
			auto& field = it->second.DefaultFields[i];
			auto& deftype = it->second.DefaultTypes[i];
			if (deftype > VariableType::Boolean) {
				if (field.getType() == VariableType::Undefined) {
					(*object)[idx] = GetTypeDefault(deftype);
				}
				else {
					(*object)[idx] = CopyVariable(field);
				}
			}
			else {
				(*object)[idx] = field;
			}
			idx++;
		}

		object->RefCount--;

		return object;
	}
	return Variable();
}

bool ObjectManager::GetType(UserDefinedType*& type, const PathType& name)
{
	auto it = NameToType.find(name);
	if (it == NameToType.end()) {
		return false;
	}
	type = &BaseTypes[it->second];
	return true;
}

bool ObjectManager::GetPropertyIndex(uint16_t& out, const NameType& name, VariableType type)
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

bool ObjectManager::GetPropertySymbol(Symbol*& symbol, const NameType& name, VariableType type)
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

void UserDefinedType::AddField(const NameType& name, Variable var, const Symbol& flags)
{
	DefaultFields.push_back(var);
	DefaultTypes.push_back(flags.VarType);
	FieldNames.emplace(name, flags); // @todo: might not work
}

VariableType UserDefinedType::GetFieldType(const NameType& name) const
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

UserObject::UserObject(const UserObject& object)
{
	Type = object.Type;
	DataCount = object.DataCount;
	Data = new Variable[DataCount];
	for (int i = 0; i < DataCount; ++i) {
		Data[i] = CopyVariable(object.Data[i]);
	}
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
