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

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "json/json.h"

#include "SAF/util.h"
#include "SAF/io.h"
#include "SAF/hacks.h"
#include "SAF/eyes.h"

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

void SamManager::SetOpen(bool isOpen)
{
	//BSWriteLocker locker(g_menuTableLock);
	std::lock_guard<std::mutex> lock(mutex);

	menuOpened = isOpen;
}

void SamManager::OpenMenu(BSFixedString menuName)
{
	std::lock_guard<std::mutex> lock(mutex);

	if ((*g_ui)->IsMenuRegistered(menuName) && !(*g_ui)->IsMenuOpen(menuName)) {
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Open);
	}
}

void SamManager::CloseMenu(BSFixedString menuName)
{
	std::lock_guard<std::mutex> lock(mutex);

	if ((*g_ui)->IsMenuRegistered(menuName)) {
		menuOpened = false;
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
	}
}

void SamManager::TryClose(BSFixedString menuName)
{
	//BSWriteLocker locker(g_menuTableLock);
	std::lock_guard<std::mutex> lock(mutex);

	bool result = false;
	if (menuOpened) {

		IMenu* menu = (*g_ui)->GetMenu(menuName);
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
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
		}
	}
}

void SamManager::Invoke(BSFixedString menuName, const char* name, GFxValue* result, GFxValue* args, UInt32 numArgs)
{
	//BSReadLocker locker(g_menuTableLock);
	std::lock_guard<std::mutex> lock(mutex);

	if (menuOpened) {
		GFxMovieRoot* root = GetRoot(menuName);
		if (root)
			root->Invoke(name, result, args, numArgs);
	}
}

void SamManager::SetVariable(BSFixedString menuName, const char* pVarPath, const GFxValue* value, UInt32 setType)
{
	//BSReadLocker locker(g_menuTableLock);
	std::lock_guard<std::mutex> lock(mutex);

	if (menuOpened) {
		GFxMovieRoot* root = GetRoot(menuName);
		if (root)
			root->SetVariable(pVarPath, value, setType);
	}
}

Json::Value GetJsonValue(GFxValue* value)
{
	switch (value->GetType()) {
	case GFxValue::kType_Bool:
		return Json::Value(value->GetBool());
	case GFxValue::kType_Int:
		return Json::Value(value->GetInt());
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
		SavedDataArrVisitor arrVisitor(arr);
		value->VisitElements(&arrVisitor, 0, value->GetArraySize());
		return arr;
	}
	case GFxValue::kType_Object:
	{
		Json::Value obj(Json::ValueType::objectValue);
		SavedDataObjVisitor visitor(obj);
		value->VisitMembers(&visitor);
		return obj;
	}
	default:
		return Json::Value(Json::ValueType::nullValue);
	}
}

void SavedDataObjVisitor::Visit(const char* member, GFxValue* value) {
	json[member] = GetJsonValue(value);
}

void SavedDataArrVisitor::Visit(UInt32 idx, GFxValue* value) {
	json[(int)idx] = GetJsonValue(value);
}

void SamManager::SaveData(GFxValue* saveData)
{
	BSReadLocker locker(g_menuTableLock);
	//std::lock_guard<std::mutex> lock(mutex);

	if (!selected.refr) 
		return;

	//copying to a json for saved menu data instead of trying to understand how GFx managed memory works
	data.clear();
	SavedDataObjVisitor visitor(data);
	saveData->VisitMembers(&visitor);

	refr = selected.refr;
}

bool SamManager::LoadData(GFxMovieRoot* root, GFxValue* result)
{
	BSReadLocker locker(g_menuTableLock);
	//std::lock_guard<std::mutex> lock(mutex);

	//if ref updated save data is invalidated so ignore
	if (!refr || selected.refr != refr) {
		ClearData();
		return false;
	}

	root->CreateObject(result);

	for (auto it = data.begin(); it != data.end(); ++it) {
		GFxValue GFxMember;
		GetGFxValue(root, &GFxMember, *it);
		result->SetMember(it.key().asCString(), &GFxMember);
	}

	ClearData();
	return true;
}

void SamManager::ClearData()
{
	refr = nullptr;
}

void SamManager::GetGFxValue(GFxMovieRoot* root, GFxValue* result, const Json::Value& value) {
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
			GetGFxValue(root, &arrValue, member);
			result->PushBack(&arrValue);
		}

		break;
	}
	case Json::ValueType::objectValue:
	{
		root->CreateObject(result);

		for (auto it = value.begin(); it != value.end(); ++it) {
			GFxValue objValue;
			GetGFxValue(root, &objValue, *it);
			result->SetMember(it.key().asCString(), &objValue);
		}

		break;
	}
	}
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
	static BSFixedString samMenu("ScreenArcherMenu");
	static BSFixedString photoMenu("PhotoMenu");

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

	GFxValue delayClose((*g_ui)->IsMenuOpen(photoMenu));
	data.SetMember("delayClose", &delayClose);

	GetMenuTarget(data);

	GFxValue saved;
	if (samManager.LoadData(root, &saved)) {
		data.SetMember("saved", &saved);
	}

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

	SetMenusHidden(false);

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);
}

