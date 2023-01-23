#include "sam.h"

#include "f4se/GameMenus.h"
#include "f4se/GameData.h"
#include "f4se/GameTypes.h"
#include "f4se/GameRTTI.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/PluginManager.h"
#include "f4se/CustomMenu.h"
#include "f4se/Serialization.h"
#include "f4se/PapyrusScaleformAdapter.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "SAF/util.h"
#include "SAF/io.h"
#include "SAF/hacks.h"
#include "SAF/eyes.h"

#include "constants.h"
#include "pose.h"
#include "mfg.h"
#include "idle.h"
#include "hacks.h"
#include "positioning.h"
#include "compatibility.h"
#include "options.h"
#include "camera.h"
#include "lights.h"
#include "papyrus.h"
#include "scripts.h"
#include "input.h"
#include "gfx.h"
#include "data.h"

#include <WinUser.h>
#include <libloaderapi.h>

#include <filesystem>

SelectedRefr selected;

MenuCache poseMenuCache;
MenuCache morphsMenuCache;
MenuCache groupsMenuCache;
MenuCache exportMenuCache;

MenuCategoryList lightsMenuCache;
MenuCategoryList tongueMenuCache;

SamManager samManager;

JsonCache menuDataCache;
SAF::InsensitiveStringSet extensionSet;

RelocPtr <BSReadWriteLock> uiMessageLock(0x65774B0);

bool NaturalSort::operator() (const std::string& a, const std::string& b) const 
{
	return strnatcasecmp(a.c_str(), b.c_str()) < 0;
}	

void StartSamQuest()
{
	//If sam still isn't registered, force register it
	static BSFixedString forceRegister("ForceRegister");
	CallSamGlobal(forceRegister);
}

GFxMovieRoot* GetRoot(BSFixedString name)
{
	IMenu* menu = (*g_ui)->GetMenu(name);
	if (!menu)
		return nullptr;

	GFxMovieView* view = menu->movie;
	if (!view)
		return nullptr;

	return view->movieRoot;
}

GFxMovieRoot* SamManager::GetSamRoot()
{
	static BSFixedString samMenuName(SAM_MENU_NAME);

	if (!menuOpened)
		return nullptr;
	
	return GetRoot(samMenuName);
}

void SamManager::SetOpen(bool isOpen)
{
	std::lock_guard<std::mutex> lock(mutex);

	menuOpened = isOpen;
}

void SamManager::OpenMenu(BSFixedString name)
{
	std::lock_guard<std::mutex> lock(mutex);

	if ((*g_ui)->IsMenuRegistered(name) && !(*g_ui)->IsMenuOpen(name)) {
		menuName = MAIN_MENU_NAME;
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(name, kMessage_Open);
	}
}

void SamManager::CloseMenu(BSFixedString name)
{
	std::lock_guard<std::mutex> lock(mutex);

	if ((*g_ui)->IsMenuRegistered(name)) {
		menuOpened = false;
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(name, kMessage_Close);
	}
}

void SamManager::TryClose(BSFixedString name)
{
	std::lock_guard<std::mutex> lock(mutex);

	SaveAndClose(name);
}

void SamManager::SaveAndClose(BSFixedString name)
{
	bool result = false;
	if (menuOpened) {
		IMenu* menu = (*g_ui)->GetMenu(name);
		if (menu) {

			GFxMovieView* view = menu->movie;
			if (view) {

				GFxMovieRoot* root = view->movieRoot;
				if (root) {

					GFxValue gfxResult;
					root->Invoke("root1.Menu_mc.tryClose", &gfxResult, nullptr, 0);
					result = gfxResult.GetBool();
				}
			}
		}

		if (result) {
			menuOpened = false;
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(name, kMessage_Close);
		}
	}
}

void SamManager::Invoke(BSFixedString name, const char* func, GFxValue* result, GFxValue* args, UInt32 numArgs)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (menuOpened) {
		GFxMovieRoot* root = GetRoot(name);
		if (root)
			root->Invoke(func, result, args, numArgs);
	}
}

