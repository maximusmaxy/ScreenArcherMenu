#include "util.h"

#include "f4se/GameData.h"
#include "f4se/GameTypes.h"

#include <algorithm>
#include <sstream>
#include <filesystem>
#include <format>

void _Log(const std::string& str) {
	_DMESSAGE(str.c_str());
}

UInt32 GetFormId(const char* modName, UInt32 formId) {
	const ModInfo* mod = (*g_dataHandler)->LookupModByName(modName);

	if (mod && mod->modIndex != 0xFF) {
		UInt32 flags = mod->recordFlags;
		if (flags & (1 << 9)) {
			formId &= 0xFFF;
			formId |= 0xFE << 24;
			formId |= mod->lightIndex << 12;
		}
		else {
			formId &= 0xFFFFFF;
			formId |= mod->modIndex << 24;
		}
		return formId;
	}
	return 0;
}

UInt32 GetFormId(const char* modName, const char* idString) {
	return GetFormId(modName, HexStringToUInt32(idString));
}

class FindModForFormId {
public:
	UInt32 formId;

	FindModForFormId(UInt32 formId) : formId(formId) {}

	bool Accept(ModInfo* modInfo) {
		return modInfo->IsFormInMod(formId);
	}
};

const char* GetModName(UInt32 formId)
{
	auto functor = FindModForFormId(formId);
	ModInfo* info = (*g_dataHandler)->modList.modInfoList.Find(functor);
	if (!info)
		return nullptr;

	return info->name;
}

std::string UInt32ToHexString(UInt32 hex)
{
	std::stringstream stream;
	stream << std::hex << hex;
	return stream.str();
}

UInt32 HexStringToUInt32(const char* str) {
	try {
		return std::stoul(str, nullptr, 16);
	}
	catch (...)
	{
		return 0;
	}
}

UInt32 StringToUInt32(const char* str) {
	try {
		return std::stoul(str);
	}
	catch (...)
	{
		return 0;
	}
}

SInt32 StringToSInt32(const char* str) {
	try {
		return std::stoi(str);
	}
	catch (...)
	{
		return 0;
	}
}

//returns the size of the prefix if match or 0 if no match
UInt32 ComparePostfix(const char* comparer,  UInt32 cLength, const char* postFix, UInt32 pLength)
{
	if (cLength < pLength)
		return 0;
	UInt32 preLength = cLength - pLength;
	const char* comparerPostFix = comparer + preLength;
	if (!_stricmp(comparerPostFix, postFix))
	{
		return preLength;
	}
	return 0;
}

std::string GetPathWithExtension(const char* folder, const char* path, const char* ext)
{
	std::filesystem::path result(folder);
	result.append(path);
	result.concat(ext);

	return result.string();
}

//gets the middle part without the root folders and extension, but with subfolders, use constStrLen where possible
std::string GetRelativePath(int rootLen, int extLen, const char* path)
{
	int pathLen = strlen(path);

	return std::string(path + rootLen + 1, pathLen - rootLen - extLen - 1);
}

void GetLoweredCString(char* buffer, const char* str)
{
	while (*str != 0) {
		*buffer = tolower(*str);
		buffer++;
		str++;
	}
	*buffer = 0;
}

std::string ToFormIdentifier(UInt32 formId) {
	auto modname = GetModName(formId);
	return std::format("{}|{:08X}", modname ? modname : "", GetBaseId(formId));
}

UInt32 FromFormIdentifier(const std::string& str) {
	const char* pipe = strchr(str.c_str(), '|');
	if (!pipe)
		return 0;
	return GetFormId(str.substr(0, pipe - str.c_str()).c_str(), (pipe + 1));
}