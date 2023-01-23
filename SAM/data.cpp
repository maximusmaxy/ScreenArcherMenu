#include "data.h"

#include "SAF/util.h"

//Json enums need to be identical to AS3 constants so make sure they are updated accordingly
enum {
	kJsonMenuMain = 1,
	kJsonMenuMixed,
	kJsonMenuList,
	kJsonMenuCheckbox,
	kJsonMenuSlider,
	kJsonMenuFolder,
	kJsonMenuFolderCheckbox,
	kJsonMenuAdjustment
};

SAF::InsensitiveUInt32Map jsonMenuTypeMap = {
	{"main", kJsonMenuMain},
	{"mixed", kJsonMenuMixed},
	{"list", kJsonMenuList},
	{"checkbox", kJsonMenuCheckbox},
	{"slider", kJsonMenuSlider},
	{"folder", kJsonMenuFolder},
	{"foldercheckbox", kJsonMenuFolderCheckbox},
	{"adjustment", kJsonMenuAdjustment}
};

enum {
	kJsonPropGet = 1,
	kJsonPropSet,
	kJsonPropEnter,
	kJsonPropLeave,
	kJsonPropSave,
	kJsonPropLoad,
	kJsonPropReset,
	kJsonPropExtra,
	kJsonPropNotification,
	kJsonPropTitle
};

SAF::InsensitiveUInt32Map jsonMenuPropertyMap = {
	{"get", kJsonPropGet},
	{"set", kJsonPropSet},
	{"enter", kJsonPropEnter},
	{"leave", kJsonPropLeave},
	{"save", kJsonPropSave},
	{"load", kJsonPropLoad},
	{"reset", kJsonPropReset},
	{"extra", kJsonPropExtra},
	{"notification", kJsonPropNotification},
	{"title", kJsonPropTitle}
};

enum {
	kJsonFuncSam = 1,
	kJsonFuncLocal,
	kJsonFuncForm,
	kJsonFuncGlobal,
	kJsonFuncEntry,
	kJsonFuncMenu,
	kJsonFuncFolder
};

SAF::InsensitiveUInt32Map jsonFuncTypeMap = {
	{"sam", kJsonFuncSam},
	{"local", kJsonFuncLocal},
	{"form", kJsonFuncForm},
	{"global", kJsonFuncGlobal},
	{"entry", kJsonFuncEntry},
	{"menu", kJsonFuncMenu},
	{"folder", kJsonFuncFolder}
};

enum {
	kJsonHotkeyFunc = 1,
	kJsonHotkeyHold
};

SAF::InsensitiveUInt32Map jsonHotkeyTypeMap = {
	{"func", kJsonHotkeyFunc},
	{"hold", kJsonHotkeyHold}
};

enum {
	kJsonItemList = 1,
	kJsonItemSlider,
	kJsonItemCheckbox,
	kJsonItemTouch,
	kJsonItemFolder,
	kJsonItemAdjustment
};

SAF::InsensitiveUInt32Map jsonItemTypeMap = {
	{"list", kJsonItemList},
	{"slider", kJsonItemSlider},
	{"checkbox", kJsonItemCheckbox},
	{"touch", kJsonItemTouch},
	{"folder", kJsonItemFolder},
	{"adjustment", kJsonItemFolder}
};

enum {
	kJsonValueNone = 1,
	kJsonValueInt,
	kJsonValueFloat
};

SAF::InsensitiveUInt32Map jsonValueTypeMap = {
	{"none", kJsonValueNone},
	{"int", kJsonValueInt},
	{"float", kJsonValueFloat}
};

enum {
	kJsonArgsVar = 1,
	kJsonArgsIndex,
	kJsonArgsValue
};

SAF::InsensitiveUInt32Map jsonArgsTypeMap = {
	{"var", kJsonArgsVar},
	{"index", kJsonArgsIndex},
	{"value", kJsonArgsValue}
};

enum {
	kJsonCheckboxCheck = 1,
	kJsonCheckboxSettings,
	kJsonCheckboxRecycle,
	kJsonCheckboxTouch,
	kJsonCheckboxFolder,
	kJsonCheckboxDown,
	kJsonCheckboxUp
};

SAF::InsensitiveUInt32Map jsonCheckboxTypeMap = {
	{"check", kJsonCheckboxCheck},
	{"settings", kJsonCheckboxSettings},
	{"recycle", kJsonCheckboxRecycle},
	{"touch", kJsonCheckboxTouch},
	{"folder", kJsonCheckboxFolder},
	{"down", kJsonCheckboxDown},
	{"up", kJsonCheckboxUp},
};

enum {
	kJsonSortNone = 0,
	kJsonSortAlphanumeric,
	kJsonSortNatural
};

