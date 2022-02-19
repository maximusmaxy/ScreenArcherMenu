#include "util.h"

#include "f4se/GameData.h"

void _Log(std::string msg, UInt64 num) {
	_DMESSAGE((msg + std::to_string(num)).c_str());
}

void _Logf(std::string msg, float num) {
	_DMESSAGE((msg + std::to_string(num)).c_str());
}

void _LogCat(std::string c1, std::string c2) {
	_DMESSAGE((c1 + c2).c_str());
}

//From MCM https://github.com/reg2k/f4mcm/blob/master/src/Utils.cpp
UInt32 GetFormID(std::string modName, std::string formId) {
	const ModInfo* mod = (*g_dataHandler)->LookupModByName(modName.c_str());

	if (mod && mod->modIndex != 0xFF) {
		UInt32 formID = std::stoul(formId, nullptr, 16) & 0xFFFFFF;
		UInt32 flags = mod->recordFlags;
		if (flags & (1 << 9)) {
			// ESL
			formID &= 0xFFF;
			formID |= 0xFE << 24;
			formID |= mod->lightIndex << 12;	// ESL load order
		}
		else {
			formID |= mod->modIndex << 24;
		}
		return formID;
	}
	return 0;
}

bool TransormIsDefault(NiTransform* transform) {
	float heading, attitude, bank;
	transform->rot.GetEulerAngles(&heading, &attitude, &bank);
	if (transform->pos.x == 0.0f &&
		transform->pos.y == 0.0f &&
		transform->pos.z == 0.0f &&
		heading == 0.0f &&
		attitude == 0.0f &&
		bank == 0.0f &&
		transform->scale == 1.0f)
		return true;
	return false;
}

//Case insensitive compare
bool ContainsBSFixed(SAF::NodeSet* set, BSFixedString* str, std::string* res) {
	for (auto it = set->begin(); it != set->end(); it++) {
		if (*str == BSFixedString(it->c_str())) {
			*res = *it;
			return true;
		}
	}
	return false;
}