#pragma once

#include <set>

struct NaturalSort {
	bool operator()(const std::string& a, const std::string& b) const;
};

typedef std::set<std::string, NaturalSort> NaturalSortedSet;
typedef std::map<std::string, std::string, NaturalSort> NaturalSortedMap;