SAF::InsensitiveUInt32Map jsonSortTypeMap = {
	{"none", kJsonSortNone},
	{"alphanumeric", kJsonSortAlphanumeric},
	{"natural", kJsonSortNatural}
};

enum {
	kJsonPathFile = 1,
	kJsonPathRelative,
	kJsonPathFull
};

SAF::InsensitiveUInt32Map jsonPathTypeMap = {
	{"file", kJsonPathFile},
	{"relative", kJsonPathRelative},
	{"full", kJsonPathFull}
};

void JsonMenuValidator::LogError(const char* error) {
	if (!hasError) {
		errorStream << menuName << " failed validation";
		hasError = true;
	}
	errorStream << std::endl << error;
}

void JsonMenuValidator::LogError(const char* error, const char* error2) {
	if (!hasError) {
		errorStream << menuName << " failed validation";
		hasError = true;
	}
	errorStream << std::endl << error << error2;
}

int JsonMenuValidator::GetType(SAF::InsensitiveUInt32Map& map, const char* name) {
	auto it = map.find(name);
	return (it != map.end() ? it->second : 0);
}

int JsonMenuValidator::GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value) {
	Json::Value typeValue = value.get("type", "");
	auto it = map.find(typeValue.asCString());
	return (it != map.end() ? it->second : 0);
}

bool JsonMenuValidator::GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value, const char* error, int* result) {
	Json::Value typeValue = value.get("type", "");
	auto it = map.find(typeValue.asCString());

	if (it != map.end()) {
		*result = it->second;
		value["type"] = (int)it->second;
		return true;
	}
	else {
		*result = 0;
		if (error)
			LogError(error, typeValue.asCString());
	}

	return false;
}

bool JsonMenuValidator::GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value, const char* key, const char* error, int* result)
{
	Json::Value typeValue = value.get(key, "");
	auto it = map.find(typeValue.asCString());

	if (it != map.end()) {
		*result = it->second;
		value[key] = (int)it->second;
		return true;
	}
	else {
		*result = 0;
		if (error)
			LogError(error, typeValue.asCString());
	}

	return false;
}

Json::Value* JsonMenuValidator::GetValue(Json::Value& value, const char* key, const char* error)
{
	if (value.isMember(key))
		return &value[key];

	if (error)
		LogError(error);

	return nullptr;
}

void JsonMenuValidator::SetDefault(Json::Value& value, const char* key, const Json::Value& defaultValue)
{
	value[key] = value.get(key, defaultValue);
}

bool JsonMenuValidator::RequireProperty(Json::Value& value, const char* key, const char* error)
{
	if (!value.isMember(key)) {
		LogError(error);
		return true;
	}

	return true;
}

//This is dumb 

//bool IsDowncased(const char* str) {
//	while (*str != '\0') {
//		if (*str >> 5 != 0b011) //check top 3 bits are 011 for lowercase, 
//			return false;
//		str++;
//	}
//
//	return true;
//}
//
//void ToDowncase(char* str) {
//	while (*str != '\0') {
//		if (*str >> 5 != 0b011) //check top 3 bits are 011 for lowercase, 
//			*str = *str ^ 0b00100000; //flip this bit
//		str++;
//	}
//}
//
//void DowncaseProperties(Json::Value& value) {
//	std::vector<std::string> members;
//
//	//fetch all members that need to be downcased
//	for (auto it = value.begin(); it != value.end(); ++it) {
//		if (!IsDowncased(it.memberName())) {
//			members.push_back(it.memberName());
//		}
//	}
//
//	for (auto& member : members) {
//		Json::Value store = value[member];
//		value.removeMember(member);
//		ToDowncase((char*)member.c_str()); 
//		value[member] = store;
//	}
//}

void JsonMenuValidator::ValidateFolder(Json::Value& value)
{
	SetDefault(value, "pop", true);

	RequireProperty(value, "path", "Path not found for json folder");
	RequireProperty(value, "ext", "Extension not found for json folder");

	//force dot for extension ie ".json"
	Json::Value* ext = GetValue(value, "ext", nullptr);
	if (ext) {
		if (ext->asCString()[0] != '.')
			value["ext"] = ('.' + ext->asString());
	}

	int type;
	GetType(jsonPathTypeMap, value, "type", "Failed to find path type", &type);

	Json::Value* func = GetValue(value, "func", "Failed to get function data for folder");
	if (func)
		ValidateFunc(*func);
}

void JsonMenuValidator::ValidateEntry(Json::Value& value)
{
	Json::Value* func = GetValue(value, "func", "Failed to get function data for entry");
		if (func) {
			ValidateFunc(*func);
		}
}