void SamManager::SetVariable(BSFixedString name, const char* pVarPath, const GFxValue* value, UInt32 setType)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (menuOpened) {
		GFxMovieRoot* root = GetRoot(name);
		if (root)
			root->SetVariable(pVarPath, value, setType);
	}
}

void SamManager::SaveData(GFxValue* saveData)
{
	//std::lock_guard<std::mutex> lock(mutex); is called during try close

	if (!selected.refr)
		return;

	//copying to a json for saved menu data instead of trying to understand how GFx managed memory works
	data.clear();
	GFxToJsonObjVisitor visitor(data);
	saveData->VisitMembers(&visitor);

	refr = selected.refr;
}

bool SamManager::LoadData(GFxMovieRoot* root, GFxValue* result)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!menuOpened)
		return false;

	//if ref updated save data is invalidated so ignore
	if (!refr || selected.refr != refr) {
		ClearData();
		return false;
	}

	//Check the saved menu name matches the menu being opened
	if (_stricmp(data.get("rootMenu", "").asCString(), menuName.c_str())) {
		ClearData();
		return false;
	}

	JsonToGFx(root, result, data);

	ClearData();
	return true;
}

void SamManager::ClearData()
{
	refr = nullptr;
}

void SamManager::OpenExtensionMenu(const char* name)
{
	std::lock_guard<std::mutex> lock(mutex);

	static BSFixedString samMenuName(SAM_MENU_NAME);

	if (menuOpened) {
		//close it
		if (!_stricmp(menuName.c_str(), name)) {
			SaveAndClose(samMenuName);
		}

		return;
	}

	if ((*g_ui)->IsMenuRegistered(samMenuName) && !(*g_ui)->IsMenuOpen(samMenuName)) {
		menuName = name;
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(samMenuName, kMessage_Open);
	}
}

void SamManager::PushMenu(const char* name)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue result;
	GFxValue args(name);
	root->Invoke("root1.Menu_mc.PushMenu", &result, &args, 1);
}

void SamManager::PopMenu()
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.PopMenu", &ret, nullptr, 0);
}

void SamManager::PopMenuTo(const char* name)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.PopMenuTo", &ret, &GFxValue(name), 1);
}

void SamManager::ShowNotification(const char* msg)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue args(msg);
	GFxValue ret;
	root->Invoke("root1.Menu_mc.showNotification", &ret, &args, 1);
}

void SamManager::SetTitle(const char* title)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue args(title);
	GFxValue ret;
	root->Invoke("root1.Menu_mc.setTitle", &ret, &args, 1);
}

void SamManager::SetMenuNames(VMArray<BSFixedString>& vmNames)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(&args);
	result.CreateNames();

	for (int i = 0; i < vmNames.Length(); i++) {
		BSFixedString name;
		vmNames.Get(&name, i);
		result.PushName(name.c_str());
	}

	result.Invoke(root, "root1.Menu_mc.PapyrusResult");
}

void SamManager::SetMenuValues(VMArray<VMVariable>& vmValues)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(&args);
	result.CreateValues();

	for (int i = 0; i < vmValues.Length(); i++) {
		VMVariable var;
		vmValues.Get(&var, i);

		GFxValue value;
		PlatformAdapter::ConvertPapyrusValue(&value, &var.GetValue(), root);

		result.PushValue(&value);
	}

	result.Invoke(root, "root1.Menu_mc.PapyrusResult");
}

void SamManager::SetMenuItems(VMArray<BSFixedString>& vmNames, VMArray<VMVariable>& vmValues)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(&args);

	//make sure both arrays are the same length
	//if (vmNames.Length() != vmValues.Length()) {
	//	result.SetError(MENU_ITEM_LENGTH_ERROR);
	//	return result.Invoke(root, "root1.Menu_mc.PapyrusResult");
	//}

	result.CreateMenuItems();

	for (int i = 0; i < vmNames.Length(); ++i) {
		BSFixedString name;
		vmNames.Get(&name, i);
		result.PushName(name.c_str());
	}

	for (int i = 0; i < vmValues.Length(); i++) {
		VMVariable var;
		vmValues.Get(&var, i);
		
		GFxValue value;
		PlatformAdapter::ConvertPapyrusValue(&value, &var.GetValue(), root);

		result.PushValue(&value);
	}

	result.Invoke(root, "root1.Menu_mc.PapyrusResult");
}

