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
#include "data.h"

#include <WinUser.h>
#include <libloaderapi.h>

SelectedRefr selected;

SamManager samManager;

typedef void(*_ScaleformRefCountImplAddRef)(IMenu* menu);
RelocAddr<_ScaleformRefCountImplAddRef> ScaleformRefCountImplAddRef(0x210EBF0);

typedef void(*_ScaleformRefCountImplRelease)(IMenu* menu);
RelocAddr<_ScaleformRefCountImplRelease> ScaleformRefCountImplRelease(0x210EC90);

IMenuWrapper::IMenuWrapper(IMenu* _menu) {
	menu = _menu;
	if (menu)
		ScaleformRefCountImplAddRef(menu);
}

IMenuWrapper::~IMenuWrapper() {
	if (menu)
		ScaleformRefCountImplRelease(menu);
}

GFxMovieRoot* IMenuWrapper::GetRoot() {
	if (!menu)
		return nullptr;

	if (!((byte)menu->flags & 0x40))
		return nullptr;

	GFxMovieView* view = menu->movie;
	if (!view)
		return nullptr;

	return view->movieRoot;
}

bool IMenuWrapper::IsOpen() {
	if (!menu)
		return false;

	return ((byte)menu->flags & 0x40);
}

IMenuWrapper GetWrappedMenu(BSFixedString name) {
	BSReadLocker lock(g_menuTableLock);

	auto tableItem = (*g_ui)->menuTable.Find(&name);
	if (!tableItem)
		return IMenuWrapper();

	if (!tableItem->menuInstance)
		return IMenuWrapper();

	return IMenuWrapper(tableItem->menuInstance);
}

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible)
{
	auto wrapped = GetWrappedMenu(menuName);
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	if (!root->SetVariable(visiblePath, &GFxValue(visible)))
		_Log("Failed to set visibility of menu: ", menuName.c_str());
}

//RelocPtr <BSReadWriteLock> g_menuStackLock(0x65774B0);
//
//void RemoveDuplicateIMenusFromStack(IMenu* menu)
//{
//	BSWriteLocker locker(g_menuStackLock);
//
//	auto& stack = (*g_ui)->menuStack;
//
//	int removeIndex;
//	int foundIndex;
//
//	//loop until every duplicate is removed
//	do {
//		removeIndex = -1;
//		foundIndex = -1;
//
//		//find index of duplicate
//		for (int i = 0; i < stack.count; ++i) {
//			if (stack.entries[i] == menu) {
//				if (foundIndex != -1) {
//					removeIndex = i;
//					break;
//				}
//				foundIndex = i;
//			}
//		}
//
//		//remove duplicate if found
//		if (removeIndex != -1) {
//			_DMESSAGE("Duplicate menu on stack found");
//			stack.Remove(removeIndex);
//		}
//
//	} while (removeIndex != -1);
//}

IMenuWrapper SamManager::StoreMenu() {
	std::lock_guard lock(mutex);
	
	BSReadLocker tableLock(g_menuTableLock);

	auto tableItem = (*g_ui)->menuTable.Find(&BSFixedString(SAM_MENU_NAME));
	if (!tableItem)
		return IMenuWrapper();

	auto menu = tableItem->menuInstance;
	if (!menu)
		return IMenuWrapper();

	ScaleformRefCountImplAddRef(menu);
	storedMenu = menu;

	return IMenuWrapper(storedMenu);
}

bool SamManager::ReleaseMenu() {
	std::lock_guard lock(mutex);

	if (!storedMenu)
		return false;

	ScaleformRefCountImplRelease(storedMenu);
	storedMenu = nullptr;

	return true;
}

IMenuWrapper SamManager::GetWrapped() {
	std::lock_guard lock(mutex);

	return IMenuWrapper(storedMenu);
}

void SamManager::ToggleMenu() {
	auto wrapped = GetWrapped();

	BSFixedString menuStr = BSFixedString(SAM_MENU_NAME);

	if (wrapped.IsOpen()) {
		auto root = wrapped.GetRoot();
		if (root) {

			bool result = false;
			GFxValue gfxResult;
			if (root->Invoke("root1.Menu_mc.TryClose", &gfxResult, nullptr, 0))
				result = gfxResult.GetBool();

			if (result) {
				//RemoveDuplicateIMenusFromStack(wrapped.menu);
				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Close);
			}
		}
		
	}
	else {
		if ((*g_ui)->IsMenuRegistered(menuStr)) {
			if (!(*g_ui)->IsMenuOpen(menuStr)) {
				storedName = MAIN_MENU_NAME;
				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Open);
			}
		}
		else {
			ShowHudMessage(F4SE_NOT_INSTALLED);
		}
	}
}

