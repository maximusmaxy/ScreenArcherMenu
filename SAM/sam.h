#pragma once

#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"
#include "f4se/GameForms.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/GameMenus.h"

#include "json.h"
#include "SAF/adjustments.h"
#include "SAF/types.h"

#include "gfx.h"

#include <unordered_map>
#include <mutex>

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

class SamManager {
private:
	std::mutex mutex;
	bool menuOpened;
	

public:
	TESObjectREFR* refr;
	Json::Value data;
	std::string menuName;

	SamManager() : menuOpened(false) {}

	GFxMovieRoot* GetSamRoot();
	void SetOpen(bool isOpen);
	void OpenMenu(BSFixedString menuName);
	void CloseMenu(BSFixedString menuName);
	void TryClose(BSFixedString menuName);
	void SaveAndClose(BSFixedString menuName);
	void Invoke(BSFixedString menuName, const char* name, GFxValue* result, GFxValue* args, UInt32 numArgs);
	void SetVariable(BSFixedString menuName, const char* pVarPath, const GFxValue* value, UInt32 setType = 0);

	void SaveData(GFxValue* data);
	bool LoadData(GFxMovieRoot* root, GFxValue* res);
	void ClearData();

	void CursorAlwaysOn(bool enabled);
	void SetVisible(bool visible);

	void OpenExtensionMenu(const char* name);
	void PushMenu(const char* name);
	void PopMenu();
	void PopMenuTo(const char* name);
	void ShowNotification(const char* name);
	void SetTitle(const char* name);
	void SetMenuNames(VMArray<BSFixedString>& names);
	void SetMenuValues(VMArray<VMVariable>& values);
	void SetMenuItems(VMArray<BSFixedString>& names, VMArray<VMVariable>& values);
	void SetString(const char* msg);
	void SetSuccess();
	void SetError(const char* error);
};

extern SamManager samManager;

TESObjectREFR* GetRefr();
GFxMovieRoot* GetRoot(BSFixedString name);

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