void SamManager::SetString(const char* msg)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(&value);
	result.SetString(msg);
	result.Invoke(root, "root1.Menu_mc.PapyrusResult");
}

void SamManager::SetSuccess()
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(&value); 
	result.Invoke(root, "root1.Menu_mc.PapyrusResult"); //success is default
}

void SamManager::SetError(const char* error)
{
	std::lock_guard<std::mutex> lock(mutex);

	auto root = GetSamRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(&value);
	result.SetError(error);
	result.Invoke(root, "root1.Menu_mc.PapyrusResult");
}

//typedef void (*_ScaleformRefCountImplAddRef)(IMenu* menu);
//RelocAddr<_ScaleformRefCountImplAddRef> ScaleformRefCountImplAddRef(0x210EBF0);
//
//typedef void (*_ScaleformRefCountImplRelease)(IMenu* menu);
//RelocAddr<_ScaleformRefCountImplRelease> ScaleformRefCountImplRelease(0x210EC90);
//
//IMenu* SamManager::AddRef(BSFixedString& name)
//{
//	std::lock_guard<std::mutex> lock(mutex);
//
//	IMenu* menu = (*g_ui)->GetMenu(name);
//
//	if (menu) {
//		ScaleformRefCountImplAddRef(menu);
//		return menu;
//	}
//
//	return nullptr;
//}

//void SamManager::Release()
//{
//	std::lock_guard<std::mutex> lock(mutex);
//
//	static BSFixedString samMenuName(SAM_MENU_NAME);
//	static BSFixedString cursorMenuName(CURSOR_MENU_NAME);
//
//	if (samMenu) {
//		ScaleformRefCountImplRelease(samMenu);
//		_Log("Sam ref count: ", samMenu->refCount);
//	}
//	else {
//		_DMESSAGE("Sam null");
//	}
//
//	if (cursorMenu) {
//		ScaleformRefCountImplRelease(cursorMenu);
//		_Log("Cursor ref count: ", samMenu->refCount);
//	}
//	else {
//		_DMESSAGE("Cursor null");
//	}
//
//	samMenu = nullptr;
//	cursorMenu = nullptr;
//}

void SamManager::CursorAlwaysOn(bool enabled) {
	std::lock_guard<std::mutex> lock(mutex);

	static BSFixedString cursorMenuName(CURSOR_MENU_NAME);

	IMenu* menu = (*g_ui)->GetMenu(cursorMenuName);
	if (menu) {
		if (enabled)
			menu->flags |= 0x02;
		else
			menu->flags &= ~0x02;
	}
}

void SamManager::SetVisible(bool visible) {
	std::lock_guard<std::mutex> lock(mutex);

	static BSFixedString samMenuName(SAM_MENU_NAME);
	SetMenuVisible(samMenuName, "root1.Menu_mc.visible", visible);
}

MenuCategoryList* GetMenu(MenuCache* cache)
{
	if (!selected.refr) 
		return nullptr;

	if (cache->count(selected.key))
		return &(*cache)[selected.key];
	if (selected.isFemale && cache->count(selected.race))
		return &(*cache)[selected.race];

	//Default to human
	UInt32 race = 0x00013746;
	UInt64 key = race;
	if (selected.isFemale)
		key |= 0x100000000;

	if (cache->count(key))
		return &(*cache)[key];
	if (selected.isFemale && cache->count(race))
		return &(*cache)[race];

	return nullptr;
}

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible)
{
	BSReadLocker locker(g_menuTableLock);
	if ((*g_ui)->IsMenuRegistered(menuName) && (*g_ui)->IsMenuOpen(menuName)) {
		GFxMovieRoot* root = GetRoot(menuName);
		if (root) {
			root->SetVariable(visiblePath, &GFxValue(visible));
		}
	}
}

