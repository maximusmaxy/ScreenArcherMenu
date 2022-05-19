#include "sam.h"

#include "f4se/GameMenus.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/GameRTTI.h"

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

#include <regex>
#include <WinUser.h>

SelectedRefr selected;

MenuOptions menuOptions;

MenuCache poseMenuCache;
MenuCache morphsMenuCache;
MenuCache groupsMenuCache;

SavedMenuData saveData;

bool menuOpened = false;

bool NaturalSort::operator() (const std::string& a, const std::string& b) const 
{
	return strnatcasecmp(a.c_str(), b.c_str()) < 0;
}	

MenuCategoryList* GetMenu(MenuCache* cache)
{
	if (!selected.refr) return nullptr;

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
	if ((*g_ui)->IsMenuOpen(menuName)) {
		GFxMovieRoot * root = (*g_ui)->GetMenu(menuName)->movie->movieRoot;
		root->SetVariable(visiblePath, &GFxValue(visible));
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
	isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);
	race = npc->race.race->formID;
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

	menuOpened = true;

	IMenu* menu = (*g_ui)->GetMenu(samMenu);
	if (!menu) {
		_DMESSAGE("Could not find screen archer menu");
		return;
	}

	TESObjectREFR* refr = GetRefr();
	selected.Update(refr);
	UpdateNonActorRefr();

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", false);

	GFxMovieRoot * root = menu->movie->movieRoot;
	GFxValue data;
	root->CreateObject(&data);

	GFxValue delayClose((*g_ui)->IsMenuOpen(photoMenu));
	data.SetMember("delayClose", &delayClose);

	GetMenuTarget(data);

	GFxValue saved;
	if (saveData.Load(root, &saved)) {
		data.SetMember("saved", &saved);
	}

	root->Invoke("root1.Menu_mc.menuOpened", nullptr, &data, 1);
}

void OnMenuClose() {
	//_DMESSAGE("Menu closed");

	static BSFixedString photoMenu("PhotoMenu");

	if (!menuOpened) {
		_DMESSAGE("Tried to close an unopened menu");
		return;
	}

	selected.Clear();

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);

	menuOpened = false;
}

void OnConsoleRefUpdate() {
	static BSFixedString samMenu("ScreenArcherMenu");

	IMenu* menu = (*g_ui)->GetMenu(samMenu);
	if (!menu) {
		_DMESSAGE("Could not find screen archer menu");
		return;
	}

	TESObjectREFR * refr = GetRefr();
	UpdateNonActorRefr();

	if (selected.refr != refr) {
		selected.Update(refr);

		GFxMovieRoot * root = menu->movie->movieRoot;
		GFxValue data;
		root->CreateObject(&data);

		GFxValue menuState;
		root->GetVariable(&menuState, "root1.Menu_mc.state");

		GetMenuTarget(data);

		root->Invoke("root1.Menu_mc.consoleRefUpdated", nullptr, &data, 1);
	}
}

void ToggleMenu() {
	static BSFixedString menuName("ScreenArcherMenu");

	if ((*g_ui)->IsMenuRegistered(menuName)) {
		UInt32 message = (*g_ui)->IsMenuOpen(menuName) ? kMessage_Close : kMessage_Open;
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, message);
	}
}

bool OpenSamFile(std::string filename) {
	if (!selected.refr) return false;

	if (LoadJsonPose(filename.c_str())) return true;
	if (LoadMfg(filename)) return true;
	if (LoadAdjustmentFile(filename.c_str())) return true;

	return false;
}

void GetOptionsGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	GFxValue hotswap(menuOptions.hotSwapping);
	result->PushBack(&hotswap);
}

class SavedDataVisitor : public GFxValue::ObjectInterface::ObjVisitor
{
public:
	Json::Value json;

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
		default:
			return Json::Value(Json::ValueType::nullValue);
		}
	}
};

void SavedMenuData::Save(GFxValue* saveData) {
	if (!selected.refr) return;

	//copying to a json for saved menu data instead of trying to understand how GFx managed memory works
	SavedDataVisitor visitor;
	saveData->VisitMembers(&visitor);
	data = visitor.json;
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
	}
}

void SavedMenuData::Clear() {
	refr = nullptr;
}

