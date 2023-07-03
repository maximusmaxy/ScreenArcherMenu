#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "json/json.h"

#include "gfx.h"
#include "types.h"

class Options {
public:
	enum OptionType {
		Hotswap = 0,
		Alignment,
		Widescreen,
		ExtraHotkeys,
		ConfirmClose,
		CameraControl,
		BoneOverlay,
		PosingGizmo,
		CameraRelative
	};

	bool hotswap;
	bool alignment;
	bool widescreen;
	bool extrahotkeys;
	bool confirmclose;
	bool cameracontrol;
	bool boneoverlay;
	bool posinggizmo;
	bool camerarelative;
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