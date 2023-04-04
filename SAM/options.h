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
	bool extrahotkeys;

	void Initialize();
	void ToJson(Json::Value& value);
	void FromJson(Json::Value& value);
};

extern Options menuOptions;

enum {
	kOptionHotswap = 0,
	kOptionAlignment,
	kOptionWidescreen,
	kOptionExtraHotkeys,

	kOptionMax
};

void GetMenuOptions(GFxResult& result);
void SetMenuOption(GFxResult& result, SInt32 index, bool value);