#include "util.h"

#include "f4se/GameData.h"
#include "conversions.h"

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

bool TransormIsDefault(NiTransform& transform) {
	if (transform.pos.x == 0.0f &&
		transform.pos.y == 0.0f &&
		transform.pos.z == 0.0f &&
		transform.rot.arr[0] == 1.0f &&
		transform.rot.arr[5] == 1.0f &&
		transform.rot.arr[10] == 1.0f &&
		transform.scale == 1.0f)
		return true;
	return false;
}