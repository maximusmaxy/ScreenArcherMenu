#include "sam.h"

#include "f4se/GameMenus.h"
#include "f4se/GameData.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/GameRTTI.h"
#include "f4se/PluginManager.h"
#include "f4se/CustomMenu.h"
#include "f4se/Serialization.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "json/json.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"
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

#include <WinUser.h>
#include <libloaderapi.h>

SelectedRefr selected;

MenuCache poseMenuCache;
MenuCache morphsMenuCache;
MenuCache groupsMenuCache;
MenuCategoryList lightsMenuCache;
MenuCache exportMenuCache;

SavedMenuData saveData;

bool menuOpened = false;

bool NaturalSort::operator() (const std::string& a, const std::string& b) const 
{
	return strnatcasecmp(a.c_str(), b.c_str()) < 0;
}	

//SamMenu::SamMenu() : GameMenuBase() {
//	static BSFixedString menuName("ScreenArcherMenu");
//	static BSFixedString menuRoot("root1.Menu_mc");
//
//	UInt32 flags = 0x8018494;
//	UInt32 movieFlags = 3;
//	UInt32 extFlags = 3;
//	UInt32 depth = 6;
//
//	if ((flags & IMenu::kFlag_UsesCursor) && (extFlags & CustomMenuData::kExtFlag_CheckForGamepad))
//	{
//		if ((*g_inputDeviceMgr)->IsGamepadEnabled())
//			flags &= ~IMenu::kFlag_UsesCursor;
//	}
//
//	if (CALL_MEMBER_FN((*g_scaleformManager), LoadMovie)(this, movie, menuName.c_str(), menuRoot.c_str(), movieFlags))
//	{
//		stage.SetMember("menuFlags", &GFxValue(flags));
//		stage.SetMember("movieFlags", &GFxValue(movieFlags));
//		stage.SetMember("extendedFlags", &GFxValue(extFlags));
//
//		CreateBaseShaderTarget(filterHolder, stage);
//
//		if (extFlags & CustomMenuData::kExtFlag_InheritColors)
//		{
//			filterHolder->SetFilterColor(false);
//			(*g_colorUpdateDispatcher)->eventDispatcher.AddEventSink(filterHolder);
//		}
//
//		if (flags & IMenu::kFlag_CustomRendering)
//		{
//			shaderFXObjects.Push(filterHolder);
//		}
//	}
//}
//
//void SamMenu::RegisterFunctions()
//{
//	RegisterNativeFunction("PlaySound", 0);
//	RegisterNativeFunction("OpenMenu", 1);
//	RegisterNativeFunction("CloseMenu", 2);
//}
//
//void SamMenu::Invoke(Args* args)
//{
//	switch (args->optionID)
//	{
//	case 0:
//	{
//		if (args->numArgs >= 1)
//		{
//			if (args->args[0].IsString())
//				PlayUISound(args->args[0].GetString());
//		}
//	}
//	break;
//	case 1:
//	{
//		if (args->numArgs >= 1)
//		{
//			if (args->args[0].IsString())
//			{
//				BSFixedString menuName(args->args[0].GetString());
//				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Open);
//			}
//		}
//	}
//	break;
//	case 2:
//	{
//		if (args->numArgs >= 1)
//		{
//			if (args->args[0].IsString())
//			{
//				BSFixedString menuName(args->args[0].GetString());
//				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
//			}
//		}
//	}
//	break;
//	default:
//		break;
//	}
//}
//
//IMenu* CreateSamMenu() {
//	return new SamMenu();
//}
//
//void RegisterSam()
//{
//	static BSFixedString menuName("ScreenArcherMenu");
//
//	if (!(*g_ui)->IsMenuRegistered(menuName))
//	{
//		(*g_ui)->Register(menuName.c_str(), CreateSamMenu);
//	}
//}

