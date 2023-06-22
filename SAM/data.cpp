#include "data.h"

#include "SAF/util.h"

#include "f4se/GameTypes.h"
#include "f4se/PapyrusScaleformAdapter.h"
#include "f4se/PapyrusArgs.h"

//Json enums need to be identical to AS3 constants so make sure they are updated accordingly
enum {
	kJsonMenuMain = 1,
	kJsonMenuMixed,
	kJsonMenuList,
	kJsonMenuCheckbox,
	kJsonMenuSlider,
	kJsonMenuFolder,
	kJsonMenuFolderCheckbox,
	kJsonMenuAdjustment,
	kJsonMenuGlobal,
	kJsonMenuRemoveable,
};

SAF::InsensitiveUInt32Map jsonMenuTypeMap = {
	{"main", kJsonMenuMain},
	{"mixed", kJsonMenuMixed},
	{"list", kJsonMenuList},
	{"checkbox", kJsonMenuCheckbox},
	{"slider", kJsonMenuSlider},
	{"folder", kJsonMenuFolder},
	{"foldercheckbox", kJsonMenuFolderCheckbox},
	{"adjustment", kJsonMenuAdjustment},
	{"global", kJsonMenuGlobal},
	{"removeable", kJsonMenuRemoveable},
};

enum {
	kJsonPropGet = 1,
	kJsonPropSet,
	kJsonPropEnter,
	kJsonPropInit,
	kJsonPropLeave,
	kJsonPropSave,
	kJsonPropLoad,
	kJsonPropReset,
	kJsonPropExtra,
	kJsonPropNotification,
	kJsonPropTitle,
	kJsonPropKeys,
	kJsonPropEdit,
	kJsonPropWidgets
};

SAF::InsensitiveUInt32Map jsonMenuPropertyMap = {
	{"get", kJsonPropGet},
	{"set", kJsonPropSet},
	{"enter", kJsonPropEnter},
	{"init", kJsonPropInit},
	{"leave", kJsonPropLeave},
	{"save", kJsonPropSave},
	{"load", kJsonPropLoad},
	{"reset", kJsonPropReset},
	{"extra", kJsonPropExtra},
	{"notification", kJsonPropNotification},
	{"title", kJsonPropTitle},
	{"keys", kJsonPropKeys},
	{"edit", kJsonPropEdit},
	{"widgets", kJsonPropWidgets},
};

SAF::InsensitiveUInt32Map jsonMenuHotkeyMap = {
	{"save", kJsonPropSave},
	{"load", kJsonPropLoad},
	{"reset", kJsonPropReset},
	{"extra", kJsonPropExtra}
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
	kJsonHotkeyHold,
	kJsonHotkeyToggle
};

SAF::InsensitiveUInt32Map jsonHotkeyTypeMap = {
	{"func", kJsonHotkeyFunc},
	{"hold", kJsonHotkeyHold},
	{"toggle", kJsonHotkeyToggle}
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
		errorStream << menuName << " failed validation:";
		hasError = true;
	}
	errorStream << std::endl << error;
}

void JsonMenuValidator::LogError(const char* error, const char* error2) {
	if (!hasError) {
		errorStream << menuName << " failed validation:";
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
	if (typeValue.isInt())
		return typeValue.asInt();

	auto it = map.find(typeValue.asCString());
	return (it != map.end() ? it->second : 0);
}

bool JsonMenuValidator::GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value, const char* error, int* result) {
	Json::Value typeValue = value.get("type", "");

	if (typeValue.isInt()) {
		*result = typeValue.asInt();
		return true;
	}

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

	if (typeValue.isInt()) {
		*result = typeValue.asInt();
		return true;
	}

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

	SetDefault(value, "format", "full");
	int format;
	GetType(jsonPathTypeMap, value, "format", "Failed to find folder format", &format);

	Json::Value* func = GetValue(value, "func", "Failed to get function data for folder");
	if (func)
		ValidateFunc(*func);

	for (auto& hotkey : jsonMenuHotkeyMap) {
		if (value.isMember(hotkey.first)) {
			ValidateHotkey(value[hotkey.first]);
		}
	}
}

void JsonMenuValidator::ValidateEntry(Json::Value& value)
{
	Json::Value* func = GetValue(value, "func", "Failed to get function data for entry");
	if (func)
		ValidateFunc(*func);

	Json::Value* text = GetValue(value, "text", nullptr);
	if (text)
		ValidateArgs(*text);
}

