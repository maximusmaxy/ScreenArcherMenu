#include "io.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "f4se/GameData.h"

#include "lights.h"
#include "idle.h"
#include "constants.h"
#include "data.h"
#include "pose.h"
#include "options.h"

#include <filesystem>

MenuCache poseMenuCache;
MenuCache morphsMenuCache;
MenuCache groupsMenuCache;
MenuCache exportMenuCache;
MenuCache filterMenuCache;

MenuCategoryList lightsMenuCache;
MenuCategoryList tongueMenuCache;
JsonCache menuDataCache;
SAF::InsensitiveStringSet extensionSet;

MenuCategoryList* GetMenu(SelectedRefr* refr, MenuCache* cache)
{
	if (!refr)
		return nullptr;

	if (cache->count(refr->key))
		return &(*cache)[refr->key];
	if (selected.isFemale && cache->count(refr->race))
		return &(*cache)[refr->race];

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

enum {
	kMenuTypePose = 1,
	kMenuTypeMorphs,
	kMenuTypeGroups,
	kMenuTypeLights,
	kMenuTypeExport,
	kMenuTypeTongue,
	kMenuTypeFilter
};

SAF::InsensitiveUInt32Map samTypeMap = {
	{"pose", kMenuTypePose},
	{"morphs", kMenuTypeMorphs},
	{"groups", kMenuTypeGroups},
	{"lights", kMenuTypeLights},
	{"export", kMenuTypeExport},
	{"tongue", kMenuTypeTongue},
	{"filter", kMenuTypeFilter}
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
			_Log("Failed to read header type: ", path.c_str());
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
				_Log("Failed to light mod: ", path.c_str());
				return false;
			}

			UInt32 modId = (mod->recordFlags & (1 << 9)) ? ((0xFE << 24) | (mod->lightIndex << 12)) : (mod->modIndex << 24);
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
				_Log("Failed to read header for race or mod: ", path.c_str());
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
		case kMenuTypeFilter: menu = &filterMenuCache[key]; break;
		}

		return true;
	}

	void ReadLine(std::string m1, std::string m2)
	{
		if (header.type == kMenuTypeLights) {
			//merge the mod id in now
			UInt32 formId = HexStringToUInt32(m1.c_str()) & 0xFFFFFF;

			UInt32 flags = mod->recordFlags;
			if (flags & (1 << 9)) {
				formId &= 0xFFF;
				formId |= 0xFE << 24;
				formId |= mod->lightIndex << 12;
			}
			else {
				formId |= mod->modIndex << 24;
			}

			(*menu)[categoryIndex].second.push_back(std::make_pair(UInt32ToHexString(formId), m2));
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

	try {
		validator.ValidateMenu();
	}
	catch (std::exception& e) {
		_Log("Error while trying to validate json menu: ", stem.c_str());
		_DMESSAGE(e.what());
	}

	if (validator.hasError) {
		_DMESSAGE(validator.errorStream.str().c_str());
		return false;
	}

	menuDataCache.emplace(stem, value);

	if (value.get("extension", false).asBool())
		extensionSet.insert(stem);

	return true;
}

bool OverrideJsonMenu(const char* path)
{
	std::string stem = std::filesystem::path(path).stem().string();

	Json::Value overrider;
	if (!SAF::ReadJsonFile(path, overrider))
		return false;

	Json::Value overrided = overrider.get("name", Json::Value());
	if (overrided.isNull() || !overrided.isString()) {
		_Log(stem.c_str(), " did not specify a menu name to override");
		return false;
	}

	auto overridedMenu = GetCachedMenu(overrided.asCString());
	if (!overridedMenu) {
		_Log("Could not find menu to override: ", overrided.asCString());
		return false;
	}

	//Override process is destructive so need to store the menu in case it fails
	Json::Value storedMenu(*overridedMenu);

	try {
		MergeJsons(overrider, overridedMenu);
	}
	catch (std::exception& e) {
		_Log("Error while trying to merge override menu: ", stem.c_str());
		_DMESSAGE(e.what());

		//return to stored menu
		*overridedMenu = storedMenu;

		return false;
	}

	std::string overriderName = stem + " Override";

	JsonMenuValidator validator(overriderName.c_str(), overridedMenu);
	try {
		validator.ValidateMenu();
	}
	catch (std::exception& e) {

		_Log("Error while trying to validate override menu: ", stem.c_str());
		_DMESSAGE(e.what());

		//return to stored menu
		*overridedMenu = storedMenu;

		return false;
	}
	
	if (validator.hasError) {
		_DMESSAGE(validator.errorStream.str().c_str());

		//return to stored menu
		*overridedMenu = storedMenu;

		return false;
	}

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
		OverrideJsonMenu(iter.GetFullPath().c_str());
	}
}

void ReloadJsonMenus()
{
	samManager.ForceQuit();

	menuDataCache.clear();
	extensionSet.clear();

	LoadJsonMenus();
}

bool SaveOptionsFile(const char* path)
{
	Json::Value value;

	menuOptions.ToJson(value);

	bool result = SAF::WriteJsonFile(path, value);

	if (!result)
		_Log("Failed to save options file: ", path);

	return result;
}

bool LoadOptionsFile(const char* path)
{
	//if options doesn't exist generate it
	if (!std::filesystem::exists(path)) {
		menuOptions.Initialize();
		SaveOptionsFile(path);
		return false;
	}

	Json::Value value;
	if (!SAF::ReadJsonFile(path, value)) {
		menuOptions.Initialize();
		return false;
	}

	menuOptions.FromJson(value);
	SaveOptionsFile(path);

	return true;
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
	LoadOptionsFile(OPTIONS_PATH);
}

Json::Value* GetCachedMenu(const char* name)
{
	auto it = menuDataCache.find(name);
	if (it != menuDataCache.end())
		return &it->second;

	return nullptr;
}

void GetMenuData(GFxResult& result, const char* name)
{
	Json::Value* menu = GetCachedMenu(name);
	if (!menu)
		return result.SetError(MENU_MISSING_ERROR);

	result.SetMenu(menu);
}

void GetExtensionMenus(GFxResult& result) {
	result.CreateMenuItems();

	//sort the extensions
	NaturalSortedSet set;
	for (auto& extension : extensionSet) {
		set.insert(extension);
	}

	for (auto& extension : set) {
		result.PushItem(extension.c_str(), extension.c_str());
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

void GetFolder(GFxResult& result, const char* path, const char* ext) {
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