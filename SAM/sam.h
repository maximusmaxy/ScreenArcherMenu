#pragma once

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameForms.h"
#include "f4se/ScaleformValue.h"

#include "json.h"

#include <unordered_map>

class SelectedRefr {
public:
	TESObjectREFR* refr;
	NiAVObject* eyeNode;
	bool isFemale;
	UInt32 race;
	UInt64 key;

	virtual void Update(TESObjectREFR* refr);
	virtual void Clear();
};

extern SelectedRefr selected;

class SavedMenuData {
public:
	TESObjectREFR* refr;
	Json::Value data;

	void Save(GFxValue* data);
	bool Load(GFxMovieRoot* root, GFxValue* res);
	void Clear();

	void GetGFxValue(GFxMovieRoot* root, GFxValue* result, const Json::Value& value);
};

extern SavedMenuData saveData;

typedef std::vector<std::pair<std::string, std::string>> MenuList;
typedef std::vector<std::pair<std::string, MenuList>> MenuCategoryList;
typedef std::unordered_map<UInt64, MenuCategoryList> MenuCache;

extern MenuCache poseMenuCache;
extern MenuCache morphsMenuCache;

MenuCategoryList* GetMenu(MenuCache* cache);

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible);

void OnMenuOpen();
void OnMenuClose();
void OnConsoleRefUpdate();

void LoadMenuFiles();