void StartSamQuest()
{
	//UInt32 id = GetFormId("ScreenArcherMenu.esp", 0x800); //ScreenArcherMenu Quest Form
	//if (id) {
	//	TESForm* form = LookupFormByID(id);
	//	TESQuest* quest = DYNAMIC_CAST(form, TESForm, TESQuest);

	//	//if the quest is completed or not running, restart it
	//	if (quest && (quest->IsCompleted() || !(quest->flags & 0x1))) {

	//		//Clear the run once flag on the quest
	//		UInt64 runOnce = quest->unk0F8[(0x2B0 - 0xF8) >> 3];
	//		UInt64 cleared = (runOnce & (~(0x0001000000000000)));
	//		quest->unk0F8[(0x2B0 - 0xF8) >> 3] = cleared;

	//		static BSFixedString startQuest("StartQuest");
	//		CallSamGlobal(startQuest);

	//		//Reenabled the run once flag
	//		quest->unk0F8[(0x2B0 - 0xF8) >> 3] = runOnce;
	//	}
	//}

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

void OpenMenu(const char* name)
{
	BSFixedString menuName(name);
	if ((*g_ui)->IsMenuRegistered(menuName) && !(*g_ui)->IsMenuOpen(menuName)) {
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Open);
	}
}

void CloseMenu(const char* name)
{
	BSFixedString menuName(name);
	if ((*g_ui)->IsMenuRegistered(menuName) && (*g_ui)->IsMenuOpen(menuName)) {
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
	}
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
	eyeNode = GetEyeNode(refr);
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
	eyeNode = nullptr;
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
	//_DMESSAGE("Menu opened");

	static BSFixedString samMenu("ScreenArcherMenu");
	static BSFixedString photoMenu("PhotoMenu");

	if (menuOpened) {
		_DMESSAGE("Tried to open an already open menu");
		return;
	}

	GFxMovieRoot* root = GetRoot(samMenu);
	if (!root) {
		_DMESSAGE("Could not find screen archer menu");
		return;
	}

	menuOpened = true;

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
	if (saveData.Load(root, &saved)) {
		data.SetMember("saved", &saved);
	}

	GFxValue widescreen(GetMenuOption(kOptionWidescreen));
	data.SetMember("widescreen", &widescreen);

	GFxValue alignment(GetMenuOption(kOptionAlignment));
	data.SetMember("swap", &alignment);

	root->Invoke("root1.Menu_mc.menuOpened", nullptr, &data, 1);
}

void OnMenuClose() {
	//_DMESSAGE("Menu closed");

	static BSFixedString photoMenu("PhotoMenu");

	if (!menuOpened) {
		_DMESSAGE("Tried to close an unopened menu");
		return;
	}

	//Restore ffc lock
	LockFfc(false);

	//Update options file
	SaveOptionsFile();

	selected.Clear();

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);

	menuOpened = false;
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

	root->Invoke("root1.Menu_mc.consoleRefUpdated", nullptr, &data, 1);
}

void ToggleMenu() {
	static BSFixedString menuName("ScreenArcherMenu");

	if ((*g_ui)->IsMenuRegistered(menuName)) {
		if ((*g_ui)->IsMenuOpen(menuName)) {
			GFxMovieRoot* root = GetRoot(menuName);
			if (root) {
				root->Invoke("root1.Menu_mc.tryClose", nullptr, nullptr, 0);
			}
		}
		else {
			CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Open);
		}
	}
}

bool OpenSamFile(std::string filename) {
	if (!selected.refr) return false;

	if (LoadJsonPose(filename.c_str())) return true;
	if (LoadMfg(filename)) return true;
	if (LoadAdjustmentFile(filename.c_str())) return true;

	return false;
}

class SavedDataVisitor : public GFxValue::ObjectInterface::ObjVisitor
{
public:
	SavedDataVisitor(Json::Value& json) : json(json) {};

	Json::Value& json;

