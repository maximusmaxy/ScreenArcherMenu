#include "util.h"

#include "f4se/GameData.h"

#include <algorithm>
#include <sstream>
#include <filesystem>

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
			formId |= mod->modIndex << 24;
		}
		return formId;
	}
	return 0;
}

UInt32 GetFormId(const char* modName, const char* idString) {
	try {
		UInt32 formId = std::stoul(idString, nullptr, 16) & 0xFFFFFF;
		return GetFormId(modName, formId);
	}
	catch (...) 
	{
		return 0;
	}
}

UInt32 GetModId(UInt32 formId)
{
	return (formId & 0xFE000000) == 0xFE000000 ? (formId & 0xFFFFF000) : (formId & 0xFF000000);
}

UInt32 GetBaseId(UInt32 formId)
{
	return (formId & 0xFE000000) == 0xFE000000 ? (formId & 0xFFF) : (formId & 0xFFFFFF);
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
	ModInfo* info = (*g_dataHandler)->modList.modInfoList.Find(FindModForFormId(formId));
	if (!info)
		return nullptr;

	return info->name;
}

float Modulo(float a, float b) {
	return fmodf((fmodf(a, b) + b), b);
}

std::string HexToString(UInt32 hex)
{
	std::stringstream stream;
	stream << std::hex << hex;
	return stream.str();
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