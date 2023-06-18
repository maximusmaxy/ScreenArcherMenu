#include "idle.h"

#include "f4se/GameData.h"
#include "f4se/GameTypes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameRTTI.h"

#include "sam.h"
#include "types.h"
#include "constants.h"
#include "pose.h"
#include "inventory.h"
#include "forms.h"
#include "SAF/util.h"
#include "SAF/adjustments.h"
#include "SAF/types.h"

#include "strnatcmp.h"

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <execution>
#include <span>

std::unordered_map<UInt32, IdleData> raceIdleData;

std::vector<const char*> idleModCategories;
const char* storedIdleBehavior = nullptr;

std::vector<std::string> idleFavorites;

typedef TESForm* (*_GetFormByEditorId)(const char* edid);
RelocAddr<_GetFormByEditorId> GetFormByEditorId(0x152EB0);

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

//RelocAddr<UInt64> BSTCaseInsensitiveStringMapIdleVFTable(0x2CB9478);
//RelocAddr<UInt64> TESIdleFormVFTable(0x2CB8FA8);

typedef NiFormArrayTESIdle* (*_GetFormArrayFromName)(BSTCaseInsensitiveStringMapIdle* idleMap, const char* name);
RelocAddr<_GetFormArrayFromName> GetFormArrayFromName(0x5AAC80);

NiFormArrayTESIdle* GetIdleFormArrayForBehavior(BSFixedString behavior) {
	auto idleMap = (BSTCaseInsensitiveStringMapIdle*)idleStringMap.GetUIntPtr();
	auto idleFormArray = GetFormArrayFromName(idleMap, behavior.c_str());
	idleFormArray++; //need to increment for some reason
	return idleFormArray;
}

void FindIdleMods(BSFixedString behavior, std::vector<const char*>& result) {
	std::vector<bool> esp;
	std::vector<bool> esl;
	GetModVectors(esp, esl);
	auto espIt = esp.begin();
	auto eslIt = esl.begin();

	auto formArray = GetIdleFormArrayForBehavior(behavior);
	std::span<TESIdleForm*> idleFormSpan(formArray->forms, formArray->count);

	std::for_each(std::execution::par_unseq, idleFormSpan.begin(), idleFormSpan.end(), [&espIt, &eslIt](TESIdleForm* form) {
		if (form) {
			const UInt32 formId = form->formID;
			if ((formId & 0xFE000000) == 0xFE000000) {
				*(eslIt + ((formId >> 12) & 0xFFF)) = true;
			}
			else {
				*(espIt + (formId >> 24)) = true;
			}
		}
	});

	AddModVectorsToListNoMasters(esp, esl, result);
}

void FindIdleForms(const ModInfo* mod, IdleData* data, std::vector<std::pair<const char*, UInt32>>& result) {
	auto behavior = data->behavior;
	auto event = data->event;
	auto formArray = GetIdleFormArrayForBehavior(behavior);
	std::span<TESIdleForm*> idleFormSpan(formArray->forms, formArray->count);
	std::mutex mutex;

	auto IdleValid = [](TESIdleForm* form, BSFixedString behavior, BSFixedString event) -> bool {
		if (!form)
			return false;
		//if (*(UInt64*)form != TESIdleFormVFTable)
		//	return false;
		if (!form->formID)
			return false;
		if (!form->editorID)
			return false;
		if (!(form->behaviorGraph == behavior))
			return false;
		if (!(form->animationEvent == event))
			return false;
		return true;
	};

	if (!mod->IsLight()) {
		const UInt32 modIndex = mod->modIndex << 24;
		std::for_each(std::execution::par, idleFormSpan.begin(), idleFormSpan.end(), [&](TESIdleForm* form) {
			if (IdleValid(form, behavior, event) && (form->formID & 0xFF000000) == modIndex) {
				std::lock_guard lock(mutex);
				result.emplace_back(form->editorID, form->formID);
			}
		});
	}
	else {
		const UInt32 modIndex = 0xFE000000 | (mod->lightIndex << 12);
		std::for_each(std::execution::par, idleFormSpan.begin(), idleFormSpan.end(), [&](TESIdleForm* form) {
			if (IdleValid(form, behavior, event) && (form->formID & 0xFFFFF000) == modIndex) {
				std::lock_guard lock(mutex);
				result.emplace_back(form->editorID, form->formID);
			}
		});
	}

	std::sort(result.begin(), result.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.first, rhs.first) < 0;
	});
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

