#include "options.h"

#include "io.h"
#include "constants.h"

Options menuOptions;

void Options::Initialize() {
	hotswap = true;
	alignment = false;
	widescreen = false;
	extrahotkeys = true;
}

void Options::ToJson(Json::Value& value) {
	value["hotswap"] = hotswap;
	value["alignment"] = alignment;
	value["widescreen"] = widescreen;
	value["extrahotkeys"] = extrahotkeys;
}

void Options::FromJson(Json::Value& value) {
	hotswap = value.get("hotswap", true).asBool();
	alignment = value.get("alignment", false).asBool();
	widescreen = value.get("widescreen", false).asBool();
	extrahotkeys = value.get("extrahotkeys", true).asBool();
}

void GetMenuOptions(GFxResult& result)
{
	result.CreateValues();

	result.PushValue(menuOptions.hotswap);
	result.PushValue(menuOptions.alignment);
	result.PushValue(menuOptions.widescreen);
	result.PushValue(menuOptions.extrahotkeys);
}

void SetMenuOption(GFxResult& result, SInt32 index, bool value) {
	switch (index) {
	case kOptionHotswap: menuOptions.hotswap = value; break;
	case kOptionAlignment: menuOptions.alignment = value; break;
	case kOptionWidescreen: menuOptions.widescreen = value; break;
	case kOptionExtraHotkeys: menuOptions.extrahotkeys = value; break;
	default: return;
	}

	SaveOptionsFile(OPTIONS_PATH);
}