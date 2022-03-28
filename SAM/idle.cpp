#include "idle.h"

#include "f4se/GameData.h"
#include "f4se/GameTypes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "sam.h"
#include "SAF/util.h"

#include "strnatcmp.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

std::unordered_map<UInt32, IdleData> raceIdleData;

std::unordered_map<UInt32, IdleMenu> idleMenus;
bool idleMenuLoaded = false;

std::unordered_set<std::string> idleExclude = {
	"Fallout4.esm",
	"DLCRobot.esm",
	"DLCworkshop01.esm",
	"DLCworkshop02.esm",
	"DLCworkshop03.esm",
	"DLCCoast.esm",
	"DLCNukaWorld.esm",
	"ScreenArcherMenu.esp"
};

//0x58D0AA0 struct* ->

//0x58D2EA0 
//0x58D2EA8 BSTCaseInsensitiveStringMap@PEAVIDLE_ANIM_ROOT
// B0
// B8 BSFixedStrings
// C0
// C8
//0x58D2ED0 Tesidleform* array
//0x58D2ED8 capacity maybe
// E0
// E8

//I'm not sure exactly what this is but it contains an array of every idle in the game
class BSTCaseInsensitiveStringMapIdle
{
public:
	UInt64 unk00;
	UInt64 VfTable;
	UInt64 unk10;
	BSFixedString** strings;
	UInt64 unk20;
	UInt64 unk28;
	TESIdleForm** forms;
	UInt64 capacity;
	UInt64 count;
	UInt64 unk48;
};

class NiFormArrayTESIdle
{
public:
	UInt64 Vftable;
	TESIdleForm** forms;
	UInt32 capacity;
	UInt32 count;
	UInt32 unk18;
	UInt32 unk1C;
};

RelocAddr<BSTCaseInsensitiveStringMapIdle> idleStringMap(0x58D2EA0);

RelocAddr<UInt64> BSTCaseInsensitiveStringMapIdleVFTable(0x2CB9478);

RelocAddr<UInt64> TESIdleFormVFTable(0x2CB8FA8);

typedef NiFormArrayTESIdle* (*GetFormArrayFromName)(BSTCaseInsensitiveStringMapIdle* idleMap, const char* name);
RelocAddr<GetFormArrayFromName> getFormArrayFromName(0x5AAC80);

struct naturalSort {
	bool operator()(const std::string& a, const std::string& b) const {
		return strnatcasecmp(a.c_str(), b.c_str()) < 0;
	}
};

bool AddIdleToMap(TESIdleForm* form, std::map<UInt32, std::map<std::string, UInt32, naturalSort>>& idles, std::unordered_map<UInt32, std::string>& modNames, IdleData& data) {
	if (!form) return false;
	if (*(UInt64*)form != TESIdleFormVFTable) return false;
	if (!form->formID) return false;
	if (!form->editorID) return false;
	if (!(form->behaviorGraph == data.behavior)) return false;
	if (!(form->animationEvent == data.event)) return false;

	UInt32 modIndex = form->formID & (((form->formID & 0xFF000000) == 0xFE000000) ? 0xFFFFF000 : 0xFF000000);
	if (modNames.count(modIndex)) {
		idles[modIndex][form->editorID] = form->formID;
	}

	return false;
}

