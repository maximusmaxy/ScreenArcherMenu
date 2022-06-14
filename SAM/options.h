#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "json/json.h"

#define OPTIONS_PATH "Data\\F4SE\\Plugins\\SAM\\options.json"

extern Json::Value menuOptions;

enum {
	kOptionHotswap = 0,
	kOptionAlignment,
	kOptionWidescreen,
	kOptionAutoplay,

	kOptionMax
};

void GetMenuOptionsGFx(GFxMovieRoot* root, GFxValue* result);
bool GetMenuOption(int index);
void SetMenuOption(int index, bool value);

void LoadOptionsFile();
void SaveOptionsFile();