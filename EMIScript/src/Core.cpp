#include "Core.h"
#include <ankerl/unordered_dense.h>
#include "VM.h"
#include <unordered_set>

uint32_t Index = 0;
ankerl::unordered_dense::map<uint32_t, VM*> VMs = {};

auto& Strings() {
	static std::unordered_set<std::string> strs;
	return strs;
}
auto& StrMutex() {
	static std::mutex strmutex;
	return strmutex;
}

uint32_t CreateVM()
{
    auto vm = new VM();
    uint32_t idx = ++Index;
    VMs.emplace(idx, vm);

    return idx;
}

void ReleaseVM(uint32_t handle)
{
    delete VMs[handle];
    VMs.erase(handle);
}

VM* GetVM(uint32_t handle)
{
    return VMs[handle];
}

NameType::NameType() : Name(nullptr)
{
}

NameType::NameType(const char* text)
{
	if (text) {
		std::unique_lock lk(StrMutex());

		if (auto it = Strings().find(text); it == Strings().end()) {
			auto [str, success] = Strings().emplace(text);
			Name = str->c_str();
		}
		else {
			Name = it->c_str();
		}
	}
	else {
		Name = nullptr;
	}
}

NameType::~NameType()
{
}

PathType::PathType() : Path({ 0 }), Size(0) {}

PathType::PathType(const char* text, PathType parent) : Path({ 0 }), Size(0)
{
    if (text) {
        Path[0] = text;
        Size = 1;

        if (parent) {
            std::copy(parent.Path.begin(), parent.Path.end(), Path.begin() + 1);
            Size += parent.Size;
        }
    }
}

PathType::PathType(NameType text, PathType parent)
{
	if (text) {
		Path[0] = text;
		Size = 1;

		if (parent) {
			std::copy(parent.Path.begin(), parent.Path.end(), Path.begin() + 1);
			Size += parent.Size;
		}
	}
}

PathType::~PathType()
{
}

PathType PathType::Get(char off) const {
	PathType out;
	std::copy(Path.begin() + off, Path.begin() + Size, out.Path.begin());
	out.Size = Size - off;
	return out;
}

PathType PathType::GetLast() const
{
	if (Size == 0) return PathType();
	PathType out;
	out.Path[0] = Path[Size - 1];
	out.Size = 1;
	return out;
}

PathType& PathType::operator<<(const PathType& name) {
	std::copy(name.Path.begin(), name.Path.begin() + name.Size, Path.begin() + Size);
	Size += name.Size;
	return *this;
}

PathType PathType::Append(const PathType& name, char off) const {
	PathType out = *this;
	if (Size + name.Size > Path.size()) return out;
	std::copy(name.Path.begin() + off, name.Path.begin() + name.Size, out.Path.begin() + Size);
	out.Size += name.Size - off;
	return out;
}

PathType PathType::Pop() const
{
	PathType out;
	if (Size == 0) return out;
	std::copy(Path.begin() + 1, Path.begin() + Size, out.Path.begin());
	out.Size = Size - 1;
	return out;
}

PathType PathType::PopLast() const
{
	PathType out = *this;
	if (Size == 0) return out;
	out.Path[--out.Size] = nullptr;
	return out;
}

bool PathType::IsChildOf(const PathType& name) const {
	if (Path[Size] == name.Path[name.Size]) {
		int j = Size;
		for (int i = name.Size; i >= 0 && j >= 0; i--, j--) {
			if (name.Path[i] != Path[j]) {
				return false;
			}
		}
		return true;
	}
	return false;
}

std::string PathType::toString() const {
	std::string out;
	for (char i = Size - 1; i >= 0; i--) {
		out += Path[i];
		out += (i == 0 ? "" : ".");
	}
	return out;
}

PathType::operator const char* () const {
	return Path[0];
}

PathType::operator size_t() const {
	return reinterpret_cast<size_t>(Path[0].GetName());
}

PathType::operator bool() const {
	return Path[0].GetName() != nullptr && Size > 0;
}

NameType::operator const char* () const {
	return Name;
}

NameType::operator size_t() const {
	return reinterpret_cast<size_t>(Name);
}

NameType::operator bool() const {
	return Name != nullptr;
}

std::string NameType::toString() const {
	return Name;
}