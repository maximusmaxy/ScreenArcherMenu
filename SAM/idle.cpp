#include "idle.h"

#include "f4se/GameData.h"
#include "f4se/GameTypes.h"
#include "f4se/GameReferences.h"

#include "sam.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

IdleMenu idleMenu;
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
	UInt64 unk40;
	UInt64 unk48;
};

RelocAddr<BSTCaseInsensitiveStringMapIdle> idleStringMap(0x58D2EA0);

IdleMenu LoadIdleMenu() {
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

	std::map<UInt32, std::map<std::string, UInt32>> idles;

	BSTCaseInsensitiveStringMapIdle* stringMap = (BSTCaseInsensitiveStringMapIdle*)idleStringMap.GetUIntPtr();

	static BSFixedString behaviourGraph("actors\\Character\\Behaviors\\RaiderRootBehavior.hkx");
	static BSFixedString animationEvent("dyn_ActivationLoop");

	TESIdleForm** idleCount = stringMap->forms + stringMap->capacity;
	for (TESIdleForm** idlePtr = stringMap->forms; idlePtr < idleCount; ++idlePtr) {
		TESIdleForm* form = *idlePtr;

		if (form && form->editorID && form->formID &&
			form->behaviorGraph == behaviourGraph && form->animationEvent == animationEvent) {

			UInt32 modIndex = form->formID & (((form->formID & 0xFF000000) == 0xFE000000) ? 0xFFFFF000 : 0xFF000000);
			if (modNames.count(modIndex)) {
				idles[modIndex][form->editorID] = form->formID;
			}
		}
	}

	IdleMenu menu;

	for (auto& modKvp : idles) {
		std::string modName = modNames[modKvp.first];
		std::string noExtension = modName.substr(0, modName.find_last_of("."));
		menu.push_back(std::make_pair(noExtension, std::vector<std::pair<std::string, UInt32>>()));
		for (auto& formKvp : modKvp.second) {
			menu.back().second.push_back(formKvp);
		}
	}

	return menu;
}

IdleMenu* GetIdleMenu() {
	if (!idleMenuLoaded) {
		idleMenu = LoadIdleMenu();
		idleMenuLoaded = true;
	}
	return &idleMenu;
}

typedef bool (*PlayIdleInternal)(Actor::MiddleProcess* middleProcess, Actor* actor, UInt32 param3, TESIdleForm* idleForm, UInt64 param5, UInt64 param6);
RelocAddr<PlayIdleInternal> playIdleAnimation(0xE35510);

bool PlayIdleAnimation(UInt32 formId) {
	if (!selected.refr) return false;

	Actor* actor = (Actor*)selected.refr;

	TESIdleForm* idleForm = (TESIdleForm*)LookupFormByID(formId);

	return (*playIdleAnimation)(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void ResetIdleAnimation() {
	if (!selected.refr) return;

	Actor* actor = (Actor*)selected.refr;

	//reset idle formId
	UInt16 modIndex = ((*g_dataHandler)->GetLoadedLightModIndex("ScreenArcherMenu.esp"));

	if (!modIndex || modIndex == 0xFFFF) return;

	UInt32 formId = (0xFE << 24) + ((*g_dataHandler)->GetLoadedLightModIndex("ScreenArcherMenu.esp") << 12) + 0x801;

	TESIdleForm* idleForm = (TESIdleForm*)LookupFormByID(formId);

	if (!idleForm) return;

	(*playIdleAnimation)(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void GetIdleMenuCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	IdleMenu* menu = GetIdleMenu();
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

	IdleMenu* menu = GetIdleMenu();
	if (!menu) return;

	for (auto& category : (*menu)[selectedCategory].second) {
		GFxValue name(category.first.c_str());
		names.PushBack(&name);

		GFxValue value(category.second);
		values.PushBack(&value);
	}

	result->SetMember("names", &names);
	result->SetMember("values", &values);
}