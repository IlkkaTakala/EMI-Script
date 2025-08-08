#pragma once
#include "Variable.h"
#include <queue>
#include <mutex>

class Object
{
public:
	VariableType getType() const { return Type; }
	Object() : Type(VariableType::Undefined), RefCount(0) {};
	virtual ~Object() {}

	Object(Object&&) noexcept = delete;
	Object & operator=(const Object&) = delete;
	Object & operator=(Object&&) noexcept = delete;

public:
	VariableType Type;
	int RefCount;
};

template <class T>
class Allocator
{
public:
	Allocator() {
		PointerList.push_back(nullptr);
	}

	template <typename ...Args>
	T* Make(const Args&... args) {
		std::unique_lock lk(AllocLock);
		if (FreeList.empty()) {
			auto obj = new T(args...);
			PointerList.push_back(obj);
			return obj;
		}
		else {
			auto idx = FreeList.front();
			FreeList.pop();
			auto& obj = PointerList[idx];
			obj->RefCount = 0;
			constexpr bool hasRealloc = requires(T& t) {
				t.Realloc(args...);
			};
			if constexpr (hasRealloc) {
				obj->Realloc(args...);
			}
			else {
				obj->~T();
				obj = new (obj) T(args...);
			}

			return obj;
		}
	}

	template <typename ...Args>
	T* Copy(T* object) {
		return Make(*object);
	}

	void Free() {
		std::unique_lock lk(AllocLock);
		for (size_t i = 1; i < PointerList.size(); i++) {
			if (PointerList[i]->RefCount == 0) {
				PointerList[i]->RefCount = -1;
				constexpr bool hasClear = requires(T & t) {
					t.Clear();
				};
				if constexpr (hasClear) {
					PointerList[i]->Clear();
				}
				FreeList.push(i);
			}
		}
	}

	void Clear() {
		std::unique_lock lk(AllocLock);
		for (auto ptr : PointerList) {
			delete ptr;
		}
		PointerList.clear();
		while (!FreeList.empty())
			FreeList.pop();
	}

private:

	std::mutex AllocLock;
	std::vector<T*> PointerList;
	std::queue<size_t> FreeList;
};