void SamManager::OpenExtensionMenu(const char* extensionName)
{
	auto wrapped = GetWrapped();

	BSFixedString menuStr = BSFixedString(SAM_MENU_NAME);

	if (wrapped.IsOpen()) {
		if (!_stricmp(storedName.c_str(), extensionName)) {
			auto root = wrapped.GetRoot();
			if (root) {

				bool result = false;

				GFxValue gfxResult;
				if (root->Invoke("root1.Menu_mc.TryClose", &gfxResult, nullptr, 0))
					result = gfxResult.GetBool();

				if (result) {
					//RemoveDuplicateIMenusFromStack(wrapped.menu);
					CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Close);
				}
			}
		}
	}
	else {
		if ((*g_ui)->IsMenuRegistered(menuStr)) {
			if (!(*g_ui)->IsMenuOpen(menuStr)) {
				storedName = extensionName;
				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Open);
			}
		}
		else {
			ShowHudMessage(F4SE_NOT_INSTALLED);
		}
	}
}

void SamManager::CloseMenu()
{
	auto wrapped = GetWrapped();
	if (wrapped.IsOpen()) {
		//RemoveDuplicateIMenusFromStack(wrapped.menu);
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(BSFixedString(SAM_MENU_NAME), kMessage_Close);
	}
}

bool SamManager::Invoke(const char* func, GFxValue* result, GFxValue* args, UInt32 numArgs)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return false;

	if (!root->Invoke(func, result, args, numArgs)) {
		_Log("Failed to invoke UI function: ", func);
		return false;
	}

	return true;
}

void SamManager::SetVariable(const char* pVarPath, const GFxValue* value, UInt32 setType)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	if (!root->SetVariable(pVarPath, value, setType))
		_Log("Failed to set menu variable", pVarPath);
}

void SamManager::SaveData(GFxValue* saveData)
{
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
	//if ref updated save data is invalidated so ignore
	if (!refr || selected.refr != refr) {
		ClearData();
		return false;
	}

	//Check the saved menu name matches the menu being opened
	if (_stricmp(data.get("rootMenu", "").asCString(), storedName.c_str())) {
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

void SamManager::ForceQuit()
{
	ClearData();

	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	root->Invoke("root1.Menu_mc.CleanUp", nullptr, nullptr, 0);
	CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(BSFixedString(SAM_MENU_NAME), kMessage_Close);
}

void SamManager::PushMenu(const char* name)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue result;
	GFxValue args(name);
	root->Invoke("root1.Menu_mc.PushMenu", &result, &args, 1);
}

void SamManager::PopMenu()
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.PopMenu", &ret, nullptr, 0);
}

void SamManager::PopMenuTo(const char* name)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.PopMenuTo", &ret, &GFxValue(name), 1);
}

void SamManager::RefreshMenu() 
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.RefreshValues", nullptr, nullptr, 0);
}

void SamManager::UpdateMenu() 
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.ReloadMenu", nullptr, nullptr, 0);
}

void SamManager::ShowNotification(const char* msg, bool store)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args[2];
	args[0] = GFxValue(msg);
	args[1] = GFxValue(true);

	root->Invoke("root1.Menu_mc.ShowNotification", nullptr, args, 2);
}

void SamManager::SetNotification(const char* msg)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.SetNotification(root, msg);
	result.InvokeCallback();
}

void SamManager::SetTitle(const char* title)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.SetTitle(root, title);
	result.InvokeCallback();
}

void SamManager::SetMenuNames(VMArray<BSFixedString>& vmNames)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.CreateNames();

	for (int i = 0; i < vmNames.Length(); i++) {
		BSFixedString name;
		vmNames.Get(&name, i);
		result.PushName(name.c_str());
	}

	result.InvokeCallback();
}

void SamManager::SetMenuValues(VMArray<VMVariable>& vmValues)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.CreateValues();

	for (int i = 0; i < vmValues.Length(); i++) {
		VMVariable var;
		vmValues.Get(&var, i);

		GFxValue value;
		VMVariableToGFx(root, &value, &var);

		result.PushValue(&value);
	}

	result.InvokeCallback();
}

