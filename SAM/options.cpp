#include "options.h"

#include "io.h"
#include "constants.h"

Options menuOptions;

void Options::Initialize() {
	hotswap = true;
	alignment = false;
	widescreen = false;
	extrahotkeys = true;
	confirmclose = false;
	cameracontrol = true;
	boneoverlay = true;
	posinggizmo = true;
	camerarelative = true;
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
	value["confirmclose"] = confirmclose;
	value["cameracontrol"] = cameracontrol;
	value["boneoverlay"] = boneoverlay;
	value["posinggizmo"] = posinggizmo;
	value["camerarelative"] = camerarelative;
	value["sliderSet"] = sliderSet;
	value["sliderGroups"] = JsonArrayFromSortedSet(sliderGroups);
}

void Options::FromJson(Json::Value& value) {
	hotswap = value.get("hotswap", true).asBool();
	alignment = value.get("alignment", false).asBool();
	widescreen = value.get("widescreen", false).asBool();
	extrahotkeys = value.get("extrahotkeys", true).asBool();
	confirmclose = value.get("confirmclose", false).asBool();
	cameracontrol = value.get("cameracontrol", true).asBool();
	boneoverlay = value.get("boneoverlay", true).asBool();
	posinggizmo = value.get("posinggizmo", true).asBool();
	camerarelative = value.get("camerarelative", true).asBool();
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
	result.PushValue(menuOptions.confirmclose);
	result.PushValue(menuOptions.cameracontrol);
	result.PushValue(menuOptions.boneoverlay);
	result.PushValue(menuOptions.posinggizmo);
	result.PushValue(menuOptions.camerarelative);
}

void SetMenuOption(GFxResult& result, SInt32 index, bool value) {
	switch (index) {
	case Options::Hotswap: menuOptions.hotswap = value; break;
	case Options::Alignment: menuOptions.alignment = value; break;
	case Options::Widescreen: menuOptions.widescreen = value; break;
	case Options::ExtraHotkeys: menuOptions.extrahotkeys = value; break;
	case Options::ConfirmClose: menuOptions.confirmclose = value; break;
	case Options::CameraControl: menuOptions.cameracontrol = value; break;
	case Options::BoneOverlay: menuOptions.boneoverlay = value; break;
	case Options::PosingGizmo: menuOptions.posinggizmo = value; break;
	case Options::CameraRelative: menuOptions.camerarelative = value; break;
	default: return;
	}

	menuOptions.Save();
}