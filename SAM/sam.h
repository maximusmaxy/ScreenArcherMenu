#pragma once

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameForms.h"
#include "f4se/ScaleformValue.h"

class SelectedRefr {
public:
	TESObjectREFR* refr;
	NiAVObject* eyeNode;
	bool isFemale;
	UInt32 race;
	UInt64 key;

	virtual void Update(TESObjectREFR* refr);
	virtual void Clear();
};

extern SelectedRefr selected;

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible);
void OnMenuOpen();
void OnMenuClose();
void OnConsoleRefUpdate();