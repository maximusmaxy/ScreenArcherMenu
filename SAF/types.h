#pragma once

#include "f4se/NiTypes.h"
#include "f4se/GameTypes.h"
#include "f4se/NiObjects.h"

#include <unordered_set>
#include <unordered_map>
#include <set>
#include <map>

namespace SAF {
	struct NodeKey {
		BSFixedString name;
		bool offset;
		std::size_t key;

		NodeKey() : name(BSFixedString()), offset(false), key(0) {}

		NodeKey(BSFixedString name, bool offset) :
			name(name),
			offset(offset),
			//data is an aligned ptr so +1 should be safe?
			key(reinterpret_cast<std::size_t>(name.data) + (offset ? 0 : 1))
		{}

		void SetOffset(bool _offset) {
			offset = _offset;
			key = reinterpret_cast<std::size_t>(name.data) + (offset ? 0 : 1);
		}
	};

	struct NodeKeyHash {
		std::size_t operator()(const NodeKey& nodeKey) const
		{
			return nodeKey.key;
		}
	};

	struct NodeKeyEqual {
		bool operator()(const NodeKey& lhs, const NodeKey& rhs) const
		{
			return lhs.key == rhs.key;
		}
	};

	struct BSFixedStringHash {
		std::size_t operator()(const BSFixedString& str) const
		{
			return reinterpret_cast<std::size_t>(str.data);
		}
	};

	struct BSFixedStringKeyEqual {
		bool operator()(const BSFixedString& lhs, const BSFixedString& rhs) const
		{
			return lhs == rhs;
		}
	};

	struct CaseInsensitiveCompareStr {
		bool operator() (const std::string& a, const std::string& b) const
		{
			return _stricmp(a.c_str(), b.c_str()) < 0;
		}
	};

	struct CaseInsensitiveCompareCStr {
		bool operator() (const char* a, const char* b) const
		{
			return _stricmp(a, b) < 0;
		}
	};

	typedef std::unordered_set<NodeKey, NodeKeyHash, NodeKeyEqual> NodeSet;
	typedef std::unordered_set<BSFixedString, BSFixedStringHash, BSFixedStringKeyEqual> BSFixedStringSet;
	typedef std::unordered_map<BSFixedString, BSFixedString, BSFixedStringHash, BSFixedStringKeyEqual> BSFixedStringMap;
	typedef std::unordered_map<BSFixedString, NiAVObject*, BSFixedStringHash, BSFixedStringKeyEqual> NodeMap;
	typedef std::unordered_map<BSFixedString, NodeKey, BSFixedStringHash, BSFixedStringKeyEqual> NodeKeyMap;
	typedef std::unordered_map<BSFixedString, std::string, BSFixedStringHash, BSFixedStringKeyEqual> BSFixedStdStringMap;
	typedef std::set<std::string, CaseInsensitiveCompareStr> InsensitiveStringSet;
	typedef std::map<const char*, UInt32, CaseInsensitiveCompareCStr> InsensitiveUInt32Map;
}