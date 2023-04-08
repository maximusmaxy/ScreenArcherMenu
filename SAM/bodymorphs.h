#pragma once

#include "gfx.h"

void GetBodyMorphs(GFxResult& result);
void SetBodyMorph(GFxResult& result, SInt32 index, SInt32 value);
void ResetBodyMorphs(GFxResult& result);
void LoadSliderSet(GFxResult& result, const char* path);
void SaveBodyslidePreset(GFxResult& result, const char* filename);
void LoadBodyslidePreset(GFxResult& result, const char* path);