void JsonMenuValidator::ValidateFunc(Json::Value& value)
{
	int type;
	if (GetType(jsonFuncTypeMap, value, "Failed to parse function type: ", &type))

	switch (type) {
	case kJsonFuncFolder:
	{
		Json::Value* folder = GetValue(value, "folder", "Failed to find folder data for folder function");
		if (folder) {
			ValidateFolder(*folder);
		}
		break;
	}
	case kJsonFuncEntry:
	{
		Json::Value* entry = GetValue(value, "entry", "Failed to find entry data for entry function");
		if (entry) {
			ValidateEntry(*entry);
		}
		break;
	}
	case kJsonFuncForm:
	{
		RequireProperty(value, "id", "Papyrus form function requires an id");

		//It is likely an int is used instead of string, so make certain that doesn't happen
		Json::Value id = value.get("id", 0);
		if (id.isInt() || id.isInt64()) {
			LogError("Papyrus form function id has int type instead of string type, make sure it is \"surrounded by quotes\"");
		}
		else if (!id.isString()) {
			_DMESSAGE("Papyrus form function id is not a string type");
		}
		else {
			//resolve form id while we're here i guess
			int formId = GetFormId(value.get("mod", "").asCString(), id.asCString());
			value["id"] = HexToString(formId);
			value.removeMember("mod");

			SetDefault(value, "name", "");
			SetDefault(value, "wait", false);
			SetDefault(value, "timeout", 1000);
		}
		break;
	}
	case kJsonFuncGlobal:
		SetDefault(value, "script", "");
		SetDefault(value, "name", "");
		SetDefault(value, "wait", false);
		SetDefault(value, "timeout", 1000);

		break;
	}

	Json::Value* args = GetValue(value, "args", nullptr);
	if (args) {
		//might accidently use object type
		if (args->isArray()) {
			for (auto it = args->begin(); it != args->end(); ++it) {
				int argType;
				GetType(jsonArgsTypeMap, *it, "Failed to parse args type: ", &argType);
			}
		}
		else {
			LogError("Function args are not an array type. Use [square brackets] for arrays");
		}
	}
}

void JsonMenuValidator::ValidateHold(Json::Value& value)
{
	SetDefault(value, "step", 1.0);
	SetDefault(value, "mod", 0.1);

	Json::Value* func = GetValue(value, "func", "Failed to get function for hold hotkey");
	if (func)
		ValidateFunc(*func);
}

void JsonMenuValidator::ValidateHotkey(Json::Value& value)
{
	int type;
	GetType(jsonHotkeyTypeMap, value, "Failed to parse hotkey type: ", &type);
	
	switch (type) {
	case kJsonHotkeyFunc:
	{
		Json::Value* func = GetValue(value, "func", "Failed to get hotkey function");
		if (func)
			ValidateFunc(*func);
		break;
	}
	case kJsonHotkeyHold:
	{
		Json::Value* hold = GetValue(value, "hold", "Failed to get hotkey hold data");
		if (hold)
			ValidateHold(*hold);
		break;
	}
	}
}

void JsonMenuValidator::ValidateList(Json::Value& value)
{

}

void JsonMenuValidator::ValidateCheckbox(Json::Value& value)
{
	//Json::Value checkbox;
	//if (!GetValue(value, "checkbox", "Failed to get checkbox data", &checkbox))
	//	return false;
}

void JsonMenuValidator::ValidateSlider(Json::Value& value)
{
	Json::Value* slider = GetValue(value, "slider", "Failed to get slider data");

	//slider defaults
	SetDefault(*slider, "type", "int");
	SetDefault(*slider, "min", 0);
	SetDefault(*slider, "max", 100);
	SetDefault(*slider, "step", 1);
	SetDefault(*slider, "stepkey", 1);
	SetDefault(*slider, "fixed", 4);

	int type;
	GetType(jsonValueTypeMap, *slider, "Failed to get slider type: ", &type);
}

void JsonMenuValidator::ValidateTouch(Json::Value& value)
{
	Json::Value* touch = GetValue(value, "touch", "Failed to get touch data");
	

	//touch slider defaults
	SetDefault(*touch, "type", "float");
	SetDefault(*touch, "visible", true);
	SetDefault(*touch, "mod", 0.01);
	SetDefault(*touch, "step", 1.0);
	SetDefault(*touch, "fixed", 2);

	int type;
	GetType(jsonValueTypeMap, *touch, "Failed to get touch slider type: ", &type);
}

void JsonMenuValidator::ValidateItem(Json::Value& value)
{
	int type;
	GetType(jsonItemTypeMap, value, "Failed to parse menu item type: ", &type);

	switch (type) {
	//case kJsonItemList:
	//	if (!ValidateJsonList(value))
	//		return false;
	//	break;
	case kJsonItemSlider: ValidateSlider(value); break;
	case kJsonItemCheckbox: ValidateCheckbox(value); break;
	case kJsonItemTouch: ValidateTouch(value); break;
	}

	Json::Value* func = GetValue(value, "func", nullptr);
	if (func) {
		ValidateFunc(*func);
	}
}