bool GetCursor(SInt32* pos) 
{
	POINT point;
	bool result = GetCursorPos(&point);
	if (result) {
		pos[0] = point.x;
		pos[1] = point.y;
	}
	return result;
}

bool SetCursor(SInt32 x, SInt32 y)
{
	return SetCursorPos(x, y);
}

void GetCursorPositionGFx(GFxMovieRoot* root, GFxValue* result) {
	root->CreateObject(result);

	SInt32 pos[2];
	bool success = GetCursor(pos);

	result->SetMember("success", &GFxValue(success));
	result->SetMember("x", &GFxValue(pos[0]));
	result->SetMember("y", &GFxValue(pos[1]));
}

TESObjectREFR * GetRefr() {
	UInt32 handle = (*g_consoleHandle);
	NiPointer<TESObjectREFR> refr;
	if (handle == 0 || handle == *g_invalidRefHandle) {
		refr = *g_player;
	} else {
		LookupREFRByHandle(handle, refr);
		if (!refr || refr->formType != kFormType_ACHR || (refr->flags & TESObjectREFR::kFlag_IsDeleted))
			return nullptr;
	}
	return refr;
}

//TESObjectREFR* SelectedRefr::Refr() {
//	TESForm* form = LookupFormByID(formId);
//	if (!form) return nullptr;
//
//	TESObjectREFR* ref = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
//	return ref;
//}

void SelectedRefr::Update(TESObjectREFR* newRefr) {
	if (!newRefr) {
		Clear();
		return;
	}
	refr = newRefr;
	TESNPC* npc = (TESNPC*)refr->baseForm;
	if (npc) {
		isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);
		TESRace* actorRace = refr->GetActorRace();
		race = (actorRace ? actorRace->formID : 0x13746);
	}
	else {
		isFemale = false;
		race = 0x13746;
	}
	key = race;
	if (isFemale)
		key += 0x100000000;
}

void SelectedRefr::Clear() {
	refr = nullptr;
}

void GetMenuTarget(GFxValue& data) {
	if (selectedNonActor.refr && selectedNonActor.refr->baseForm) {
		const char* name = "";
		switch (selectedNonActor.refr->baseForm->formType) {
		case kFormType_NPC_:
			TESNPC* actor = DYNAMIC_CAST(selectedNonActor.refr->baseForm, TESForm, TESNPC);
			name = actor->fullName.name.c_str();
			break;
		}
		data.SetMember("title", &GFxValue(name));
	}
	else {
		data.SetMember("title", &GFxValue("No Target"));
	}
}

void OnMenuOpen() {
	static BSFixedString samMenu(SAM_MENU_NAME);
	static BSFixedString photoMenu(PHOTO_MENU_NAME);

	samManager.SetOpen(true);

	GFxMovieRoot* root = GetRoot(samMenu);
	if (!root) {
		_DMESSAGE("Could not find screen archer menu");
		return;
	}

	//Store ffc lock state and lock screen
	LockFfc(true);

	//Load options
	LoadOptionsFile();

	TESObjectREFR* refr = GetRefr();
	selected.Update(refr);
	UpdateNonActorRefr();

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", false);

	GFxValue data;
	root->CreateObject(&data);

	data.SetMember("menuName", &GFxValue(samManager.menuName.c_str()));

	GFxValue delayClose((*g_ui)->IsMenuOpen(photoMenu));
	data.SetMember("delayClose", &delayClose);

	GetMenuTarget(data);

	GFxValue saved;
	if (samManager.LoadData(root, &saved))
		data.SetMember("saved", &saved);

	GFxValue widescreen(GetMenuOption(kOptionWidescreen));
	data.SetMember("widescreen", &widescreen);

	GFxValue alignment(GetMenuOption(kOptionAlignment));
	data.SetMember("swap", &alignment);

	samManager.Invoke(samMenu, "root1.Menu_mc.menuOpened", nullptr, &data, 1);
}

