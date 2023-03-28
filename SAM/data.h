#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/PapyrusValue.h"
#include "f4se/PapyrusArgs.h"

#include "SAF/types.h"
#include "json.h"
#include <sstream>
#include <vector>

class JsonMenuValidator {
public:
	const char* menuName;
	Json::Value* menuValue;
	std::stringstream errorStream;
	bool hasError;

	JsonMenuValidator(const char* name, Json::Value* value) : menuName(name), menuValue(value), hasError(false) {}

	void ValidateMenu();
	void ValidateMenuProperties(Json::Value& value);
	void ValidateMixedMenu(Json::Value& value);
	void ValidateListMenu(Json::Value& value);
	void ValidateCheckboxMenu(Json::Value& value);
	void ValidateSliderMenu(Json::Value& value);
	void ValidateFolderMenu(Json::Value& value);
	void ValidateAdjustmentMenu(Json::Value& value);
	void ValidateGlobalMenu(Json::Value& value);
	void ValidateItem(Json::Value& value);
	void ValidateTouch(Json::Value& value);
	void ValidateSlider(Json::Value& value);
	void ValidateCheckbox(Json::Value& value);
	void ValidateList(Json::Value& value);
	void ValidateHotkey(Json::Value& value);
	void ValidateFunc(Json::Value& value);
	void ValidateArgs(Json::Value& value);
	void ValidateEntry(Json::Value& value);
	void ValidateFolder(Json::Value& value);
	void ValidateHold(Json::Value& value);
	//void ValidateMove(Json::Value& value);
	void ValidateKeys(Json::Value& value);
	void ValidateEdit(Json::Value& value);
	void ValidateWidgets(Json::Value& value);

	void LogError(const char* error);
	void LogError(const char* error, const char* error2);

	int GetType(SAF::InsensitiveUInt32Map& map, const char* name);
	int GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value);
	bool GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value, const char* error, int* result);
	bool GetType(SAF::InsensitiveUInt32Map& map, Json::Value& value, const char* key, const char* error, int* result);
	Json::Value* GetValue(Json::Value& value, const char* key, const char* error);
	void SetDefault(Json::Value& value, const char* key, const Json::Value& defaultValue);
	bool RequireProperty(Json::Value& value, const char* key, const char* error);
};

void JsonToGFx(GFxMovieRoot* root, GFxValue* result, const Json::Value& value);
Json::Value GFxToJson(GFxValue* value);
void VMVariableToGFx(GFxMovieRoot* root, GFxValue* value, VMVariable* var);
void GFxToVMVariable(GFxValue* value, VMVariable* var);

class GFxToJsonObjVisitor : public GFxValue::ObjectInterface::ObjVisitor
{
public:
	Json::Value& json;

	GFxToJsonObjVisitor(Json::Value& json) : json(json) {};

	void Visit(const char* member, GFxValue* value);
};

class GFxToJsonArrVisitor : public GFxValue::ObjectInterface::ArrayVisitor
{
public:
	Json::Value& json;

	GFxToJsonArrVisitor(Json::Value& json) : json(json) {};

	void Visit(UInt32 idx, GFxValue* val);
};

void MergeJsons(Json::Value& src, Json::Value* dst);