enum {
	kMenuHeaderRace = 0,
	kMenuHeaderMod,
	kMenuHeaderSex,
	kMenuHeaderType
};

enum {
	kMenuTypePose = 1,
	kMenuTypeMorphs,
	kMenuTypeGroups
};

std::regex tabSeperatedRegex("([^\\t]+)\\t+([^\\t]+)");		//matches (1)\t(2)
std::regex tabOptionalRegex("([^\\t]+)(?:\\t+([^\\t]+))?");	//matches (1) or (1)\t(2)

std::unordered_map<std::string, UInt32> menuHeaderMap = {
	{"race", kMenuHeaderRace},
	{"mod", kMenuHeaderMod},
	{"sex", kMenuHeaderSex},
	{"type", kMenuHeaderType},
};

std::unordered_map<std::string, UInt32> menuTypeMap = {
	{"pose", kMenuTypePose},
	{"morphs", kMenuTypeMorphs},
	{"groups", kMenuTypeGroups},
};

bool ParseMenuFile(std::string path, IFileStream& file) {

	char buf[512];
	std::cmatch match;
	std::string categoryIdentifier = "Category";
	std::string header[4];

	//header
	for (int i = 0; i < 4; ++i) {
		file.ReadString(buf, 512, '\n', '\r');
		bool matched = std::regex_search(buf, match, tabSeperatedRegex);
		std::string lower = toLower(match[1].str());
		if (matched && match[2].str().size() && menuHeaderMap.count(lower)) {
			header[menuHeaderMap[lower]] = match[2].str();
		}
		else {
			_LogCat("Failed to read header ", path);
			return false;
		}
	}

	UInt64 key = GetFormId(header[kMenuHeaderMod], header[kMenuHeaderRace]);
	if (header[kMenuHeaderSex] == "female" || header[kMenuHeaderSex] == "Female")
		key |= 0x100000000;

	MenuCache* cache;
	UInt32 menuType = menuTypeMap[header[kMenuHeaderType]];
	if (!menuType) {
		_LogCat("Unknown menu type: ", header[kMenuHeaderType]);
		return false;
	}

	switch (menuType) {
		case kMenuTypePose: cache = &poseMenuCache; break;
		case kMenuTypeMorphs: cache = &morphsMenuCache; break;
		case kMenuTypeGroups: cache = &groupsMenuCache; break;
	}

	UInt32 categoryIndex = 0;

	try {
		while (!file.HitEOF()) {
			file.ReadString(buf, 512, '\n', '\r');
			if (std::regex_search(buf, match, tabOptionalRegex)) {
				if (match[1].str() == categoryIdentifier) {
					//Try find existing category
					UInt32 size = (*cache)[key].size();
					for (categoryIndex = 0; categoryIndex < size; categoryIndex++) {
						if ((*cache)[key][categoryIndex].first == match[2].str()) {
							break;
						}
					}
					//if category not found add it
					if (categoryIndex >= size) {
						MenuList list;
						(*cache)[key].push_back(std::make_pair(match[2].str(), list));
						categoryIndex = (*cache)[key].size() - 1;
					}
				}
				else {
					//add to existing category
					(*cache)[key][categoryIndex].second.push_back(std::make_pair(match[2].str(), match[1].str()));
				}
			}
		}
	}
	catch (...) {
		_LogCat("Failed to read ", path);
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
		_LogCat(path, " not found");
		return false;
	}

	std::string jsonString;
	SAF::ReadAll(&file, &jsonString);
	file.Close();

	Json::Reader reader;
	Json::Value value;

	if (!reader.parse(jsonString, value)) {
		_LogCat("Failed to parse ", path);
		return false;
	}

	try {
		Json::Value::Members members = value.getMemberNames();

		for (auto& memberStr : members) {
			Json::Value member = value[memberStr];
			IdleData data;

			data.raceId = GetFormId(member["mod"].asString(), member["race"].asString());
			data.resetId = GetFormId(member["reset"]["mod"].asString(), member["reset"]["idle"].asString());
			data.behavior = BSFixedString(member["filter"]["behavior"].asCString());
			data.event = BSFixedString(member["filter"]["event"].asCString());
			
			raceIdleData[data.raceId] = data;
		}
	}
	catch (...) {
		_LogCat("Failed to read ", path);
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