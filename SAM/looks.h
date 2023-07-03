#pragma once

#include "gfx.h"

enum KeywordType {
	AnimArchetype = 7,
	AnimFlavor = 13,
	AnimGender = 14,
	AnimFace = 15
};

void ShowLooksMenu(GFxResult& result);
void GetTypedKeywords(GFxResult& result, UInt32 type);