void JsonMenuValidator::ValidateArgs(Json::Value& value)
{
	int type;
	GetType(jsonArgsTypeMap, value, "Failed to parse args type: ", &type);

	switch (type) {
	case kJsonArgsVar: RequireProperty(value, "name", "Var argument requires a name property"); break;
	case kJsonArgsIndex: RequireProperty(value, "index", "Index argument requires an index property"); break;
	case kJsonArgsValue: RequireProperty(value, "value", "Value argument requires a value property"); break;
	}
}

void JsonMenuValidator::ValidateFunc(Json::Value& value)
{
	int type;
	GetType(jsonFuncTypeMap, value, "Failed to parse function type: ", &type);

	SetDefault(value, "pop", false);
	SetDefault(value, "refresh", false);
	SetDefault(value, "default", true);

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
		if (RequireProperty(value, "name", "Papyrus form function requires a name for the function") &&
			//RequireProperty(value, "mod", "Papyrus form function requires a mod name") &&
			RequireProperty(value, "id", "Papyrus form function requires an id"))
		{
			//It is likely an int is used instead of string, so make certain that doesn't happen
			Json::Value id = value.get("id", 0);
			if (id.isInt() || id.isInt64()) {
				LogError("Papyrus form function id has int type instead of string type, make sure id is surrounded by quotes eg. \"id\": \"0005DE4D\"");
			}
			else if (!id.isString()) {
				LogError("Papyrus form function id is not a string type");
			}
			else {

				//resolve form id while we're here i guess
				Json::Value modValue = value.get("mod", Json::Value());
				if (modValue.isString()) {
					int formId = GetFormId(modValue.asCString(), id.asCString());
					value["id"] = UInt32ToHexString(formId);
					value.removeMember("mod");

					SetDefault(value, "wait", false);
					SetDefault(value, "timeout", 1000);
				}
			}
		}
		break;
	}
	case kJsonFuncGlobal:
	{
		if (RequireProperty(value, "script", "Papyrus global function requires a script name") &&
			RequireProperty(value, "name", "Papyrus global function requires a name for the function"))
		{
			SetDefault(value, "script", "");
			SetDefault(value, "name", "");
			SetDefault(value, "wait", false);
			SetDefault(value, "timeout", 1000);
		}
		break;
	}
	case kJsonFuncSam:
	{
		RequireProperty(value, "name", "Sam function requires a name");
		break;
	}
	case kJsonFuncLocal:
	{
		RequireProperty(value, "name", "Local function requires a name");
		break;
	}

	}

	Json::Value* args = GetValue(value, "args", nullptr);
	if (args) {
		//might accidently use object type
		if (args->isArray()) {
			for (auto it = args->begin(); it != args->end(); ++it) {
				ValidateArgs(*it);
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

//void JsonMenuValidator::ValidateMove(Json::Value& value)
//{
//	SetDefault(value, "step", 1.0);
//	SetDefault(value, "mod", 0.1);
//
//	Json::Value* func = GetValue(value, "func", "Failed to get function for move hotkey");
//	if (func)
//		ValidateFunc(*func);
//}

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
	//case kJsonHotkeyMove:
	//{
	//	Json::Value* move = GetValue(value, "move", "Failed to get hotkey move data");
	//	if (move)
	//		ValidateMove(*move);
	//	break;
	//}
	case kJsonHotkeyToggle:
	{
		Json::Value* func = GetValue(value, "func", nullptr);
		if (func)
			ValidateFunc(*func);
		break;
	}
	}
}

void JsonMenuValidator::ValidateKeys(Json::Value& value)
{
	for (auto it = value.begin(); it != value.end(); ++it) {
		UInt32 key = StringToUInt32(it.key().asCString());
		if (key != 0) {
			ValidateHotkey(*it);
		}
		else {
			LogError("Key code for \"keys\" was not a valid number or 0");
		}
	}
}

void JsonMenuValidator::ValidateEdit(Json::Value& value)
{
	Json::Value* undo = GetValue(value, "undo", "Failed to get edit undo function data");
	if (undo)
		ValidateFunc(*undo);

	Json::Value* redo = GetValue(value, "redo", "Failed to get edit redo function data");
	if (redo)
		ValidateFunc(*redo);

	Json::Value* start = GetValue(value, "start", nullptr);
	if (start)
		ValidateFunc(*start);

	Json::Value* end = GetValue(value, "end", nullptr);
	if (end)
		ValidateFunc(*end);
}

void JsonMenuValidator::ValidateWidgets(Json::Value& value)
{
	if (!value.isArray())
		LogError("Failed to validate widgets, must be an array type");
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
	if (!slider)
		return;

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
	if (!touch)
		return;
	
	//touch slider defaults
	SetDefault(*touch, "type", "float");
	SetDefault(*touch, "visible", true);
	SetDefault(*touch, "mod", 1.0);
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
	//if (value.isMember("sort")) {
	//	int sortType;
	//	GetType(jsonSortTypeMap, value, "sort", "Failed to get sort type: ", &sortType);
	//}
	//else {
	//	value["sort"] = 0;
	//}

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

void JsonMenuValidator::ValidateGlobalMenu(Json::Value& value)
{
	Json::Value* funcs = GetValue(value, "funcs", "Failed to get funcs for Global");
	if (funcs) {
		for (auto it = funcs->begin(); it != funcs->end(); ++it) {
			ValidateHotkey(*it);
		}
	}
}

void JsonMenuValidator::ValidateRemoveableMenu(Json::Value& value) 
{
	ValidateListMenu(value);
	Json::Value* remove = GetValue(value, "remove", "Failed to remove function for removeable menu");
	if (remove)
		ValidateFunc(*remove);
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
			//		LogError("Get function type cannot be menu, entry or folder");
			//		return false;
			//	}
			//}

			ValidateFunc(*it);
			break;
		}
		case kJsonPropSet:
		case kJsonPropEnter:
		case kJsonPropInit:
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
		case kJsonPropKeys:
			ValidateKeys(*it);
			break;
		case kJsonPropEdit:
			ValidateEdit(*it);
			break;
		case kJsonPropWidgets:
			ValidateWidgets(*it);
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
	case kJsonMenuFolderCheckbox: ValidateFolderMenu(*menuValue); break;
	case kJsonMenuAdjustment: ValidateAdjustmentMenu(*menuValue); break;
	case kJsonMenuGlobal: ValidateGlobalMenu(*menuValue); break;
	case kJsonMenuRemoveable: ValidateRemoveableMenu(*menuValue); break;
	}

	//Defaults
	SetDefault(*menuValue, "extension", false);
	SetDefault(*menuValue, "update", false);
}

