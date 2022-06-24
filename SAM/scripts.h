#pragma once

#include "f4se/GameReferences.h"

bool GetMenusHidden();
void SetMenusHidden(bool hidden);
bool ToggleMenusHidden();

void ExecuteMfgObScript(TESObjectREFR* refr, UInt32 id, UInt32 scale);
void ExecuteLookObScript(TESObjectREFR* refr, UInt32 target);