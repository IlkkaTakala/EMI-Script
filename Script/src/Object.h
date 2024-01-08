#pragma once
#include "Defines.h"
#include "Eril/Variable.h"
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
	ObjectType getType() const { return type; }
	Object() : type(ObjectType::None), refCount(0) {};
	virtual ~Object() {

	}

protected:
	ObjectType type;
	int refCount;
};

class String : public Object
{
public:
	String(const char* str) : String(str, strlen(str) + 1) {}

	String(const char* str, size_t s) {
		type = ObjectType::String;
		_size = s;
		_data = new char[s]();
		memcpy(_data, str, s - 1);
		_data[s - 1] = '\0';
	}
	~String() {
		delete[] _data;
	}
	const char* data() const { return _data; }
	size_t size() const { return _size; }

private:
	char* _data;
	size_t _size;
};

class UserDefined : public Object
{
public:
	UserDefined() {
		type = ObjectType::UserDefined;
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

	void AddType(const std::string& name, UserDefined& obj) {
		obj.type = (ObjectType)++typeCounter;
		baseTypes.emplace(name, obj);
	}

	ankerl::unordered_dense::map<std::string, UserDefined> baseTypes;

private:
	uint32 typeCounter = (uint32)ObjectType::UserDefined;
};