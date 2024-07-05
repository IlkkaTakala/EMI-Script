#pragma once
#include "Defines.h"
#include "ankerl/unordered_dense.h"
#include <sstream>

uint32_t CreateVM();

void ReleaseVM(uint32_t handle);

class VM* GetVM(uint32_t handle);


class TName
{
	std::array<const char*, 5> Path;
	char Size;

public:
	TName();
	TName(const char* text, TName parent = {});
	~TName();

	auto FullPath() const {
		return Path;
	}
	auto Length() const { return Size; }

	TName Append(const TName& name, char off = 0) const;
	bool IsChildOf(const TName& name) const;

	TName Get(char off);

	TName& operator<<(const TName& name);
	bool operator<(const TName& rhs) const;
	friend bool operator==(const TName& lhs, const TName& rhs) {
		return lhs.Path[0] < rhs.Path[0];
	}
	std::string toString() const;
	operator const char* () const;
	operator size_t() const;
	operator bool() const;
};

class TNameQuery
{
	TName Target;
	std::vector<TName> SearchPaths;

public:
	operator bool() const {
		return Target;
	}

	auto& GetPaths() const { return SearchPaths; }
	auto& GetTarget() const { return Target; }

	TNameQuery() {}
	TNameQuery(const TName& name, std::initializer_list<TName> list = {}) {
		SearchPaths = list;
		Target = name;
	}

	friend bool operator==(const TNameQuery& lhs, const TNameQuery& rhs) {
		return lhs.Target == rhs.Target;
	}
	friend bool operator==(const TNameQuery& lhs, const TName& rhs) {
		return lhs.Target == rhs;
	}
};

inline Logger& operator<<(Logger& log, const TName& arg) {
	log << arg.toString();
	return log;
}

template <>
struct ankerl::unordered_dense::hash<TName> {
	using is_avalanching = void;

	[[nodiscard]] auto operator()(TName const& x) const noexcept -> uint64_t {
		return tuple_hash_helper<size_t>::calc_hash(x.FullPath(), std::index_sequence_for<size_t>{});
	}
};

inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
inline void trim(std::string& s) {
	rtrim(s);
	ltrim(s);
}

template <typename Out>
void splits(const std::string& s, char delim, Out result) {
	std::istringstream iss(s);
	std::string item;
	while (std::getline(iss, item, delim)) {
		trim(item);
		*result++ = item;
	}
}

inline std::vector<std::string> splits(const std::string& s, char delim) {
	std::vector<std::string> elems;
	splits(s, delim, std::back_inserter(elems));
	return elems;
}

inline TName operator""_name(const char* src, size_t) {
	auto parts = splits(src, '.');
	TName out;
	for (auto& p : parts) {
		out.Append(p.c_str());
	}
	return out;
}