void OnConsoleUpdate() {
	static BSFixedString samMenu("ScreenArcherMenu");

	GFxMovieRoot* root = GetRoot(samMenu);
	if (!root) {
		_DMESSAGE("Could not find screen archer menu");
		return;
	}

	GFxValue data;
	root->CreateObject(&data);

	GFxValue menuState;
	root->GetVariable(&menuState, "root1.Menu_mc.state");

	//Idle states
	if (menuState.GetInt() == 14 || menuState.GetInt() == 15) {
		const char* idleName = GetCurrentIdleName();
		if (idleName) {
			GFxValue idle(idleName);
			data.SetMember("idle", &idle);
		}
	}

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
	static BSFixedString menuName("ScreenArcherMenu");

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
		BSFixedString samMenu("ScreenArcherMenu");
		BSFixedString photoMenu("PhotoMenu");
		BSFixedString consoleMenu("Console");

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
					if (evn->isOpen) {
						inputHandler.enabled = false;
					}
					else {
						OnConsoleUpdate();
						inputHandler.enabled = true;
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
	IFileStream file;

	if (!file.Open(path)) {
		_Log("Failed to open", path);
		return false;
	}

	std::string jsonString;
	SAF::ReadAll(&file, &jsonString);
	file.Close();

	Json::Reader reader;
	Json::Value value;

	if (!reader.parse(jsonString, value)) {
		_Log("Failed to parse ", path);
		return false;
	}

	try {

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
			mod = apose.get("mod", "ScreenArcherMenu.esp").asString();
			id = apose.get("idle", "802").asString();
			data.aposeId = GetFormId(mod.c_str(), id.c_str());

			raceIdleData[data.raceId] = data;
		}
	}
	catch (...) {
		_Log("Failed to read ", path);
		return false;
	}

	return true;
}

void LoadMenuFiles() {
	//Add menu categories preemptively for ordering purposes
	std::unordered_set<std::string> loadedMenus;

	//Load human menu first for ordering purposes
	const char* humanPoseFile = "Data\\F4SE\\Plugins\\SAM\\Menus\\Human Pose.txt";
	LoadMenuFile(humanPoseFile);
	loadedMenus.insert(humanPoseFile);

	//Load Vanilla and ZeX Export files first for ordering purposes
	const char* vanillaExport = "Data\\F4SE\\Plugins\\SAM\\Menus\\Vanilla Export.txt";
	LoadMenuFile(vanillaExport);
	loadedMenus.insert(vanillaExport);

	const char* zexExport = "Data\\F4SE\\Plugins\\SAM\\Menus\\ZeX Export.txt";
	LoadMenuFile(zexExport);
	loadedMenus.insert(zexExport);

	for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAM\\Menus", "*.txt"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		if (!loadedMenus.count(path)) {
			LoadMenuFile(path.c_str());
			loadedMenus.insert(path);
		}
	}

	for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAM\\Idles", "*.json"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		LoadIdleFile(path.c_str());
	}
}

bool isDotOrDotDot(const char* cstr) {
	if (cstr[0] != '.') return false;
	if (cstr[1] == 0) return true;
	if (cstr[1] != '.') return false;
	return (cstr[2] == 0);
}

void GetSubFolderGFx(GFxMovieRoot* root, GFxValue* result, const char* path, const char* ext) {
	root->CreateArray(result);

	std::map<std::string, std::string, NaturalSort> folders;
	std::map<std::string, std::string, NaturalSort> files;

	int extLen = strlen(ext);

	for (IDirectoryIterator iter(path, "*"); !iter.Done(); iter.Next())
	{
		const char* cFileName = iter.Get()->cFileName;
		if (!isDotOrDotDot(cFileName)) {

			std::string filename(iter.Get()->cFileName);
			std::string filepath = iter.GetFullPath();

			if (std::filesystem::is_directory(filepath)) {
				folders[filename] = filepath;
			}
			else {
				UInt32 size = filename.size();
				if (size >= extLen) {
					if (!_stricmp(&filename.c_str()[size - extLen], ext)) {
						std::string noExtension = filename.substr(0, filename.length() - extLen);
						files[noExtension] = filepath;
					}
				}
			}
		}
	}

	for (auto& folder : folders) {
		GFxValue value;
		root->CreateObject(&value);

		std::string folderName = folder.first;
		GFxValue name(folderName.c_str());
		value.SetMember("name", &name);

		GFxValue isFolder(true);
		value.SetMember("folder", &isFolder);

		GFxValue pathname(folder.second.c_str());
		value.SetMember("path", &pathname);

		result->PushBack(&value);
	}

	for (auto& file : files) {
		GFxValue value;
		root->CreateObject(&value);

		GFxValue name(file.first.c_str());
		value.SetMember("name", &name);

		GFxValue pathname(file.second.c_str());
		value.SetMember("path", &pathname);

		result->PushBack(&value);
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
}