IdleData* GetIdleData(TESObjectREFR* refr) {
	auto actor = DYNAMIC_CAST(refr, TESObjectREFR, Actor);
	if (!actor)
		return nullptr;

	return GetIdleData(actor->race->formID);
}

//IdleMenu* GetIdleMenu(UInt32 raceId) {
//	if (!idleMenuLoaded) {
//		LoadIdleMenus();
//		idleMenuLoaded = true;
//	}
//	
//	IdleData* idleData = GetIdleData(raceId);
//	if (!idleData) 
//		return nullptr;
//
//	auto it = idleMenus.find(idleData->behavior);
//	if (it == idleMenus.end())
//		return nullptr;
//
//	return &it->second;
//}

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

	PlayIdleInternal(actor->middleProcess, actor, 0x35, idleForm, 1, 0);
	samManager.ShowNotification(idleForm->editorID, true);
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
	samManager.ShowNotification("", true);
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

void GetIdleMenuCategories(GFxResult& result)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	IdleData* idleData = GetIdleData(selected.refr);
	if (!idleData)
		return result.SetError("Failed to get idle data for selected race");

	if (idleData->behavior != storedIdleBehavior) {
		idleModCategories.clear();
		FindIdleMods(idleData->behavior, idleModCategories);
		storedIdleBehavior = idleData->behavior;
	}

	result.CreateMenuItems();

	//favorites
	result.PushItem("$SAM_IdleFavorites", (SInt32)-1);
	
	for (SInt32 i = 0; i < idleModCategories.size(); ++i) {
		const char* mod = idleModCategories.at(i);
		auto ext = strrchr(mod, '.');
		if (ext) {
			std::string str(mod, ext - mod);
			result.PushItem(str.c_str(), i);
		}
		else {
			result.PushItem(mod, i);
		}
	}
}

void SetIdleMod(GFxResult& result, SInt32 selectedCategory)
{
	if (selectedCategory == -1) {
		samManager.PushMenu("PlayIdleFavorites");
	}
	else {
		samManager.PushMenu("PlayIdleList");
	}
}

void GetIdleMenu(GFxResult& result, SInt32 selectedCategory)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto idleData = GetIdleData(selected.refr);
	if (!idleData)
		return result.SetError("Failed to get idle data for selected race");

	if (selectedCategory < 0 || selectedCategory >= idleModCategories.size())
		return result.SetError("Play idle menu not found");

	const ModInfo* mod = (*g_dataHandler)->LookupModByName(idleModCategories[selectedCategory]);
	if (!mod)
		return result.SetError("Could not find mod");

	std::vector<std::pair<const char*, UInt32>> searchResult;
	FindIdleForms(mod, idleData, searchResult);

	result.CreateMenuItems();
	for (auto& [edid, formId] : searchResult) {
		result.PushItem(edid, formId);
	}
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

//Actor* GetActorFromCachedIdleData(CachedIdleData& data) {
//	if (!data.animationGraph || !data.animationFile) 
//		return nullptr;
//
//	auto graphData = data.animationGraph->animationGraphData;
//	if (!graphData) 
//		return nullptr;
//
//	auto graphManager = graphData->animationGraphManager;
//	if (!graphManager) 
//		return nullptr;
//
//	auto dataChannel = graphManager->dataChannels[0];
//	if (!dataChannel) 
//		return nullptr;
//
//	return dataChannel->actor;
//}

const char* GetCurrentIdleName() {
	if (!selected.refr) 
		return nullptr;

	//Get the actors anim graph
	auto idleData = GetIdleData(selected.refr);
	if (!idleData)
		return nullptr;

	auto actor = (Actor*)selected.refr;
	auto middleProcess = actor->middleProcess;
	if (!middleProcess)
		return nullptr;

	auto unk08 = middleProcess->unk08;
	if (!unk08)
		return nullptr;

	auto animGraphHolder = (BShkbAnimationGraphHolder*)unk08->unk00[0x260 >> 3];
	if (!animGraphHolder)
		return nullptr;

	BShkbAnimationGraph* animGraph = animGraphHolder->animGraph;
	if (!animGraph)
		return nullptr;

	//Compare the anim graph against the cached idles to find the anim path
	auto idles = (tMutexArray<CachedIdleData>*)cachedIdles.GetUIntPtr();

	auto GetCachedIdleForAnimGraph = [](tMutexArray<CachedIdleData>* idles, BShkbAnimationGraph* animGraph) -> CachedIdleData* {
		SimpleLocker locker(&idles->lock);
		for (auto it = idles->entries; it != (idles->entries + idles->count); ++it) {
			if (it->animationGraph == animGraph)
				return it;
		}
		return nullptr;
	};

	auto cachedIdle = GetCachedIdleForAnimGraph(idles, animGraph);
	if (!cachedIdle)
		return nullptr;

	auto idlePath = cachedIdle->animationFile;
	if (!idlePath.c_str() || !*idlePath.c_str())
		return nullptr;

	//Search the actors idle forms for the form that has the animation path
	auto formArray = GetIdleFormArrayForBehavior(idleData->behavior.c_str());
	std::span<TESIdleForm*> idleFormSpan(formArray->forms, formArray->count);

	auto it = std::find_if(std::execution::par_unseq, idleFormSpan.begin(), idleFormSpan.end(), [&idlePath](TESIdleForm* form) {
		return form && (form->editorID == idlePath.c_str());
	});

	if (it != idleFormSpan.end())
		return (*it)->editorID;

	return nullptr;
}

