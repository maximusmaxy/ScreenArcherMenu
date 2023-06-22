#include "options.h"

#include "io.h"
#include "constants.h"

Options menuOptions;

void Options::Initialize() {
	hotswap = true;
	alignment = false;
	widescreen = false;
	extrahotkeys = true;
	cameracontrol = true;
	boneoverlay = true;
	posinggizmo = true;
	sliderSet = "CBBE";
	sliderGroups = NaturalSortedSet{ "CBBE", "CBBE Bodies" };
}

Json::Value JsonArrayFromSortedSet(NaturalSortedSet& set) {
	Json::Value arr(Json::ValueType::arrayValue);
	for (auto& key : set) {
		arr.append(key);
	}
	return arr;
}

void Options::ToJson(Json::Value& value) {
	value["hotswap"] = hotswap;
	value["alignment"] = alignment;
	value["widescreen"] = widescreen;
	value["extrahotkeys"] = extrahotkeys;
	value["cameracontrol"] = cameracontrol;
	value["boneoverlay"] = boneoverlay;
	value["posinggizmo"] = posinggizmo;
	value["sliderSet"] = sliderSet;
	value["sliderGroups"] = JsonArrayFromSortedSet(sliderGroups);
}

void Options::FromJson(Json::Value& value) {
	hotswap = value.get("hotswap", true).asBool();
	alignment = value.get("alignment", false).asBool();
	widescreen = value.get("widescreen", false).asBool();
	extrahotkeys = value.get("extrahotkeys", true).asBool();
	cameracontrol = value.get("cameracontrol", true).asBool();
	boneoverlay = value.get("boneoverlay", true).asBool();
	posinggizmo = value.get("posinggizmo", true).asBool();
	sliderSet = value.get("sliderSet", "CBBE").asString();

	auto groups = value.get("sliderGroups", Json::Value());
	if (groups.isNull())
		sliderGroups = NaturalSortedSet{ "CBBE", "CBBE Bodies" };
	else {
		sliderGroups.clear();
		for (auto& group : groups) {
			sliderGroups.emplace(group.asString());
		}
	}
}

bool Options::Save()
{
	return SaveOptionsFile(OPTIONS_PATH);
}

void GetMenuOptions(GFxResult& result)
{
	result.CreateValues();

	result.PushValue(menuOptions.hotswap);
	result.PushValue(menuOptions.alignment);
	result.PushValue(menuOptions.widescreen);
	result.PushValue(menuOptions.extrahotkeys);
	result.PushValue(menuOptions.cameracontrol);
	result.PushValue(menuOptions.boneoverlay);
	result.PushValue(menuOptions.posinggizmo);
}

enum {
	kOptionHotswap = 0,
	kOptionAlignment,
	kOptionWidescreen,
	kOptionExtraHotkeys,
	kOptionCameraControl,
	kOptionBoneOverlay,
	kOptionPosingGizmo,
};

void SetMenuOption(GFxResult& result, SInt32 index, bool value) {
	switch (index) {
	case kOptionHotswap: menuOptions.hotswap = value; break;
	case kOptionAlignment: menuOptions.alignment = value; break;
	case kOptionWidescreen: menuOptions.widescreen = value; break;
	case kOptionExtraHotkeys: menuOptions.extrahotkeys = value; break;
	case kOptionCameraControl: menuOptions.cameracontrol = value; break;
	case kOptionBoneOverlay: menuOptions.boneoverlay = value; break;
	case kOptionPosingGizmo: menuOptions.posinggizmo = value; break;
	default: return;
	}

	menuOptions.Save();
}