void OnMenuClose() {
	static BSFixedString photoMenu("PhotoMenu");

	//Restore ffc lock
	LockFfc(false);

	//Update options file
	SaveOptionsFile();

	selected.Clear();

	//SetMenusHidden(false); Doesn't work

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);
}

void OnConsoleUpdate() {
	static BSFixedString samMenu(SAM_MENU_NAME);

	GFxMovieRoot* root = GetRoot(samMenu);
	if (!root) {
		_DMESSAGE("Could not find screen archer menu");
		return;
	}

	GFxValue data;
	root->CreateObject(&data);

	UpdateNonActorRefr();
	TESObjectREFR * refr = GetRefr();
	bool refUpdated = false;

	if (selected.refr != refr && GetMenuOption(kOptionHotswap))
	{
		selected.Update(refr);
		refUpdated = true;
	}

	GFxValue updated(refUpdated);
	data.SetMember("updated", &updated);

	GetMenuTarget(data);

	samManager.Invoke(samMenu, "root1.Menu_mc.consoleRefUpdated", nullptr, &data, 1);
}

void ToggleMenu() {
	static BSFixedString menuName(SAM_MENU_NAME);

	if ((*g_ui)->IsMenuRegistered(menuName)) {
		if ((*g_ui)->IsMenuOpen(menuName)) {
			samManager.TryClose(menuName);
		}
		else {
			samManager.OpenMenu(menuName);
		}
	}
}

SAMMessaging samMessaging;
SAF::SAFDispatcher safDispatcher;

class SamOpenCloseHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual ~SamOpenCloseHandler() { };
	virtual	EventResult	ReceiveEvent(MenuOpenCloseEvent* evn, void* dispatcher) override
	{
		BSFixedString samMenu(SAM_MENU_NAME);
		BSFixedString photoMenu(PHOTO_MENU_NAME);
		BSFixedString consoleMenu(CONSOLE_MENU_NAME);
		BSFixedString cursorMenu(CURSOR_MENU_NAME);
		BSFixedString containerMenu(CONTAINER_MENU_NAME);

		if (evn->menuName == samMenu) {
			if (evn->isOpen) {
				inputHandler.enabled = true;
				BSInputEventUser* handlerPtr = &inputHandler;
				int idx = (*g_menuControls)->inputEvents.GetItemIndex(handlerPtr);
				if (idx == -1) {
					_DMESSAGE("ScreenArcherMenu Registered for input");
					(*g_menuControls)->inputEvents.Push(handlerPtr);
				}
				OnMenuOpen();
			}
			else {
				OnMenuClose();
				inputHandler.enabled = false;
			}
		}
		else {
			if ((*g_ui)->IsMenuOpen(samMenu)) {
				if (evn->menuName == consoleMenu) {
					//console opened while sam is open
					if (evn->isOpen) {
						inputHandler.enabled = false;
					}
					//console closed while same is open
					else {
						OnConsoleUpdate();
						inputHandler.enabled = true;
					}
				}
				else if (evn->menuName == cursorMenu) {
					//cursor opened while sam is open
					if (evn->isOpen) {
						samManager.CursorAlwaysOn(true);
					}
				}
				else if (evn->menuName == containerMenu) {
					//container menu opened while sam is open
					if (evn->isOpen) {
						inputHandler.enabled = false;
						samManager.SetVisible(false);
					}
					//container menu closed while sam is open
					else {
						inputHandler.enabled = true;
						samManager.SetVisible(true);
					}
				}
			}
			else {
				if (evn->menuName == cursorMenu) {
					//cursor opened while sam is closed
					if (evn->isOpen) {
						samManager.CursorAlwaysOn(false);
					}
				}
			}
		}
		return kEvent_Continue;
	};
};

SamOpenCloseHandler openCloseHandler;

class SAMEventReciever :
	public BSTEventSink<TESLoadGameEvent>
{
public:
	EventResult	ReceiveEvent(TESLoadGameEvent* evn, void* dispatcher)
	{
		//RegisterSam();
		StartSamQuest();
		return kEvent_Continue;
	}
};

SAMEventReciever samEventReciever;