void SamManager::SetMenuItems(VMArray<BSFixedString>& vmNames, VMArray<VMVariable>& vmValues)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);

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
		VMVariableToGFx(root, &value, &var);

		result.PushValue(&value);
	}

	result.InvokeCallback();
}

void SamManager::SetSuccess()
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(root, &value); 
	result.InvokeCallback();
}

void SamManager::SetError(const char* error)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(root, &value);
	result.SetError(error);
	result.InvokeCallback();
}

void SamManager::SetLocal(const char* key, GFxValue* value)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args[2];
	args[0] = GFxValue(key);
	args[1] = *value;
	root->Invoke("root1.Menu_mc.SetLocalVariable", nullptr, args, 2);
}

void SamManager::SetVisible(bool isVisible)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;
	
	root->SetVariable("root1.Menu_mc.visible", &GFxValue(isVisible));
}

void MenuAlwaysOn(BSFixedString menuStr, bool enabled) {
	auto menu = GetWrappedMenu(menuStr);
	if (menu.IsOpen()) {
		if (enabled)
			menu.menu->flags |= 0x02;
		else
			menu.menu->flags &= ~0x02;
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

void GetCursorPosition(GFxResult& result) {
	result.CreateValues();

	SInt32 pos[2];
	if (!GetCursor(pos))
		return result.SetError("Failed to get cursor position");

	result.PushValue(pos[0]);
	result.PushValue(pos[1]);
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

	if (selectedNonActor.refr) 
	{
		const char* name;
		if (selectedNonActor.refr->baseForm) {
			name = selectedNonActor.refr->baseForm->GetFullName();
		}
		else {
			name = selectedNonActor.refr->GetFullName();
		}
		if (name && *name) {
			data.SetMember("title", &GFxValue(name));
		}
		else {
			data.SetMember("title", &GFxValue(" "));
		}
	}
	else {
		data.SetMember("title", &GFxValue("No Target"));
	}
}

bool SamManager::OnMenuOpen() {
	auto wrapped = StoreMenu();
	auto root = wrapped.GetRoot();
	if (!root) {
		_DMESSAGE("Failed to get root on menu open");
		return false;
	}

	LockFfc(true);

	TESObjectREFR* refr = GetRefr();
	selected.Update(refr);
	UpdateNonActorRefr();

	GFxValue data;
	root->CreateObject(&data);

	data.SetMember("menuName", &GFxValue(storedName.c_str()));

	GetMenuTarget(data);

	GFxValue saved;
	if (LoadData(root, &saved))
		data.SetMember("saved", &saved);

	GFxValue widescreen(GetMenuOption(kOptionWidescreen));
	data.SetMember("widescreen", &widescreen);

	GFxValue alignment(GetMenuOption(kOptionAlignment));
	data.SetMember("swap", &alignment);

	root->Invoke("root1.Menu_mc.MenuOpened", nullptr, &data, 1);
	root->SetVariable("root1.Menu_mc.visible", &GFxValue(true));

	return true;
}

bool SamManager::OnMenuClose() {
	LockFfc(false);
	selected.Clear();
	return ReleaseMenu();
}

bool SamManager::OnConsoleUpdate() {
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root) {
		_DMESSAGE("Failed to get menu root on console update");
		return false;
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

	root->Invoke("root1.Menu_mc.ConsoleRefUpdated", nullptr, &data, 1);

	return true;
}

void StartSamQuest()
{
	//If sam still isn't registered, force register it
	static BSFixedString forceRegister("ForceRegister");
	CallSamGlobal(forceRegister);
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
					(*g_menuControls)->inputEvents.Push(handlerPtr);
				}
				if (samManager.OnMenuOpen())
					SetMenuVisible(photoMenu, "root1.Menu_mc.visible", false);
			}
			else {
				inputHandler.enabled = false;
				if (samManager.OnMenuClose())
					SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);
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
						samManager.OnConsoleUpdate();
						inputHandler.enabled = true;
					}
				}
				else if (evn->menuName == cursorMenu) {
					//cursor opened while sam is open
					if (evn->isOpen) {
						MenuAlwaysOn(cursorMenu, true);
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
						MenuAlwaysOn(cursorMenu, false);
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