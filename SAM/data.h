#pragma once

#include "SAF/types.h"
#include "json.h"
#include <sstream>

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
	void ValidateItem(Json::Value& value);
	void ValidateTouch(Json::Value& value);
	void ValidateSlider(Json::Value& value);
	void ValidateCheckbox(Json::Value& value);
	void ValidateList(Json::Value& value);
	void ValidateHotkey(Json::Value& value);
	void ValidateFunc(Json::Value& value);
	void ValidateEntry(Json::Value& value);
	void ValidateFolder(Json::Value& value);
	void ValidateHold(Json::Value& value);

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