void SAFDispatchReciever(F4SEMessagingInterface::Message* msg)
{
	safDispatcher.Recieve(msg);
	if (msg->type == SAF::kSafAdjustmentManager) {
		LoadMenuFiles();
	}
}

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case F4SEMessagingInterface::kMessage_PostLoad:
	{
		if (samMessaging.messaging)
			samMessaging.messaging->RegisterListener(samMessaging.pluginHandle, "SAF", SAFDispatchReciever);

		if (samMessaging.f4se) {
			RegisterCompatibility(samMessaging.f4se);
		}
		break;
	}
	case F4SEMessagingInterface::kMessage_GameDataReady:
	{
		if (msg->data)
			(*g_ui)->menuOpenCloseEventSource.AddEventSink(&openCloseHandler);
		break;
	}
	case F4SEMessagingInterface::kMessage_GameLoaded:
	{
		GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&samEventReciever);
	}
	}
}

enum {
	kMenuTypePose = 1,
	kMenuTypeMorphs,
	kMenuTypeGroups,
	kMenuTypeLights,
	kMenuTypeExport,
	kMenuTypeTongue
};

SAF::InsensitiveUInt32Map samTypeMap = {
	{"pose", kMenuTypePose},
	{"morphs", kMenuTypeMorphs},
	{"groups", kMenuTypeGroups},
	{"lights", kMenuTypeLights},
	{"export", kMenuTypeExport},
	{"tongue", kMenuTypeTongue},
};

class SamTsvReader : public SAF::TsvReader {
public:

	const ModInfo* mod;
	MenuCategoryList* menu;

	SamTsvReader(std::string path, SAF::InsensitiveUInt32Map* typeMap) :
		SAF::TsvReader(path, typeMap),
		mod(nullptr),
		menu(nullptr)
	{};

	void ReadCategory(std::string m1, std::string m2)
	{
		//Try find existing category
		UInt32 size = menu->size();
		for (categoryIndex = 0; categoryIndex < size; categoryIndex++) {
			if ((*menu)[categoryIndex].first == m2) {
				break;
			}
		}

		//if category not found add it
		if (categoryIndex >= size) {
			MenuList list;
			menu->push_back(std::make_pair(m2, list));
			categoryIndex = menu->size() - 1;
		}
	}

	bool FinalizeHeader(std::string m1, std::string m2) 
	{ 
		if (!header.type)
		{
			_Log("Failed to read header type", path.c_str());
			return false;
		}

		//verify header
		switch (header.type)
		{
		case kMenuTypeLights:
		{
			mod = (*g_dataHandler)->LookupModByName(header.mod.c_str());
			if (!mod || mod->modIndex == 0xFF)
			{
				_Log("Failed to read header mod", path.c_str());
				return false;
			}

			UInt32 modId = (mod->recordFlags & (1 << 9)) ? ((0xFE << 24) | (mod->lightIndex << 12)) : (mod->modIndex << 24);
			lightModMap.emplace(modId, mod->name);
			break;
		}
		case kMenuTypeTongue:
		{
			//no verification needed
			break;
		}
		default: {
			key = GetFormId(header.mod.c_str(), header.form.c_str());
			if (!key)
			{
				_Log("Failed to read header race or mod", path.c_str());
				return false;
			}
			if (header.isFemale)
				key |= 0x100000000;
			break;
		}
		}

		switch (header.type)
		{
		case kMenuTypePose: menu = &poseMenuCache[key]; break;
		case kMenuTypeMorphs: menu = &morphsMenuCache[key]; break;
		case kMenuTypeGroups: menu = &groupsMenuCache[key]; break;
		case kMenuTypeLights: menu = &lightsMenuCache; break;
		case kMenuTypeExport: menu = &exportMenuCache[key]; break;
		case kMenuTypeTongue: menu = &tongueMenuCache; break;
		}

		return true;
	}

