#pragma once
#include "Defines.h"
#include "ankerl/unordered_dense.h"
#include <sstream>

uint32_t CreateVM();

void ReleaseVM(uint32_t handle);

class VM* GetVM(uint32_t handle);

class NameType
{
private:
	const char* Name;

public:
	NameType();
	NameType(const char* text);
	~NameType();

	inline const char* GetName() const {
		return Name;
	}
	auto Length() const { return strlen(Name); }

	bool operator<(const NameType& rhs) const {
		return Name[0] < rhs.Name[0];
	}
	friend bool operator==(const NameType& lhs, const NameType& rhs) {
		return lhs.Name == rhs.Name;
	}
	std::string toString() const;
	operator const char* () const;
	operator size_t() const;
	operator bool() const;
};

namespace std {
	template <>
	class hash<NameType> {
	public:
		auto operator()(const NameType& name) {
			return std::hash<size_t>()((size_t)name);
		}
	};
}

class PathType
{
private:
	std::array<NameType, 5> Path;
	char Size;

public:
	PathType();
	PathType(const char* text, PathType parent = {});
	PathType(NameType text, PathType parent = {});
	~PathType();

	auto& FullPath() const {
		return Path;
	}
	auto Length() const { return Size; }
	constexpr static size_t MaxLength() { return 5; }

	PathType Append(const PathType& name, char off = 0) const;
	PathType Pop() const;
	PathType PopLast() const;
	bool IsChildOf(const PathType& name) const;

	NameType GetFirst() const { return Path[0]; }
	PathType Get(char off) const;
	PathType GetLast() const;

	PathType& operator<<(const PathType& name);
	bool operator<(const PathType& rhs) const {
		return Path[0] < rhs.Path[0];
	}
	friend bool operator==(const PathType& lhs, const PathType& rhs) {
		return lhs.Path == rhs.Path;
	}
	std::string toString() const;
	operator const char* () const;
	operator size_t() const;
	operator bool() const;
};

class PathTypeQuery
{
	PathType Target;
	std::vector<PathType> SearchPaths;

public:
	operator bool() const {
		return Target;
	}

	auto& Paths() { return SearchPaths; }
	auto& GetPaths() const { return SearchPaths; }
	auto& GetTarget() const { return Target; }

	PathTypeQuery() {}
	PathTypeQuery(const PathType& name, std::initializer_list<PathType> list = {}) {
		SearchPaths = list;
		Target = name;
	}
	PathTypeQuery(const PathType& name, const std::vector<PathType>& list) {
		SearchPaths = list;
		Target = name;
	}

	friend bool operator==(const PathTypeQuery& lhs, const PathTypeQuery& rhs) {
		return lhs.Target == rhs.Target;
	}
	friend bool operator==(const PathTypeQuery& lhs, const PathType& rhs) {
		return lhs.Target == rhs;
	}
};

inline BaseLogger& operator<<(BaseLogger& log, const PathType& arg) {
	log << arg.toString();
	return log;
}
inline BaseLogger& operator<<(BaseLogger& log, const PathTypeQuery& arg) {
	log << arg.GetTarget().toString();
	return log;
}

template <>
struct ankerl::unordered_dense::hash<PathType> {
	using is_avalanching = void;

	[[nodiscard]] auto operator()(PathType const& x) const noexcept -> uint64_t {
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

inline PathType toPath(const char* src) {
	auto parts = splits(src, '.');
	PathType out;
	for (auto p = parts.rbegin(); p != parts.rend(); p++) {
		out = out.Append(p->c_str());
	}
	return out;
}

inline NameType toName(const char* src) {
	auto parts = splits(src, '.');
	if (parts.size() > 0) return parts[0].c_str();
	return nullptr;
}

inline PathType operator""_name(const char* src, size_t) {
	return toPath(src);
}