void LoadIdleMenus() {
	std::unordered_map<UInt32, std::string> modNames;

	tArray<ModInfo*>* loadedMods = &(*g_dataHandler)->modList.loadedMods;
	ModInfo** loadedCount = loadedMods->entries + loadedMods->count;
	for (ModInfo** loaded = loadedMods->entries; loaded < loadedCount; ++loaded)
	{
		ModInfo* modInfo = *loaded;
		if (modInfo && modInfo->name && modInfo->modIndex && !idleExclude.count(modInfo->name)) {
			UInt32 modIndex = (modInfo->modIndex << 24);
			modNames[modIndex] = modInfo->name;
		}
	}

	tArray<ModInfo*>* lightMods = &(*g_dataHandler)->modList.lightMods;
	ModInfo** lightCount = lightMods->entries + lightMods->count;
	for (ModInfo** light = lightMods->entries; light < lightCount; ++light)
	{
		ModInfo* modInfo = *light;
		if (modInfo && modInfo->name && modInfo->lightIndex) {
			UInt32 modIndex = (0xFE << 24) + (modInfo->lightIndex << 12);
			modNames[modIndex] = modInfo->name;
		}
	}

	for (auto& dataKvp : raceIdleData) {
		std::map<UInt32, std::map<std::string, UInt32, naturalSort>> idles;

		NiFormArrayTESIdle* idleFormArray = (*getFormArrayFromName)((BSTCaseInsensitiveStringMapIdle*)idleStringMap.GetUIntPtr(), dataKvp.second.behavior.c_str());
		idleFormArray++;

		for (TESIdleForm** idlePtr = idleFormArray->forms; idlePtr < idleFormArray->forms + idleFormArray->count; ++idlePtr) {
			if (AddIdleToMap(*idlePtr, idles, modNames, dataKvp.second)) {
				break;
			}
		}

		for (auto& modKvp : idles) {
			std::string modName = modNames[modKvp.first];
			std::string noExtension = modName.substr(0, modName.find_last_of("."));
			idleMenus[dataKvp.first].push_back(std::make_pair(noExtension, std::vector<std::pair<std::string, UInt32>>()));
			for (auto& formKvp : modKvp.second) {
				idleMenus[dataKvp.first].back().second.push_back(formKvp);
			}
		}
	}
}

UInt32 GetIdleDataId(UInt32 raceId) {
	if (raceIdleData.count(raceId)) return raceId;
	//Default to human if missing
	if (raceIdleData.count(0x13746)) return 0x13746;
	//Human really shouldn't be missing
	return 0;
}

IdleMenu* GetIdleMenu(UInt32 raceId) {
	if (!idleMenuLoaded) {
		LoadIdleMenus();
		idleMenuLoaded = true;
	}
	
	UInt32 dataId = GetIdleDataId(raceId);
	if (!dataId) return nullptr;

	return &idleMenus[dataId];
}

typedef bool (*PlayIdleInternal)(Actor::MiddleProcess* middleProcess, Actor* actor, UInt32 param3, TESIdleForm* idleForm, UInt64 param5, UInt64 param6);
RelocAddr<PlayIdleInternal> playIdleAnimation(0xE35510);

bool PlayIdleAnimation(UInt32 formId) {
	if (!selected.refr) return false;

	Actor* actor = (Actor*)selected.refr;

	TESForm* form = LookupFormByID(formId);
	if (!form) return false;

	TESIdleForm* idleForm = DYNAMIC_CAST(form, TESForm, TESIdleForm);
	if (!idleForm) return false;

	return (*playIdleAnimation)(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void ResetIdleAnimation() {
	if (!selected.refr) return;

	Actor* actor = (Actor*)selected.refr;
	
	UInt32 dataId = GetIdleDataId(selected.race);
	if (!dataId) return;

	UInt32 resetId = raceIdleData[dataId].resetId;
	if (!resetId) return;

	TESForm* form = LookupFormByID(resetId);
	if (!form) return;

	TESIdleForm* idleForm = DYNAMIC_CAST(form, TESForm, TESIdleForm);
	if (!idleForm) return;

	(*playIdleAnimation)(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void GetIdleMenuCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	if (!selected.refr) return;

	IdleMenu* menu = GetIdleMenu(selected.race);
	if (!menu) return;

	for (auto& category : *menu) {
		GFxValue name(category.first.c_str());
		result->PushBack(&name);
	}
}

void GetIdleMenuGFx(GFxMovieRoot* root, GFxValue* result, UInt32 selectedCategory)
{
	root->CreateObject(result);

	GFxValue names, values;
	root->CreateArray(&names);
	root->CreateArray(&values);

	IdleMenu* menu = GetIdleMenu(selected.race);
	if (!menu || selectedCategory >= menu->size()) return;

	for (auto& category : (*menu)[selectedCategory].second) {
		GFxValue name(category.first.c_str());
		names.PushBack(&name);

		GFxValue value(category.second);
		values.PushBack(&value);
	}

	result->SetMember("names", &names);
	result->SetMember("values", &values);
}