	void ReadLine(std::string m1, std::string m2) 
	{
		if (header.type == kMenuTypeLights) {
			//merge the mod id in now
			UInt32 formId = std::stoul(m1, nullptr, 16) & 0xFFFFFF;

			UInt32 flags = mod->recordFlags;
			if (flags & (1 << 9)) {
				formId &= 0xFFF;
				formId |= 0xFE << 24;
				formId |= mod->lightIndex << 12;
			}
			else {
				formId |= mod->modIndex << 24;
			}

			(*menu)[categoryIndex].second.push_back(std::make_pair(HexToString(formId), m2));
		}
		else {
			(*menu)[categoryIndex].second.push_back(std::make_pair(m1, m2));
		}
	}
};

bool LoadMenuFile(const char* path) 
{
	SamTsvReader reader(path, &samTypeMap);
	return reader.Read();
}

bool LoadIdleFile(const char* path) {

	Json::Value value;
	if (!SAF::ReadJsonFile(path, value))
		return false;

	for (auto it = value.begin(); it != value.end(); ++it) {
		IdleData data;

		std::string mod = it->get("mod", "").asString();
		std::string id = it->get("race", "").asString();
		data.raceId = GetFormId(mod.c_str(), id.c_str());

		Json::Value reset = it->get("reset", Json::Value());
		mod = reset.get("mod", "").asString();
		id = reset.get("idle", "").asString();
		data.resetId = GetFormId(mod.c_str(), id.c_str());

		Json::Value filter = it->get("filter", Json::Value());
		data.behavior = BSFixedString(filter.get("behavior", "").asCString());
		data.event = BSFixedString(filter.get("event", "").asCString());

		//default to human a pose
		Json::Value apose = it->get("apose", Json::Value());
		mod = apose.get("mod", SAM_ESP).asString();
		id = apose.get("idle", "802").asString();
		data.aposeId = GetFormId(mod.c_str(), id.c_str());

		raceIdleData[data.raceId] = data;
	}

	return true;
}

bool LoadJsonMenu(const char* path)
{
	Json::Value value;
	if (!SAF::ReadJsonFile(path, value))
		return false;

	std::string stem = std::filesystem::path(path).stem().string();

	JsonMenuValidator validator(stem.c_str(), &value);
	validator.ValidateMenu();
	if (validator.hasError) {
		_DMESSAGE(validator.errorStream.str().c_str());
		return false;
	}

	menuDataCache.emplace(stem, value);

	if (value.get("extension", false).asBool())
		extensionSet.insert(stem);
	
	return true;
}

void LoadJsonMenus() 
{
	//load overrides last to override defaults and extensions
	for (IDirectoryIterator iter(EXTENSIONS_PATH, "*.json"); !iter.Done(); iter.Next())
	{
		LoadJsonMenu(iter.GetFullPath().c_str());
	}

	for (IDirectoryIterator iter(MENUDATA_PATH, "*.json"); !iter.Done(); iter.Next())
	{
		LoadJsonMenu(iter.GetFullPath().c_str());
	}

	//override data and extensions
	for (IDirectoryIterator iter(OVERRIDE_PATH, "*.json"); !iter.Done(); iter.Next())
	{
		LoadJsonMenu(iter.GetFullPath().c_str());
	}
}

