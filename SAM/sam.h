#pragma once

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameForms.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/GameMenus.h"

#include "json.h"
#include "strnatcmp.h"
#include "SAF/io.h"

#include <unordered_map>
#include <regex>

//class SamMenu : public GameMenuBase
//{
//public:
//	SamMenu();
//
//	virtual void	RegisterFunctions() override;
//	virtual void	Invoke(Args* args) override final;
//};

class SelectedRefr {
public:
	TESObjectREFR* refr;
	NiAVObject* eyeNode;
	bool isFemale;
	UInt32 race;
	UInt64 key;

	//virtual TESObjectREFR* Refr();
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

extern MenuCache poseMenuCache;
extern MenuCache morphsMenuCache;
extern MenuCache groupsMenuCache;
extern MenuCategoryList lightsMenuCache;
extern MenuCache exportMenuCache;

struct NaturalSort {
	bool operator()(const std::string& a, const std::string& b) const;
};

TESObjectREFR* GetRefr();
GFxMovieRoot* GetRoot(BSFixedString name);
MenuCategoryList* GetMenu(MenuCache* cache);

//void RegisterSam();
void StartSamQuest();

void OpenMenu(const char* name);
void CloseMenu(const char* name);
void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible);

bool GetCursor(SInt32* pos);
bool SetCursor(SInt32 x, SInt32 y);
void GetCursorPositionGFx(GFxMovieRoot* root, GFxValue* result);

void OnMenuOpen();
void OnMenuClose();
void OnConsoleUpdate();

void ToggleMenu();
bool OpenSamFile(std::string filename);

void LoadMenuFiles();

void SamSerializeSave(const F4SESerializationInterface* ifc);
void SamSerializeLoad(const F4SESerializationInterface* ifc);
void SamSerializeRevert(const F4SESerializationInterface* ifc);