#pragma once
#include "Defines.h"
#include "ankerl/unordered_dense.h"

uint32_t CreateVM();

void ReleaseVM(uint32_t handle);

class VM* GetVM(uint32_t handle);


class TName
{
	const char* Text;
	size_t ID;

public:
	TName();
	constexpr TName(const char* text);
	~TName();

	friend bool operator==(const TName& lhs, const TName& rhs) {
		return lhs.ID == rhs.ID;
	}

	bool operator<(const TName& rhs) const {
		return ID < rhs.ID;
	}

	const char* toString() const {
		return Text;
	}

	operator const char* () const {
		return Text;
	}

	operator size_t() const {
		return ID;
	}

	operator bool() const {
		return ID > 0;
	}
};

struct TNameQuery
{
	std::vector<TName> NameList;

	operator bool() const {
		return NameList.empty();
	}

	TNameQuery() {}
	TNameQuery(std::initializer_list<TName> list) {
		NameList = list;
	}
};

template <>
struct ankerl::unordered_dense::hash<TName> {
	using is_avalanching = void;

	[[nodiscard]] auto operator()(TName const& x) const noexcept -> uint64_t {
		return detail::wyhash::hash(size_t(x));
	}
};