void LoadMenuFiles() {
	//Add menu categories preemptively for ordering purposes
	std::unordered_set<std::string> loadedMenus;

	//Load human menu first for ordering purposes
	std::string humanPoseFile = std::string(MENUS_PATH) + "\\Human Pose.txt";
	LoadMenuFile(humanPoseFile.c_str());
	loadedMenus.insert(humanPoseFile.c_str());

	//Load Vanilla and ZeX Export files first for ordering purposes
	std::string vanillaExport = std::string(MENUS_PATH) + "\\Vanilla Export.txt";
	LoadMenuFile(vanillaExport.c_str());
	loadedMenus.insert(vanillaExport.c_str());

	std::string zexExport = std::string(MENUS_PATH) + "\\ZeX Export.txt";
	LoadMenuFile(zexExport.c_str());
	loadedMenus.insert(zexExport.c_str());

	for (IDirectoryIterator iter(MENUS_PATH, "*.txt"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		if (!loadedMenus.count(path)) {
			LoadMenuFile(path.c_str());
			loadedMenus.insert(path);
		}
	}

	for (IDirectoryIterator iter(IDLES_PATH, "*.json"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		LoadIdleFile(path.c_str());
	}

	LoadJsonMenus();
	LoadIdleFavorites();
	LoadPoseFavorites();
}

void ReloadJsonMenus()
{
	menuDataCache.clear();
	extensionSet.clear();

	LoadJsonMenus();
}

Json::Value* GetCachedMenu(const char* name)
{
	auto it = menuDataCache.find(name);
	if (it != menuDataCache.end())
		return &it->second;

	return nullptr;
}

void GetMenuGFx(GFxResult& result, const char* name) 
{
	Json::Value* menu = GetCachedMenu(name);
	if (!menu)
		return result.SetError(MENU_MISSING_ERROR);
		
	result.SetMenu(menu);
}

void GetExtensionMenusGFx(GFxResult& result) {
	result.CreateMenuItems();

	//sort the extensions
	NaturalSortedSet set;
	for (auto& extension : extensionSet) {
		set.insert(extension);
	}

	for (auto& extension : set) {
		result.PushItem(extension.c_str(), &GFxValue(extension.c_str()));
	}
}

bool isDotOrDotDot(const char* cstr) {
	if (cstr[0] != '.') return false;
	if (cstr[1] == 0) return true;
	if (cstr[1] != '.') return false;
	return (cstr[2] == 0);
}

void GetSortedFilesAndFolders(const char* path, const char* ext, NaturalSortedMap& files, NaturalSortedMap& folders)
{
	int extLen = strlen(ext);

	for (IDirectoryIterator iter(path, "*"); !iter.Done(); iter.Next())
	{
		const char* cFileName = iter.Get()->cFileName;
		if (!isDotOrDotDot(cFileName)) {

			std::string filename(iter.Get()->cFileName);
			std::string filepath = iter.GetFullPath();

			if (std::filesystem::is_directory(filepath)) {
				folders.emplace(filename, filepath);
			}
			else {
				UInt32 size = filename.size();
				if (size >= extLen) {
					if (!_stricmp(&filename.c_str()[size - extLen], ext)) {
						std::string noExtension = filename.substr(0, filename.length() - extLen);
						files.emplace(noExtension, filepath);
					}
				}
			}
		}
	}
}

void GetFolderGFx(GFxResult& result, const char* path, const char* ext) {
	result.CreateFolder();

	NaturalSortedMap files;
	NaturalSortedMap folders;
	GetSortedFilesAndFolders(path, ext, files, folders);

	for (auto& folder : folders) {
		result.PushFolder(folder.first.c_str(), folder.second.c_str());
	}

	for (auto& file : files) {
		result.PushFile(file.first.c_str(), file.second.c_str());
	}
}

void GetPathStem(GFxResult& result, const char* path)
{
	result.SetManagedString(result.root, std::filesystem::path(path).stem().string().c_str());
}

void GetPathRelative(GFxResult& result, const char* root, const char* ext, const char* path)
{
	std::string relative = GetRelativePath(strlen(root), strlen(ext), path);
	result.SetManagedString(result.root, relative.c_str());
}

void SamSerializeSave(const F4SESerializationInterface* ifc)
{
	ifc->OpenRecord('CAM', CAM_SERIALIZE_VERSION);
	SerializeCamera(ifc, CAM_SERIALIZE_VERSION);
	ifc->OpenRecord('LIGH', LIGHTS_SERIALIZE_VERSION);
	SerializeLights(ifc, LIGHTS_SERIALIZE_VERSION);
}

void SamSerializeLoad(const F4SESerializationInterface* ifc)
{
	UInt32 type, length, version;

	while (ifc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'CAM': DeserializeCamera(ifc, version); break;
		case 'LIGH': DeserializeLights(ifc, version); break;
		}
	}
}

void SamSerializeRevert(const F4SESerializationInterface* ifc)
{
	RevertCamera();
	RevertLights();
	lastSelectedPose.clear();
}