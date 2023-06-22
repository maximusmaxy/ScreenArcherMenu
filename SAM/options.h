#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "json/json.h"

#include "gfx.h"
#include "types.h"

class Options {
public:
	bool hotswap;
	bool alignment;
	bool widescreen;
	bool extrahotkeys;
	bool cameracontrol;
	bool boneoverlay;
	bool posinggizmo;
	std::string sliderSet;
	NaturalSortedSet sliderGroups;

	void Initialize();
	void ToJson(Json::Value& value);
	void FromJson(Json::Value& value);
	bool Save();
};

extern Options menuOptions;

void GetMenuOptions(GFxResult& result);
void SetMenuOption(GFxResult& result, SInt32 index, bool value);