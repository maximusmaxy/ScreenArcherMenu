#include "idle.h"

#include "f4se/GameData.h"
#include "f4se/GameTypes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "sam.h"
#include "SAF/util.h"
#include "SAF/adjustments.h"

#include "strnatcmp.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

std::unordered_map<UInt32, IdleData> raceIdleData;

typedef std::unordered_map<BSFixedString, IdleMenu, SAF::BSFixedStringHash, SAF::BSFixedStringKeyEqual> IdleMenuMap;
IdleMenuMap idleMenus;
bool idleMenuLoaded = false;

std::unordered_map<const char*, const char*> idlePathToEditorIdMap;

std::unordered_set<std::string> idleExclude = {
	"Fallout4.esm",
	"DLCRobot.esm",
	"DLCworkshop01.esm",
	"DLCworkshop02.esm",
	"DLCworkshop03.esm",
	"DLCCoast.esm",
	"DLCNukaWorld.esm"//,
	//"ScreenArcherMenu.esp"
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

typedef NiFormArrayTESIdle* (*_GetFormArrayFromName)(BSTCaseInsensitiveStringMapIdle* idleMap, const char* name);
RelocAddr<_GetFormArrayFromName> GetFormArrayFromName(0x5AAC80);

typedef std::map<std::string, std::map<std::string, UInt32, NaturalSort>, NaturalSort> SortedIdles;

bool IdleValid(TESIdleForm* form, BSFixedString behavior, BSFixedString event) {
	if (!form) return false;
	if (*(UInt64*)form != TESIdleFormVFTable) return false;
	if (!form->formID) return false;
	if (!form->editorID) return false;
	if (!(form->behaviorGraph == behavior)) return false;
	if (!(form->animationEvent == event)) return false;
	return true;
}

void GetLoadedModNames(std::unordered_map<UInt32, std::string>& modNames, tArray<ModInfo*>& modList, bool light)
{
	ModInfo** loadedCount = modList.entries + modList.count;
	for (ModInfo** loaded = modList.entries; loaded < loadedCount; ++loaded)
	{
		ModInfo* modInfo = *loaded;
		if (modInfo && modInfo->name && modInfo->modIndex && !idleExclude.count(modInfo->name)) {
			UInt32 modIndex = light ? (0xFE << 24) + (modInfo->lightIndex << 12) : (modInfo->modIndex << 24);
			std::string modName = modInfo->name;
			modNames[modIndex] = modName.substr(0, modName.find_last_of("."));
		}
	}
}

void LoadIdleMenus() {
	std::unordered_map<UInt32, std::string> modNames;

	GetLoadedModNames(modNames, (*g_dataHandler)->modList.loadedMods, false);
	GetLoadedModNames(modNames, (*g_dataHandler)->modList.lightMods, true);

	//Need to group idles by behavior so make a set to prevent duplicates
	SAF::BSFixedStringMap behaviorSet; //TODO: this will break if the same behavior with a different event exists
	for (auto& data : raceIdleData) {
		behaviorSet.emplace(data.second.behavior, data.second.event);
	}

	for (auto& behavior : behaviorSet) {
		SortedIdles idles;

		NiFormArrayTESIdle* idleFormArray = GetFormArrayFromName((BSTCaseInsensitiveStringMapIdle*)idleStringMap.GetUIntPtr(), behavior.first.c_str());
		idleFormArray++;

		for (TESIdleForm** idlePtr = idleFormArray->forms; idlePtr < idleFormArray->forms + idleFormArray->count; ++idlePtr) {
			TESIdleForm* form = *idlePtr;

			if (IdleValid(form, behavior.first, behavior.second)) {
				UInt32 modIndex = form->formID & (((form->formID & 0xFF000000) == 0xFE000000) ? 0xFFFFF000 : 0xFF000000);
				if (modNames.count(modIndex)) {
					idles[modNames[modIndex]][form->editorID] = form->formID;
					idlePathToEditorIdMap.emplace(form->animationFile.c_str(), form->editorID);
				}
			}
		}

		for (auto& modKvp : idles) {
			idleMenus[behavior.first].push_back(std::make_pair(modKvp.first, std::vector<std::pair<std::string, UInt32>>()));
			for (auto& formKvp : modKvp.second) {
				idleMenus[behavior.first].back().second.push_back(formKvp);
			}
		}
	}
}

IdleData* GetIdleData(UInt32 raceId) {
	auto foundRace = raceIdleData.find(raceId);
	if (foundRace != raceIdleData.end())
		return &foundRace->second;

	//Default to human if missing
	auto foundHuman = raceIdleData.find(0x13746);
	if (foundHuman != raceIdleData.end())
		return &foundHuman->second;

	//Human really shouldn't be missing
	_DMESSAGE("Human idle data not found");

	return nullptr;
}

IdleMenu* GetIdleMenu(UInt32 raceId) {
	if (!idleMenuLoaded) {
		LoadIdleMenus();
		idleMenuLoaded = true;
	}
	
	IdleData* idleData = GetIdleData(raceId);
	if (!idleData) 
		return nullptr;

	auto it = idleMenus.find(idleData->behavior);
	if (it == idleMenus.end())
		return nullptr;

	return &it->second;
}

typedef bool (*_PlayIdleInternal)(Actor::MiddleProcess* middleProcess, Actor* actor, UInt32 param3, TESIdleForm* idleForm, UInt64 param5, UInt64 param6);
RelocAddr<_PlayIdleInternal> PlayIdleInternal(0xE35510);

void PlayIdleAnimation(UInt32 formId) {
	if (!selected.refr) 
		return;

	Actor* actor = (Actor*)selected.refr;

	TESForm* form = LookupFormByID(formId);
	if (!form) 
		return;

	TESIdleForm* idleForm = DYNAMIC_CAST(form, TESForm, TESIdleForm);
	if (!idleForm)
		return;

	bool result = PlayIdleInternal(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void ResetIdleAnimation() {
	if (!selected.refr) 
		return;

	Actor* actor = (Actor*)selected.refr;
	
	IdleData* idleData = GetIdleData(selected.race);
	if (!idleData) 
		return;

	UInt32 resetId = idleData->resetId;
	if (!resetId) 
		return;

	TESForm* form = LookupFormByID(resetId);
	if (!form) 
		return;

	TESIdleForm* idleForm = DYNAMIC_CAST(form, TESForm, TESIdleForm);
	if (!idleForm) 
		return;

	PlayIdleInternal(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void PlayAPose() {
	if (!selected.refr) 
		return;

	Actor* actor = (Actor*)selected.refr;

	IdleData* idleData = GetIdleData(selected.race);
	if (!idleData) return;

	UInt32 aposeId = idleData->aposeId;
	if (!aposeId) 
		return;

	TESForm* form = LookupFormByID(aposeId);
	if (!form) 
		return;

	TESIdleForm* idleForm = DYNAMIC_CAST(form, TESForm, TESIdleForm);
	if (!idleForm) 
		return;

	PlayIdleInternal(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
}

void GetIdleMenuCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	if (!selected.refr) 
		return;

	IdleMenu* menu = GetIdleMenu(selected.race);
	if (!menu) 
		return;

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
	if (!menu || selectedCategory >= menu->size()) 
		return;

	for (auto& category : (*menu)[selectedCategory].second) {
		GFxValue name(category.first.c_str());
		names.PushBack(&name);

		GFxValue value(category.second);
		values.PushBack(&value);
	}

	result->SetMember("names", &names);
	result->SetMember("values", &values);
}

class BSTAnimationGraphDataChannel
{
public:
	UInt64 vfTable;
	UInt64 unk2;
	UInt64 unk3;
	UInt64 unk4;
	UInt64 unk5;
	Actor* actor;
};

class BSAnimationGraphManager
{
public:
	UInt64 vfTable;
	UInt64 unk2;
	BSTAnimationGraphDataChannel* dataChannels[12];
};

class BShkbAnimationGraphData
{
public:
	UInt64 actorMediator;
	BSAnimationGraphManager* animationGraphManager;
};

class BShkbAnimationGraph
{
public:
	UInt64 vfTable;
	UInt64 unk2;
	UInt64 unk3;
	UInt64 unk4;
	UInt64 unk5;
	UInt64 unk6;
	UInt64 unk7;
	UInt64 unk8;
	UInt64 unk9;
	UInt64 unk10;
	UInt64 unk11;
	UInt64 unk12;
	UInt64 unk13;
	UInt64 unk14;
	BShkbAnimationGraphData* animationGraphData;
};

class BShkbAnimationGraphHolder
{
public:
	UInt64 unk1;
	UInt64 unk2;
	UInt64 unk3;
	UInt64 unk4;
	UInt64 unk5;
	UInt64 unk6;
	UInt64 unk7;
	BShkbAnimationGraph* animGraph;
};

struct CachedIdleData
{
	BSFixedString animationFile;
	UInt64 unk2;
	UInt64 hkbAnimationBindingWithTriggers;
	UInt64 hkbClipGenerator;
	BShkbAnimationGraph* animationGraph;
};

RelocAddr<tMutexArray<CachedIdleData>> cachedIdles(0x5B02F38);

Actor* GetActorFromCachedIdleData(CachedIdleData& data) {
	if (!data.animationGraph || !data.animationFile) 
		return nullptr;

	auto graphData = data.animationGraph->animationGraphData;
	if (!graphData) 
		return nullptr;

	auto graphManager = graphData->animationGraphManager;
	if (!graphManager) 
		return nullptr;

	auto dataChannel = graphManager->dataChannels[0];
	if (!dataChannel) 
		return nullptr;

	return dataChannel->actor;
}

/*
* To find the current idle name i'm looking at what idles are cached, finding the animation graph for each idle,
* backtracing to see which actor is managed by that animation graph and checking to see if that's the currently selected actor
* Then once that cached idle is found i'm using a map of file->editor ids, created at the same time the idle menus are cached, 
* to find the name. This is extremely convoluted but i could not find a better solution as it seems the currently playing idle 
* form isn't stored anywhere I could find
* 
* From tracing the IsLastIdlePlayed console command (unimplemented) it seems like it was at one point located at 
* actor->middleprocess->data->(0x3F8) but it is never actually used at runtime
*/
const char* GetCurrentIdleName() {
	if (!selected.refr) 
		return nullptr;
	
	Actor* actor = (Actor*)selected.refr;

	auto animGraphHolder = (BShkbAnimationGraphHolder*)actor->middleProcess->unk08->unk00[0x260 >> 3];
	if (!animGraphHolder)
		return nullptr;

	BShkbAnimationGraph* animGraph = animGraphHolder->animGraph;
	if (!animGraph)
		return nullptr;

	auto idles = (tMutexArray<CachedIdleData>*)cachedIdles.GetUIntPtr();

	SimpleLocker locker(&idles->lock);

	for (int i = 0; i < idles->count; ++i) {
		if (idles->entries[i].animationGraph == animGraph) {
			return idles->entries[i].animationFile ? idlePathToEditorIdMap[idles->entries[i].animationFile.c_str()] : nullptr;
		}
	}

	return nullptr;
}