	void Visit(const char* member, GFxValue* value) {
		json[member] = GetJsonValue(value);
	}

private:
	Json::Value GetJsonValue(GFxValue* value) {
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
			UInt32 size = value->GetArraySize();
			for (int i = 0; i < size; ++i) {
				GFxValue arrValue;
				value->GetElement(i, &arrValue);
				arr[i] = GetJsonValue(&arrValue);
			}
			return arr;
		}
		case GFxValue::kType_Object:
		{
			Json::Value obj;
			SavedDataVisitor visitor(obj);
			value->VisitMembers(&visitor);
			return obj;
		}
		default:
			return Json::Value(Json::ValueType::nullValue);
		}
	}
};

void SavedMenuData::Save(GFxValue* saveData) {
	if (!selected.refr) return;

	//copying to a json for saved menu data instead of trying to understand how GFx managed memory works
	data.clear();
	SavedDataVisitor visitor(data);
	saveData->VisitMembers(&visitor);

	refr = selected.refr;
}

bool SavedMenuData::Load(GFxMovieRoot* root, GFxValue* result) {
	//if ref updated save data is invalidated so ignore
	if (!refr || selected.refr != refr) {
		Clear();
		return false;
	}

	root->CreateObject(result);
	
	Json::Value::Members memberNames = data.getMemberNames();

	for (auto& name : memberNames) {
		GFxValue GFxMember;
		GetGFxValue(root, &GFxMember, data[name]);
		result->SetMember(name.c_str(), &GFxMember);
	}

	Clear();
	return true;
}

void SavedMenuData::GetGFxValue(GFxMovieRoot* root, GFxValue* result, const Json::Value& value) {
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

		Json::Value::Members memberNames = value.getMemberNames();
		for (auto& member : memberNames) {
			GFxValue  objValue;
			GetGFxValue(root, &objValue, value[member]);
			result->SetMember(member.c_str(), &objValue);
		}

		break;
	}
	}
}

void SavedMenuData::Clear() {
	refr = nullptr;
}

enum {
	kMenuTypePose = 1,
	kMenuTypeMorphs,
	kMenuTypeGroups,
	kMenuTypeLights,
	kMenuTypeExport
};

std::unordered_map<std::string, UInt32> menuTypeMap = {
	{"pose", kMenuTypePose},
	{"morphs", kMenuTypeMorphs},
	{"groups", kMenuTypeGroups},
	{"lights", kMenuTypeLights},
	{"export", kMenuTypeExport}
};

