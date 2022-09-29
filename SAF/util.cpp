#include "util.h"

#include "f4se/GameData.h"
#include "conversions.h"

#include <algorithm>
#include <sstream>

void _Log(std::string msg, UInt64 num) {
	_DMESSAGE((msg + std::to_string(num)).c_str());
}

void _Logf(std::string msg, float num) {
	_DMESSAGE((msg + std::to_string(num)).c_str());
}

UInt32 GetFormId(std::string modName, UInt32 formId) {
	const ModInfo* mod = (*g_dataHandler)->LookupModByName(modName.c_str());

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

UInt32 GetFormId(std::string modName, std::string idString) {
	UInt32 formId = std::stoul(idString, nullptr, 16) & 0xFFFFFF;
	return GetFormId(modName, formId);
}

UInt32 GetModId(UInt32 formId)
{
	return (formId & 0xFE000000) == 0xFE000000 ? (formId & 0xFFFFF000) : (formId & 0xFF000000);
}

UInt32 GetBaseId(UInt32 formId)
{
	return (formId & 0xFE000000) == 0xFE000000 ? (formId & 0xFFF) : (formId & 0xFFFFFF);
}

bool TransformIsDefault(NiTransform& transform) {
	if (FloatEqual(transform.pos.x, 0.0f) &&
		FloatEqual(transform.pos.y, 0.0f) &&
		FloatEqual(transform.pos.z, 0.0f) &&
		FloatEqual(transform.rot.arr[0], 1.0f) &&
		FloatEqual(transform.rot.arr[5], 1.0f) &&
		FloatEqual(transform.rot.arr[10], 1.0f) &&
		FloatEqual(transform.scale, 1.0f))
		return true;
	return false;
}

bool TransformMapIsDefault(SAF::TransformMap& map)
{
	for (auto& kvp : map) {
		if (!TransformIsDefault(kvp.second)) {
			return false;
		}
	}
	return true;
}

float Modulo(float a, float b) {
	return fmodf((fmodf(a, b) + b), b);
}

std::string toLower(std::string& str) {
	std::string result(str.size(), '\0');
	std::transform(str.begin(), str.end(), result.begin(), std::tolower);
	return result;
}

std::string getFilename(std::string& path) {
	int lastSlash = path.find_last_of('\\') + 1;
	if (!lastSlash)
		lastSlash = path.find_last_of('/') + 1;

	int lastPeriod = path.find_last_of('.');

	return std::string(path.substr(lastSlash, (path.size() - lastSlash) - (path.size() - lastPeriod)));
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