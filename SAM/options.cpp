#include "options.h"

#include "io.h"
#include "constants.h"

Options menuOptions;

void Options::Initialize() {
	hotswap = true;
	alignment = false;
	widescreen = false;
}

void Options::ToJson(Json::Value& value) {
	value["hotswap"] = hotswap;
	value["alignment"] = alignment;
	value["widescreen"] = widescreen;
}

void Options::FromJson(Json::Value& value) {
	hotswap = value.get("hotswap", true).asBool();
	alignment = value.get("alignment", false).asBool();
	widescreen = value.get("widescreen", false).asBool();
}

void GetMenuOptions(GFxResult& result)
{
	result.CreateValues();

	result.PushValue(menuOptions.hotswap);
	result.PushValue(menuOptions.alignment);
	result.PushValue(menuOptions.widescreen);
}

bool GetMenuOption(SInt32 index) {
	switch (index) {
	case kOptionHotswap: return menuOptions.hotswap;
	case kOptionAlignment: return menuOptions.alignment;
	case kOptionWidescreen: return menuOptions.widescreen;
	}

	return false;
}

void SetMenuOption(GFxResult& result, SInt32 index, bool value) {
	switch (index) {
	case kOptionHotswap: menuOptions.hotswap = value; break;
	case kOptionAlignment: menuOptions.alignment = value; break;
	case kOptionWidescreen: menuOptions.widescreen = value; break;
	default: return;
	}

	if (!SaveOptionsFile(OPTIONS_PATH))
		_Log("Failed to save options: ", OPTIONS_PATH);
}