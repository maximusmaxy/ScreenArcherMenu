#pragma once

#include "gfx.h"

void ShowLooksMenu(GFxResult& result);

void GetLooksHairMods(GFxResult& result);
void GetLooksHairs(GFxResult& result, const char* mod);
void GetLooksEyeMods(GFxResult& result);
void GetLooksEyes(GFxResult& result, const char* mod);
void GetLooksFaceSliderCategories(GFxResult& result);
void GetLooksFaceSliders(GFxResult& result, SInt32 index);