void JsonToGFx(GFxMovieRoot* root, GFxValue* result, const Json::Value& value) {
	switch (value.type()) {
	case Json::ValueType::booleanValue:
		result->SetBool(value.asBool());
		break;
	case Json::ValueType::intValue:
		result->SetInt((SInt32)value.asInt());
		break;
	case Json::ValueType::uintValue:
		result->SetUInt((UInt32)value.asUInt());
		break;
	case Json::ValueType::realValue:
		result->SetNumber(value.asDouble());
		break;
	case Json::ValueType::stringValue:
		result->SetString(value.asCString());
		break;
	case Json::ValueType::arrayValue:
	{
		root->CreateArray(result);

		for (auto& member : value) {
			GFxValue arrValue;
			JsonToGFx(root, &arrValue, member);
			result->PushBack(&arrValue);
		}

		break;
	}
	case Json::ValueType::objectValue:
	{
		root->CreateObject(result);

		for (auto it = value.begin(); it != value.end(); ++it) {
			GFxValue objValue;
			JsonToGFx(root, &objValue, *it);
			result->SetMember(it.key().asCString(), &objValue);
		}

		break;
	}
	}
}

Json::Value GFxToJson(GFxValue* value)
{
	switch (value->GetType()) {
	case GFxValue::kType_Bool:
		return Json::Value(value->GetBool());
	case GFxValue::kType_Int:
		return Json::Value((int)value->GetInt());
	case GFxValue::kType_UInt:
	{
		UInt64 uint = value->GetUInt();
		return Json::Value(uint);
	}
	case GFxValue::kType_Number:
		return Json::Value(value->GetNumber());
	case GFxValue::kType_String:
		return Json::Value(value->GetString());
	case GFxValue::kType_Array:
	{
		Json::Value arr(Json::ValueType::arrayValue);
		GFxToJsonArrVisitor arrVisitor(arr);
		value->VisitElements(&arrVisitor, 0, value->GetArraySize());
		return arr;
	}
	case GFxValue::kType_Object:
	{
		Json::Value obj(Json::ValueType::objectValue);
		GFxToJsonObjVisitor visitor(obj);
		value->VisitMembers(&visitor);
		return obj;
	}
	default:
		return Json::Value(Json::ValueType::nullValue);
	}
}

