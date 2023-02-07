#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "json/json.h"

#include "gfx.h"

class Options {
public:
	bool hotswap;
	bool alignment;
	bool widescreen;

	void Initialize();
	void ToJson(Json::Value& value);
	void FromJson(Json::Value& value);
};

extern Options menuOptions;

enum {
	kOptionHotswap = 0,
	kOptionAlignment,
	kOptionWidescreen,

	kOptionMax
};

void GetMenuOptions(GFxResult& result);
bool GetMenuOption(SInt32 index);
void SetMenuOption(GFxResult& result, SInt32 index, bool value);