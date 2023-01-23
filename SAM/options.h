#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "json/json.h"

#include "gfx.h"

extern Json::Value menuOptions;

enum {
	kOptionHotswap = 0,
	kOptionAlignment,
	kOptionWidescreen,

	kOptionMax
};

void GetMenuOptionsGFx(GFxResult& result);
bool GetMenuOption(int index);
void SetMenuOption(int index, bool value);

void LoadOptionsFile();
void SaveOptionsFile();