void GFxToJsonObjVisitor::Visit(const char* member, GFxValue* value) {
	json[member] = GFxToJson(value);
}

void GFxToJsonArrVisitor::Visit(UInt32 idx, GFxValue* value) {
	json[(int)idx] = GFxToJson(value);
}

//enum {
//	kJsonOverrideAppend = 1,
//	kJsonOverrideReplace,
//	kJsonOverrideMerge
//};
//
//SAF::InsensitiveUInt32Map jsonOverridePropertyMap = {
//	{"appends", kJsonOverrideAppend},
//	{"replace", kJsonOverrideReplace},
//	{"merge", kJsonOverrideMerge}
//};

void OverrideJson(Json::Value& overrides, Json::Value* dst) {
	if (!overrides.isArray()) {
		_DMESSAGE("Override is not an array type");
		return;
	}

	for (auto& overrider : overrides) {

		//must be overriding an array
		Json::Value name = overrider.get("override", Json::Value());
		if (name.isString()) {
			if (dst->isMember(name.asString())) {
				Json::Value& overidee = (*dst)[name.asString()];
				if (overidee.isArray()) {

					//appends first
					Json::Value append = overrider.get("append", Json::Value());
					if (append.isArray()) {
						for (auto& item : append) {
							overidee.append(item);
						}
					}

					//replaces
					Json::Value replace = overrider.get("replace", Json::Value());
					if (replace.isArray()) {
						for (auto& item : replace) {
							Json::Value index = item.get("index", Json::Value());
							if (index.isInt()) {
								int i = index.asInt();
								if (i < overidee.size()) {
									Json::Value replacer = item.get("replace", Json::Value());
									overidee[i] = replacer;
								}
							}
						}
					}

					//merges
					Json::Value merge = overrider.get("merge", Json::Value());
					if (merge.isArray()) {
						for (auto& item : merge) {
							Json::Value index = item.get("index", Json::Value());
							if (index.isInt()) {
								int i = index.asInt();
								if (i < overidee.size()) {
									Json::Value merger = item.get("merge", Json::Value());
									MergeJsons(merger, &overidee[i]);
								}
							}
						}
					}
				}
			}
		}
	}
}

void MergeJsons(Json::Value& src, Json::Value* dst) {
	Json::ValueType srcType = src.type();

	//if not same type ignore
	if (srcType != dst->type()) {
		return;
	}

	switch (srcType) {
	case Json::ValueType::arrayValue:
	{
		for (int i = 0; i < src.size(); ++i) 
		{
			//if in bounds merge, else add direct
			if (i < dst->size()) 
				MergeJsons(src[i], &(*dst)[i]);
			else 
				(*dst)[i] = src[i];
		}
		break;
	}
				
	case Json::ValueType::objectValue:
	{
		for (auto it = src.begin(); it != src.end(); ++it) {

			//check for override
			if (!_stricmp(it.name().c_str(), "override"))
				OverrideJson(*it, dst);

			//if member merge
			else if (dst->isMember(it.name()))
				MergeJsons(*it, &(*dst)[it.name()]);

			//else add direct
			else
				(*dst)[it.name()] = *it;
		}
		break;
	}

	default: *dst = src; 
		break;
	}
}

void VMVariableToGFx(GFxMovieRoot* root, GFxValue* value, VMVariable* var)
{
	if (var->GetValue().GetTypeEnum() == VMValue::kType_String) {
		value->SetString(var->As<BSFixedString>());
	}
	else {
		PlatformAdapter::ConvertPapyrusValue(value, &var->GetValue(), root);
	}
}

//TODO this will fail for arrays
void GFxToVMVariable(GFxValue* value, VMVariable* var)
{
	switch (value->type)
	{
	case GFxValue::kType_Int:
	{
		SInt32 i = value->GetInt();
		var->Set<SInt32>(&i, false);
		break;
	}
	case GFxValue::kType_UInt:
	{
		UInt32 u = value->GetUInt();
		var->Set<UInt32>(&u, false);
		break;
	}
	case GFxValue::kType_Bool:
	{
		bool b = value->GetBool();
		var->Set<bool>(&b, false);
		break;
	}
	case GFxValue::kType_Number:
	{
		float f = value->GetNumber();
		var->Set<float>(&f, false);
		break;
	}
	case (GFxValue::kType_String | GFxValue::kTypeFlag_Managed):
	case GFxValue::kType_String:
	{
		BSFixedString str(value->GetString());
		var->Set<BSFixedString>(&str, false);
		break;
	}
	default:
		break;
	}
}