bool ParseMenuFile(std::string path, IFileStream& file) {

	char buf[512];
	std::cmatch match;
	std::string categoryIdentifier = "Category";
	MenuCategoryList* menu = nullptr;
	MenuHeader header;
	UInt32 categoryIndex = 0;
	const ModInfo* mod; 

	try {
		while (!file.HitEOF()) {
			file.ReadString(buf, 512, '\n', '\r');
			if (std::regex_search(buf, match, tabOptionalRegex)) {
				if (match[1].str() == categoryIdentifier) {

					//After reaching the first category, validate the header and either break or continue
					if (!menu) 
					{
						if (!header.type) 
						{
							_DMESSAGE("Failed to read header type", path);
							return false;
						}

						UInt32 key = 0;

						if (header.type == kMenuTypeLights)
						{
							mod = (*g_dataHandler)->LookupModByName(header.mod.c_str());
							if (!mod || mod->modIndex == 0xFF) 
							{
								_DMESSAGE("Failed to read header mod", path);
								return false;
							}

							UInt32 modId = (mod->recordFlags & (1 << 9)) ? ((0xFE << 24) | (mod->lightIndex << 12)) : (mod->modIndex << 24);
							lightModMap.emplace(modId, mod->name);
						}
						else
						{
							key = GetFormId(header.mod, header.race);
							if (!key)
							{
								_DMESSAGE("Failed to read header race or mod", path);
								return false;
							}
							if (header.isFemale)
								key |= 0x100000000;
						}

						switch (header.type) {
							case kMenuTypePose: menu = &poseMenuCache[key]; break;
							case kMenuTypeMorphs: menu = &morphsMenuCache[key]; break;
							case kMenuTypeGroups: menu = &groupsMenuCache[key]; break;
							case kMenuTypeLights: menu = &lightsMenuCache; break;
							case kMenuTypeExport: menu = &exportMenuCache[key]; break;
						}
					}

					//Try find existing category
					UInt32 size = menu->size();
					for (categoryIndex = 0; categoryIndex < size; categoryIndex++) {
						if ((*menu)[categoryIndex].first == match[2].str()) {
							break;
						}
					}

					//if category not found add it
					if (categoryIndex >= size) {
						MenuList list;
						menu->push_back(std::make_pair(match[2].str(), list));
						categoryIndex = menu->size() - 1;
					}
				}

				//Continue reading header
				else if (!menu)
				{
					std::string lowerHeader = toLower(match[1].str());
					if (match[2].str().size() && menuHeaderMap.count(lowerHeader))
					{
						switch (menuHeaderMap[lowerHeader])
						{
						case kMenuHeaderRace:
							header.race = match[2].str();
							break;
						case kMenuHeaderMod:
							header.mod = match[2].str();
							break;
						case kMenuHeaderSex:
							header.isFemale = !_stricmp(match[2].str().c_str(), "female");
							break;
						case kMenuHeaderType:
							std::string lowerType = toLower(match[2].str());
							if (menuTypeMap.count(lowerType)) {
								header.type = menuTypeMap[lowerType];
							}
							break;
						}
					}
				}
				//Add to category
				else
				{
					if (header.type == kMenuTypeLights) {
						//merge the mod id in now
						UInt32 formId = std::stoul(match[1].str(), nullptr, 16) & 0xFFFFFF;

						UInt32 flags = mod->recordFlags;
						if (flags & (1 << 9)) {
							formId &= 0xFFF;
							formId |= 0xFE << 24;
							formId |= mod->lightIndex << 12;
						}
						else {
							formId |= mod->modIndex << 24;
						}

						(*menu)[categoryIndex].second.push_back(std::make_pair(HexToString(formId), match[2].str()));
					}
					else {
						(*menu)[categoryIndex].second.push_back(std::make_pair(match[2].str(), match[1].str()));
					}
				}
			}
		}
	}
	catch (...) {
		_DMESSAGE("Failed to read ", path);
		return false;
	}

	return true;
}

bool LoadMenuFile(std::string path) {
	IFileStream file;

	if (!file.Open(path.c_str())) {
		_DMESSAGE("File not found");
		return false;
	}

	bool result = ParseMenuFile(path, file);

	file.Close();

	return result;
}

bool LoadIdleFile(std::string path) {
	IFileStream file;

	if (!file.Open(path.c_str())) {
		_DMESSAGE(path.c_str(), " not found");
		return false;
	}

	std::string jsonString;
	SAF::ReadAll(&file, &jsonString);
	file.Close();

	Json::Reader reader;
	Json::Value value;

	if (!reader.parse(jsonString, value)) {
		_DMESSAGE("Failed to parse ", path);
		return false;
	}

	try {
		Json::Value::Members members = value.getMemberNames();

		for (auto& memberStr : members) {
			Json::Value member = value[memberStr];
			IdleData data;

			data.raceId = GetFormId(member.get("mod", "").asString(), member.get("race", "").asString());

			Json::Value reset = member.get("reset", Json::Value());
			data.resetId = GetFormId(reset.get("mod", "").asString(), reset.get("idle", "").asString());

			Json::Value filter = member.get("filter", Json::Value());
			data.behavior = BSFixedString(filter.get("behavior", "").asCString());
			data.event = BSFixedString(filter.get("event", "").asCString());

			//default to human a pose
			Json::Value apose = member.get("apose", Json::Value());
			data.aposeId = GetFormId(apose.get("mod", "ScreenArcherMenu.esp").asString(), apose.get("idle", "802").asString());

			raceIdleData[data.raceId] = data;
		}
	}
	catch (...) {
		_DMESSAGE("Failed to read ", path);
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
			LoadMenuFile(path);
			loadedMenus.insert(path);
		}
	}

	for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAM\\Idles", "*.json"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		LoadIdleFile(path);
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