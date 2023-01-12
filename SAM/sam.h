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
#include "SAF/adjustments.h"

#include <unordered_map>
#include <regex>
#include <mutex>

#define SAM_MENU_NAME "ScreenArcherMenu"
#define CURSOR_MENU_NAME "CursorMenu"
#define CONSOLE_MENU_NAME "ConsoleMenu"
#define PHOTO_MENU_NAME "PhotoMenu"

#define MENUS_PATH "Data\\F4SE\\Plugins\\SAM\\Menus"
#define IDLES_PATH "Data\\F4SE\\Plugins\\SAM\\Idles"
#define OVERRIDE_PATH "Data\\F4SE\\Plugins\\SAM\\Override"
#define MENUDATA_PATH "Data\\F4SE\\Plugins\\SAM\\Data"

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
	bool isFemale;
	UInt32 race;
	UInt64 key;

	//virtual TESObjectREFR* Refr();
	void Update(TESObjectREFR* refr);
	void Clear();
};

extern SelectedRefr selected;

extern MenuCache poseMenuCache;
extern MenuCache morphsMenuCache;
extern MenuCache groupsMenuCache;
extern MenuCache exportMenuCache;

extern MenuCategoryList lightsMenuCache;
extern MenuCategoryList tongueMenuCache;

struct NaturalSort {
	bool operator()(const std::string& a, const std::string& b) const;
};

class SamManager {
private:
	std::mutex mutex;
	bool menuOpened;

public:
	TESObjectREFR* refr;
	Json::Value data;

	//IMenu* samMenu;
	//IMenu* cursorMenu;

	SamManager() : menuOpened(false) {}

	void SetOpen(bool isOpen);
	void OpenMenu(BSFixedString menuName);
	void CloseMenu(BSFixedString menuName);
	void TryClose(BSFixedString menuName);
	void Invoke(BSFixedString menuName, const char* name, GFxValue* result, GFxValue* args, UInt32 numArgs);
	void SetVariable(BSFixedString menuName, const char* pVarPath, const GFxValue* value, UInt32 setType = 0);

	void SaveData(GFxValue* data);
	bool LoadData(GFxMovieRoot* root, GFxValue* res);
	void ClearData();

	void CursorAlwaysOn(bool enabled);

	//IMenu* AddRef(BSFixedString& name);
	//void Release();
};

extern SamManager samManager;

TESObjectREFR* GetRefr();
GFxMovieRoot* GetRoot(BSFixedString name);
MenuCategoryList* GetMenu(MenuCache* cache);

//void RegisterSam();
void StartSamQuest();

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible);

bool GetCursor(SInt32* pos);
bool SetCursor(SInt32 x, SInt32 y);
void GetCursorPositionGFx(GFxMovieRoot* root, GFxValue* result);

void OnMenuOpen();
void OnMenuClose();
void OnConsoleUpdate();

void ToggleMenu();
bool OpenSamFile(std::string filename);

typedef std::unordered_map<std::string, Json::Value> JsonCache;
extern JsonCache jsonMenuCache;

void LoadMenuFiles();
void ReloadJsonMenus();
void LoadCachedMenu(GFxMovieRoot* root, GFxValue* result, const char* name);

bool isDotOrDotDot(const char* cstr);
void GetSubFolderGFx(GFxMovieRoot* root, GFxValue* result, const char* path, const char* ext);

extern SAF::SAFDispatcher safDispatcher;

class SAMMessaging {
public:
	PluginHandle pluginHandle;

	F4SEScaleformInterface* scaleform;
	F4SEMessagingInterface* messaging;
	F4SEPapyrusInterface* papyrus;
	F4SESerializationInterface* serialization;
	F4SEInterface* f4se;

	SAMMessaging() :
		pluginHandle(kPluginHandle_Invalid),
		scaleform(nullptr),
		messaging(nullptr),
		papyrus(nullptr),
		serialization(nullptr),
		f4se(nullptr)
	{};
};

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg);

extern SAMMessaging samMessaging;

void SamSerializeSave(const F4SESerializationInterface* ifc);
void SamSerializeLoad(const F4SESerializationInterface* ifc);
void SamSerializeRevert(const F4SESerializationInterface* ifc);