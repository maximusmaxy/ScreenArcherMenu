#pragma once

#include <set>

struct NaturalSort {
	bool operator()(const std::string& a, const std::string& b) const;
};

struct NaturalSortCString {
	bool operator()(const char* a, const char* b) const;
};

typedef std::set<std::string, NaturalSort> NaturalSortedSet;
typedef std::map<std::string, std::string, NaturalSort> NaturalSortedMap;

typedef std::set<const char*, NaturalSortCString> NaturalSortedCStringSet;
typedef std::map<const char*, const char*, NaturalSortCString> NaturalSortedCStringMap;

typedef std::map<const char*, UInt32, NaturalSortCString> NaturalSortedUInt32Map;