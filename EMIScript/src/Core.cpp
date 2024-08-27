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

TName::TName() : Path({ 0 }), Size(0) {}

TName::TName(const char* text, TName parent) : Path({ 0 }), Size(0)
{
    if (text) {
        std::unique_lock lk(StrMutex());

        if (auto it = Strings().find(text); it == Strings().end()) {
            auto [str, success] = Strings().emplace(text);
            Path[0] = str->c_str();
            Size = 1;
        }
        else {
            Path[0] = it->c_str();
            Size = 1;
        }

        if (parent) {
            std::copy(parent.Path.begin(), parent.Path.end(), Path.begin() + 1);
            Size += parent.Size;
        }
    }
}

TName::~TName()
{
}

TName TName::Get(char off) const {
	TName out;
	std::copy(Path.begin() + off, Path.begin() + Size, out.Path.begin());
	out.Size = Size - off;
	return out;
}

TName& TName::operator<<(const TName& name) {
	std::copy(name.Path.begin(), name.Path.begin() + name.Size, Path.begin() + Size);
	Size += name.Size;
	return *this;
}

TName TName::Append(const TName& name, char off) const {
	TName out = *this;
	if (Size + name.Size > Path.size()) return out;
	std::copy(name.Path.begin() + off, name.Path.begin() + name.Size, out.Path.begin() + Size);
	out.Size += name.Size - off;
	return out;
}

TName TName::Pop() const
{
	TName out;
	if (Size == 0) return out;
	std::copy(Path.begin() + 1, Path.begin() + Size, out.Path.begin());
	out.Size = Size - 1;
	return out;
}

TName TName::PopLast() const
{
	TName out = *this;
	if (Size == 0) return out;
	out.Path[--out.Size] = nullptr;
	return out;
}

bool TName::IsChildOf(const TName& name) const {
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

std::string TName::toString() const {
	std::string out;
	for (char i = Size - 1; i >= 0; i--) {
		out += Path[i];
		out += (i == 0 ? "" : ".");
	}
	return out;
}

TName::operator const char* () const {
	return Path[0];
}

TName::operator size_t() const {
	return reinterpret_cast<size_t>(Path[0]);
}

TName::operator bool() const {
	return Path[0] != nullptr;
}