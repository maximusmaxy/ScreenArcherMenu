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

#include "SAF/util.h"
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
#include "io.h"

#include <WinUser.h>
#include <libloaderapi.h>

SelectedRefr selected;

SamManager samManager;

RelocPtr <BSReadWriteLock> uiMessageLock(0x65774B0);

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