void GetIdleFavorites(GFxResult& result)
{
	result.CreateMenuItems();

	//natural sort them
	NaturalSortedSet set;
	for (auto& item : idleFavorites) {
		set.insert(item);
	}

	for (auto& item : set) {
		result.PushItem(item.c_str(), item.c_str());
	}
}

bool SaveIdleFavorites()
{
	SAF::OutStreamWrapper wrapper(IDLE_FAVORITES);
	if (wrapper.fail)
		return false;

	for (auto& favorite : idleFavorites) {
		wrapper.stream << favorite << std::endl;
	}

	return true;
}

void AppendIdleFavorite(GFxResult& result)
{
	const char* idleName = GetCurrentIdleName();
	if (!idleName)
		return result.SetError("Idle name could not be found");

	TESForm* form = GetFormByEditorId(idleName);
	if (!form)
		return result.SetError("Could not find form for idle");

	auto it = std::find(idleFavorites.begin(), idleFavorites.end(), idleName);
	if (it != idleFavorites.end())
		return result.SetError("Idle has already been favorited!");

	idleFavorites.push_back(idleName);

	//std::ofstream stream;
	//if (!SAF::OpenAppendFileStream(IDLE_FAVORITES, &stream))
	//	return result.SetError("Failed to open IdleFavorites.txt");

	//stream << idleName << std::endl;
	//stream.close();

	//TODO Append wasn't working so we're just writing the whole thing for now
	if (!SaveIdleFavorites())
		return result.SetError("Failed to save IdleFavorites.txt");

	std::string notif = std::string(idleName) + " has been favorited!";
	samManager.ShowNotification(notif.c_str(), false);
}

void RemoveIdleFavorite(GFxResult& result, SInt32 index) {
	if (index < 0 || index >= idleFavorites.size())
		return result.SetError("Idle index out of range");

	idleFavorites.erase(idleFavorites.begin() + index);

	if (!SaveIdleFavorites())
		return result.SetError("Failed to save IdleFavorites.txt");
}

void PlayIdleFavorite(GFxResult& result, const char* idleName)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = GetFormByEditorId(idleName);
	if (!form)
		return result.SetError("Could not find form for idle");

	TESIdleForm* idleForm = DYNAMIC_CAST(form, TESForm, TESIdleForm);
	if (!idleForm)
		return result.SetError("Could not find form for idle");

	Actor* actor = (Actor*)selected.refr;
	if (!PlayIdleInternal(actor->middleProcess, actor, 0x35, idleForm, 1, 0))
		return result.SetError("Play idle request failed");

	samManager.ShowNotification(idleForm->editorID, true);
}

bool LoadIdleFavorites()
{
	if (!std::filesystem::exists(IDLE_FAVORITES)) {
		IFileStream::MakeAllDirs(IDLE_FAVORITES);
		IFileStream file;
		if (!file.Create(IDLE_FAVORITES)) {
			_Log("Failed to create idle favorites: ", IDLE_FAVORITES);
			return false;
		}
		file.Close();
	}

	std::ifstream stream;
	stream.open(IDLE_FAVORITES);

	if (stream.fail()) {
		_Log("Failed to read idle favorites: ", IDLE_FAVORITES);
		return false;
	}

	idleFavorites.clear();

	std::string line;
	while (std::getline(stream, line, '\n'))
	{
		if (line.back() == '\r')
			line.pop_back();
		idleFavorites.push_back(line);
	}

	return true;
}