void JsonMenuValidator::ValidateCheckboxMenu(Json::Value& value)
{

}

void JsonMenuValidator::ValidateSliderMenu(Json::Value& value)
{
	ValidateSlider(value);
	//Json::Value slider;
	//if (GetValue(value, "slider", "Failed to find slider data for slider menu", &slider))
	//	ValidateSlider(slider);
}

void JsonMenuValidator::ValidateAdjustmentMenu(Json::Value& value)
{
	Json::Value* adjustment = GetValue(value, "adjustment", "Failed to find adjustmnet data for adjustment menu");
	if (adjustment) {
		for (auto it = adjustment->begin(); it != adjustment->end(); ++it) {
			ValidateFunc(*it);
		}
	}
}

void JsonMenuValidator::ValidateFolderMenu(Json::Value& value)
{
	Json::Value* folder = GetValue(value, "folder", "Failed to find folder data for folder menu");
	if (folder)
		ValidateFolder(*folder);
}

void JsonMenuValidator::ValidateListMenu(Json::Value& value)
{
	//default sort if exists
	if (value.isMember("sort")) {
		int sortType;
		GetType(jsonSortTypeMap, value, "sort", "Failed to get sort type: ", &sortType);
	}
	else {
		value["sort"] = 0;
	}

	Json::Value* list = GetValue(value, "list", nullptr);
	if (list) {
		if (list->isArray()) {
			for (auto it = list->begin(); it != list->end(); ++it) {
				Json::Value* func = GetValue(*it, "func", nullptr);
				if (func) {
					ValidateFunc(*func);
				}
			}
		}
		else {
			LogError("List menu list is not an array type, make sure it is surrounded by [square brackets]");
		}
	}
}

void JsonMenuValidator::ValidateMixedMenu(Json::Value& value)
{
	//Json::Value items;
	//if (GetValue(value, "items", "Could not find items for mixed menu", &items)) {

	//	if (items.isArray()) {
	//		for (auto it = items.begin(); it != items.end(); ++it) {
	//			ValidateItem(*it);
	//		}
	//	}
	//	else {
	//		LogError("Mixed menu items is not an array type. Make sure the items are surrounded by [square brackets]");
	//	}
	//}

	Json::Value* items = GetValue(value, "items", "Could not find items for mixed menu");
	if (items) {
		if (items->isArray()) {
			for (auto it = items->begin(); it != items->end(); ++it) {
				ValidateItem(*it);
			}
		}
		else {
			LogError("Mixed menu items is not an array type. Make sure the items are surrounded by [square brackets]");
		}
	}
}

void JsonMenuValidator::ValidateMenuProperties(Json::Value& value)
{
	if (!value.isObject()) {
		LogError("Menu is not an object type, make sure it is surrounded by {curly brackets}");
		return;
	}

	for (auto it = value.begin(); it != value.end(); ++it) {
		UInt32 prop = GetType(jsonMenuPropertyMap, it.key().asCString());
		switch (prop) {
		case kJsonPropGet:
		{
			//Get functions can't be entry, menu or folder
			//Json::Value typeProp;
			//if (GetValue(*it, "type", nullptr, &typeProp)) {
			//	int type = typeProp.asInt();
			//	if (type == kJsonFuncEntry || type == kJsonFuncMenu || type == kJsonFuncFolder) {
			//		_DMESSAGE("Get function type cannot be menu, entry or folder");
			//		return false;
			//	}
			//}

			ValidateFunc(*it);
			break;
		}
		case kJsonPropSet:
		case kJsonPropEnter:
		case kJsonPropLeave:
		case kJsonPropNotification:
		case kJsonPropTitle:
			ValidateFunc(*it);
			break;
		case kJsonPropSave:
		case kJsonPropLoad:
		case kJsonPropReset:
		case kJsonPropExtra:
			ValidateHotkey(*it);
			break;
		}
	}
}

//Validates properties and type strings and converts them into constant ints
void JsonMenuValidator::ValidateMenu()
{
	int type;
	GetType(jsonMenuTypeMap, *menuValue, "Failed to parse menu type: ", &type);

	ValidateMenuProperties(*menuValue);

	switch (type) {
	case kJsonMenuMixed: ValidateMixedMenu(*menuValue); break;
	case kJsonMenuList: ValidateListMenu(*menuValue); break;
	case kJsonMenuCheckbox: ValidateCheckboxMenu(*menuValue); break;
	case kJsonMenuSlider: ValidateSliderMenu(*menuValue); break;
	case kJsonMenuFolder: ValidateFolderMenu(*menuValue); break;
	case kJsonMenuAdjustment: ValidateAdjustmentMenu(*menuValue); break;
	}

	//Defaults
	SetDefault(*menuValue, "extension", false);
	SetDefault(*menuValue, "refresh", false);
	SetDefault(*menuValue, "update", false);
}