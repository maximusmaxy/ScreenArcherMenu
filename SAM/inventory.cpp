#include "inventory.h"

#include "f4se/GameForms.h"
#include "f4se_common/Relocation.h"
#include "f4se/GameData.h"
#include "f4se/GameObjects.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameRTTI.h"
#include "f4se/PapyrusVM.h"

#include "SAF/util.h"

#include "sam.h"
#include "constants.h"
#include "idle.h"
#include "types.h"
#include "papyrus.h"
#include "types.h"
#include "camera.h"
#include "positioning.h"
#include "strnatcmp.h"

#include "BS_thread_pool.hpp"

#include <thread>
#include <fstream>
#include <filesystem>
#include <array>

#define SEARCH_SIZE 9
FormSearchResult substringSearchResult;
NaturalSortedSet storedItemMods;
std::string lastSelectedMod;
bool availableModGroups[SEARCH_SIZE];

const char* modGroupNames[] = {
	"Armor",
	"Weapon",
	"Chems",
	"Ammo",
	"Book",
	"Note",
	"Key",
	"Mod",
	"Misc"
};

//std::unordered_map<UInt32, std::string> itemGroupMap {
//	{'OMRA', "Armour"},
//	{'OMMA', "Ammo"},
//	{'PAEW', "Weapon"},
//	{'KOOB', "Book"},
//	{'ENOT', "Note"},
//	{'DOMO', "Mod"},
//	{'MYEK', "Key"},
//	{'HCLA', "Chems"},
//	{'CSIM', "Misc"}

void FormSearchResult::Push(const char* name, UInt32 formId) 
{
	std::lock_guard lock(mutex);
	list.push_back(std::make_pair(name, formId));
}

void FormSearchResult::Sort() 
{
	struct {
		bool operator() (std::pair<const char*,UInt32>& a, std::pair<const char*, UInt32>& b) const {
			return strnatcasecmp(a.first, b.first) < 0;
		}
	}
	sorter;
	
	std::sort(list.begin(), list.end(), sorter);
}

void FormSearchResult::Clear()
{
	list.clear();
}

int GetThreadCount() {
	int threads = std::thread::hardware_concurrency();
	if (threads > 1)
		threads -= 2;

	return threads;
}

void GetFormArraysAndSize(tArray<TESForm*>** formArrays, UInt32* size)
{
	formArrays[0] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrARMO);
	formArrays[1] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrWEAP);
	formArrays[2] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrALCH);
	formArrays[3] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrAMMO);
	formArrays[4] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrBOOK);
	formArrays[5] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrNOTE);
	formArrays[6] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrKEYM);
	formArrays[7] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrOMOD);
	formArrays[8] = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrMISC);

	*size = 0;
	for (auto it = formArrays; it != formArrays + SEARCH_SIZE; ++it) {
		*size += (*it)->count;
	}
}

//Gets the slice of the current form array to iterate through and sets the current position to the next array
bool GetFormStartAndEnd(tArray<TESForm*>** dataArrays, UInt32* current, UInt32 endIndex, TESForm*** start, TESForm*** end)
{
	UInt32 size = 0;
	UInt32 newSize;

	for (auto it = dataArrays; it != dataArrays + SEARCH_SIZE; ++it) {
		newSize = size + (*it)->count;
		if (*current < newSize) {
			*start = (*it)->entries + (*current - size);
			*end = (*it)->entries + min((*it)->count, endIndex - size);
			*current = newSize;
			return true;
		}
		size = newSize;
	}

	return false;
}

bool GetFormStartAndEndWithIndex(tArray<TESForm*>** dataArrays, UInt32* current, UInt32 endIndex, TESForm*** start, TESForm*** end, int* index)
{
	UInt32 size = 0;
	UInt32 newSize;

	for (int i = 0; i < SEARCH_SIZE; ++i) {
		newSize = size + dataArrays[i]->count;
		if (*current < newSize) {
			*start = dataArrays[i]->entries + (*current - size);
			*end = dataArrays[i]->entries + min(dataArrays[i]->count, endIndex - size);
			*current = newSize;
			*index = i;
			return true;
		}
		size = newSize;
	}

	return false;
}

typedef bool (*_OpenActorContainerInternal)(TESObjectREFR* refr, UInt32 unk2, bool unk3);
RelocAddr<_OpenActorContainerInternal> OpenActorContainerInternal(0x1264F40);

void OpenActorContainer(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	OpenActorContainerInternal(selected.refr, 3, false);
}

//void GetModsFromModList(tArray<ModInfo*>& modlist, NaturalSortedCStringSet& mods) 
//{
//	for (auto it = modlist.entries; it < modlist.entries + modlist.count; ++it) {
//		mods.insert((*it)->name);
//	}
//}

//void GetItemMods(GFxResult& result) {
//	NaturalSortedCStringSet mods;
//	GetModsFromModList((*g_dataHandler)->modList.loadedMods, mods);
//	GetModsFromModList((*g_dataHandler)->modList.lightMods, mods);
//
//	result.CreateNames();
//
//	for (auto& mod : mods) {
//		result.PushName(mod);
//	}
//}

void SearchForItemMods(NaturalSortedSet& itemMods) 
{
	//creating two fixed length boolean vectors to store mods with items
	int last = 0;
	auto end = (*g_dataHandler)->modList.loadedMods.entries + (*g_dataHandler)->modList.loadedMods.count;
	for (auto it = (*g_dataHandler)->modList.loadedMods.entries; it < end; ++it) {
		if ((*it)->modIndex > last)
			last = (*it)->modIndex;
	}
	std::vector<bool> esp(last + 1, false);

	last = 0;
	end = (*g_dataHandler)->modList.lightMods.entries + (*g_dataHandler)->modList.lightMods.count;
	for (auto it = (*g_dataHandler)->modList.lightMods.entries; it < end; ++it) {
		if ((*it)->lightIndex > last)
			last = (*it)->lightIndex;
	}
	std::vector<bool> esl(last + 1, false);

	tArray<TESForm*>* formArrays[SEARCH_SIZE];
	UInt32 size;
	GetFormArraysAndSize(formArrays, &size);

	BS::thread_pool pool(GetThreadCount());

	//the full form loop is the intensive part of the operation so we do the minimal amount of work which is just checking availability
	pool.parallelize_loop(size, [&formArrays, &esp, &esl](const UInt32 startIndex, const UInt32 endIndex)
	{
		TESForm** formStart;
		TESForm** formEnd;
		UInt32 current = startIndex;

		auto espIt = esp.begin();
		auto eslIt = esl.begin();
		UInt32 formId;

		while (current < endIndex) {
			if (!GetFormStartAndEnd(formArrays, &current, endIndex, &formStart, &formEnd))
				break;

			while (formStart != formEnd) {
				if (*formStart) {
					formId = (*formStart)->formID;
					if ((formId & 0xFE000000) == 0xFE000000) {
						*(eslIt + ((formId >> 12) & 0xFFF)) = true;
					}
					else {
						*(espIt + (formId >> 24)) = true;
					}
				}

				++formStart;
			}
		}
	}).wait();

	//Collect the names of available mods
	end = (*g_dataHandler)->modList.loadedMods.entries + (*g_dataHandler)->modList.loadedMods.count;
	for (auto it = (*g_dataHandler)->modList.loadedMods.entries; it < end; ++it) {
		if (esp[(*it)->modIndex])
			itemMods.insert((*it)->name);
	}

	end = (*g_dataHandler)->modList.lightMods.entries + (*g_dataHandler)->modList.lightMods.count;
	for (auto it = (*g_dataHandler)->modList.lightMods.entries; it < end; ++it) {
		if (esl[(*it)->lightIndex])
			itemMods.insert((*it)->name);
	}
}

void GetItemMods(GFxResult& result) 
{
	if (storedItemMods.empty())
		SearchForItemMods(storedItemMods);

	//if (itemModsSearchResult.list.empty())
	//	return result.SetError("No mods with addable items found");

	result.CreateNames();

	for (auto& mod : storedItemMods) {
		result.PushName(mod.c_str());
	}
}

void GetModIndexAndMask(const ModInfo* modInfo, UInt32* modIndex, UInt32* mask)
{
	if (modInfo->IsLight()) {
		*modIndex = (0xFE000000 | (modInfo->lightIndex << 12));
		*mask = 0xFFFFF000;
	}
	else {
		*modIndex = modInfo->modIndex << 24;
		*mask = 0xFF000000;
	}
}

void SearchForItemGroups(const ModInfo* modInfo)
{
	memset(availableModGroups, false, sizeof(availableModGroups));

	UInt32 modIndex;
	UInt32 mask;
	GetModIndexAndMask(modInfo, &modIndex, &mask);

	tArray<TESForm*>* formArrays[SEARCH_SIZE];
	UInt32 size;
	GetFormArraysAndSize(formArrays, &size);

	BS::thread_pool pool(GetThreadCount());

	pool.parallelize_loop(size, [&formArrays, &modIndex, &mask](const UInt32 startIndex, const UInt32 endIndex)
	{
		TESForm** formStart;
		TESForm** formEnd;
		UInt32 current = startIndex;
		int arrayIndex;

		while (current < endIndex) {
			if (!GetFormStartAndEndWithIndex(formArrays, &current, endIndex, &formStart, &formEnd, &arrayIndex))
				break;

			while (formStart != formEnd) {
				if (*formStart && (((*formStart)->formID & mask) == modIndex)) {
					availableModGroups[arrayIndex] = true;
					break;
				}	

				++formStart;
			}
		}
	}).wait();
}

void GetItemGroups(GFxResult& result, const char* modName)
{
	const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(modName);
	if (!modInfo)
		return result.SetError("Mod info was not found");

	//if mod has changed
	if (_stricmp(modName, lastSelectedMod.c_str())) 
	{
		SearchForItemGroups(modInfo);
		lastSelectedMod = modName;
	}

	result.CreateMenuItems();

	for (int i = 0; i < SEARCH_SIZE; ++i) {
		if (availableModGroups[i])
			result.PushItem(modGroupNames[i], i);
	}
}

void GetItemList(GFxResult& result, const char* modName, SInt32 groupIndex)
{
	const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(modName);
	if (!modInfo)
		return result.SetError("Mod info was not found");

	tArray<TESForm*>* formArray;
	switch (groupIndex) {
	case 0: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrARMO); break;
	case 1: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrWEAP); break;
	case 2: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrALCH); break;
	case 3: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrAMMO); break;
	case 4: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrBOOK); break;
	case 5: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrNOTE); break;
	case 6: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrKEYM); break;
	case 7: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrOMOD); break;
	case 8: formArray = reinterpret_cast<tArray<TESForm*>*>(&(*g_dataHandler)->arrMISC); break;
	default: return result.SetError("Group index was out of range");
	}

	UInt32 modIndex;
	UInt32 mask;
	GetModIndexAndMask(modInfo, &modIndex, &mask);

	FormSearchResult searchResult;

	BS::thread_pool pool(GetThreadCount());

	pool.parallelize_loop(formArray->count, [&formArray, &searchResult, &modIndex, &mask](const UInt32 startIndex, const UInt32 endIndex)
	{
		TESForm** formStart = formArray->entries + startIndex;
		TESForm** formEnd = formArray->entries + endIndex;

		while (formStart != formEnd) {
			if (*formStart && (((*formStart)->formID & mask) == modIndex))
				searchResult.Push((*formStart)->GetFullName(), (*formStart)->formID);

			formStart++;
		}
	}).wait();

	searchResult.Sort();

	result.CreateMenuItems();
	
	for (auto& kvp : searchResult.list) {
		if (kvp.first && *kvp.first)
			result.PushItem(kvp.first, kvp.second);
	}
}

typedef void (*_AddInventoryItem)(TESObjectREFR* refr, TESForm* form, ExtraDataList* filter, UInt64 unk);
RelocAddr<_AddInventoryItem> AddInventoryItem(0x3FBB00);

void AddItemFromForm(TESObjectREFR* refr, TESForm* form)
{
	ExtraDataList list;
	memset(&list, 0, sizeof(ExtraDataList));

	AddInventoryItem(refr, form, &list, 1);
}

bool RefrHasItem(TESObjectREFR* refr, TESForm* form) {
	auto inventoryList = selected.refr->inventoryList;
	if (!inventoryList)
		return false;

	BSReadLocker locker(&refr->inventoryList->inventoryLock);

	auto items = &inventoryList->items;
	auto end = items->entries + items->count;
	for (auto it = items->entries; it != end; ++it) {
		if (it->form == form)
			return true;
	}

	return false;
}

typedef void (*_QueueActorUpdateModel)(UInt64 taskQueueInterface, Actor* actor, UInt16 flags, float unk);
RelocAddr<_QueueActorUpdateModel> QueueActorUpdateModel(0xD59AA0);

RelocPtr<UInt64> taskQueueInterface(0x5AC64F0);

void UpdateActorModel(Actor* actor, bool force)
{
	auto freeState = GetFreeCameraState();

	//If in TFC mode and the player is the target, or game is paused, force update the 3D model
	if (force || (actor == *g_player && freeState) || GetGamePaused())
		QueueActorUpdateModel(*taskQueueInterface, actor, 0x11, 0.0f);
}

class BGSObjectInstance {
public:
	TESForm* form;
	UInt64* instanceDataPtr;
};

typedef bool (*_ActorEquipManagerEquipObject)(UInt64 manager, TESObjectREFR* actor, BGSObjectInstance* instance, UInt32 stackId,
	UInt32 unk, BGSEquipSlot* slot, bool queue, bool force, bool sound, bool apply, bool locked);
RelocAddr<_ActorEquipManagerEquipObject> ActorEquipManagerEquipObject(0xE1BCD0);

RelocPtr<UInt64> actorEquipManagerInstance(0x59D75C8);

void EquipItemFromForm(TESObjectREFR* refr, TESForm* form)
{
	BGSObjectInstance instance { form, nullptr };

	ActorEquipManagerEquipObject(*actorEquipManagerInstance, refr, &instance, 0, 1, nullptr, false, true, true, true, false);
}

typedef bool (*_ActorEquipManagerUnequipObject)(UInt64 manager, TESObjectREFR* actor, BGSObjectInstance* instance, 
	UInt32 unk, BGSEquipSlot* slot, int unk2, bool b1, bool b2, bool b3, bool b4, UInt64 unk3);
RelocAddr<_ActorEquipManagerUnequipObject> ActorEquipManagerUnequipObject(0xE1C0B0);

void UnequipItemFromForm(TESObjectREFR* refr, TESForm* form) 
{
	BGSObjectInstance instance{ form, nullptr };

	ActorEquipManagerUnequipObject(*actorEquipManagerInstance, refr, &instance, 1, nullptr, -1, false, true, true, false, 0);
}

//typedef bool (*_InventoryItemIsEquippedInternal)(TESObjectREFR* refr, TESForm* form);
//RelocAddr<_InventoryItemIsEquippedInternal> InventoryItemIsEquippedInternal(0x1A6C20);

bool IsFormEquipped(TESObjectREFR* refr, TESForm* form) {
	Actor* actor = (Actor*)selected.refr;
	if (!actor->equipData)
		return false;

	for (auto it = actor->equipData->slots; it != actor->equipData->slots + ActorEquipData::kMaxSlots; ++it) {
		if (it->item && it->item->formID == form->formID)
			return true;
	}

	return false;
}

void AddItem(GFxResult& result, UInt32 formId, bool equip) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find form id for item");

	Actor* actor = (Actor*)selected.refr;

	//Add item
	if (!equip) {
		//PapyrusAddItem(selected.refr, form, 1, true);
		AddItemFromForm(actor, form);
		std::string notif = std::string(form->GetFullName()) + " added to inventory";
		samManager.ShowNotification(notif.c_str(), false);
	}

	//Equip item
	else {
		
		//Return error on game paused
		//if (GetGamePaused())
		//	return result.SetError("Cannot equip/unequip while the game is paused");

		//If equipped, unequip instead
		if (IsFormEquipped(actor, form)) {
			UnequipItemFromForm(actor, form);
			UpdateActorModel(actor, false);
		}
		else {
			if (!RefrHasItem(actor, form))
				AddItemFromForm(actor, form);

			EquipItemFromForm(actor, form);
			UpdateActorModel(actor, false);
		}
	}
}

void GetLastSearchResult(GFxResult& result) {
	result.CreateMenuItems();

	for (auto& kvp : substringSearchResult.list) {
		if (kvp.first && *kvp.first)
			result.PushItem(kvp.first, kvp.second);
	}
}

void SearchFormsForSubstring(FormSearchResult& searchResult, const char* search) {

	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	tArray<TESForm*>* formArrays[SEARCH_SIZE];
	UInt32 size;
	GetFormArraysAndSize(formArrays, &size);

	BS::thread_pool pool(GetThreadCount());

	//BS::timer timer;
	//timer.start();

	pool.parallelize_loop(size, [&formArrays, &lowered, &searchResult](const UInt32 startIndex, const UInt32 endIndex)
	{
		TESForm** start;
		TESForm** end;
		UInt32 current = startIndex;
		while (current < endIndex) {
			if (!GetFormStartAndEnd(formArrays, &current, endIndex, &start, &end))
				break;

			while (start != end) {
				if (*start && HasInsensitiveSubstring((*start)->GetFullName(), lowered))
					searchResult.Push((*start)->GetFullName(), (*start)->formID);
				++start;
			}
		}
	}).wait();

	//timer.stop();
	//_Log("Substring search operation completed in ms: ", timer.ms());

	searchResult.Sort();
}

void SearchItems(GFxResult& result, const char* search) 
{
	if (!search || !*search)
		return result.SetError("No search term found");

	int len = strlen(search);

	if (len < 2)
		return result.SetError("Search term must be longer than 1 character");

	if (len >= MAX_PATH - 1)
		return result.SetError("Search term is too big! 0_0");

	substringSearchResult.Clear();

	SearchFormsForSubstring(substringSearchResult, search);
	
	if (substringSearchResult.list.empty())
		return result.SetError("Search returned no results");
}


void GetMatSwapEquipment(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	Actor* actor = (Actor*)selected.refr;
	if (!actor->equipData)
		return result.SetError("Could not find actor equip data");

	NaturalSortedUInt32Map map;

	for (auto it = actor->equipData->slots; it != actor->equipData->slots + ActorEquipData::kMaxSlots; ++it) {
		if (it->item && it->item->formType == kFormType_ARMO)
			map.emplace(it->item->GetFullName(), it->item->formID);
	}

	result.CreateMenuItems();

	for (auto& kvp : map) {
		if (kvp.first && *kvp.first)
			result.PushItem(kvp.first, kvp.second);
	}
}

BGSInventoryItem* GetEquippedFromForm(TESObjectREFR* refr, UInt32 formId) {
	auto inventoryList = selected.refr->inventoryList;
	if (!inventoryList)
		return nullptr;

	BSReadLocker locker(&inventoryList->inventoryLock);

	auto items = &inventoryList->items;
	auto end = items->entries + items->count;
	for (auto it = items->entries; it != end; ++it) {
		if (it->form->formID == formId)
			return it;
	}

	return nullptr;
}

BGSMod::Container::Data* GetModContainerMatSwapData(BGSMod::Attachment::Mod* mod) {
	auto end = mod->modContainer.data + (mod->modContainer.dataSize >> 4);
	for (auto it = mod->modContainer.data; it != end; ++it) {
		if (it->target == BGSMod::Container::kArmorTarget_pwMaterialSwaps)
			return it;
	}

	return nullptr;
}

typedef bool (*_CanBeUsedOnForm)(BGSMod::Attachment::Mod* mod, TESForm* form, ExtraDataList* extraData);
RelocAddr<_CanBeUsedOnForm> CanBeUsedOnForm(0x2F1B70);

void GetMatSwaps(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find form id");

	FormSearchResult searchResult;
	auto& omods = (*g_dataHandler)->arrOMOD;
	auto item = GetEquippedFromForm(selected.refr, formId);
	if (!item)
		return result.SetError("Could not find equipped item");

	auto extraData = item->stack->extraData;

	BS::thread_pool pool(GetThreadCount());

	pool.parallelize_loop(omods.count, [&](const UInt32 startIndex, const UInt32 endIndex)
	{
		auto omodEnd = omods.entries + endIndex;
		for (auto omodIt = omods.entries + startIndex; omodIt != omodEnd; ++omodIt) {
			if (*omodIt && (*omodIt)->targetType == BGSMod::Attachment::Mod::kTargetType_Armor && CanBeUsedOnForm(*omodIt, form, extraData)) {
				auto containerData = GetModContainerMatSwapData(*omodIt);
				if (containerData)
					searchResult.Push((*omodIt)->GetFullName(), (*omodIt)->formID);
			}
		}
	}).wait();

	searchResult.Sort();

	result.CreateMenuItems();

	for (auto& kvp : searchResult.list) {
		if (kvp.first && *kvp.first)
			result.PushItem(kvp.first, kvp.second);
	}
}

struct CheckStackIdFunctor {
	UInt64 vfTable;
	UInt64 unk08;
};

struct ModifyModDataFunctor {
	UInt64 vfTable;
	UInt64 unk08;
	TESForm* modForm;
	UInt64 unk18;
	UInt64* unk20;
	bool unk28;
	bool unk29;
	bool unk2A;
	UInt8 pad2B;
	UInt32 pad2C;
};

typedef void (*_FindAndWriteStackDataForInventoryItem)(TESObjectREFR* refr, TESForm* form, CheckStackIdFunctor* comparer, ModifyModDataFunctor* writer, UInt64 callback, bool unk);
RelocAddr<_FindAndWriteStackDataForInventoryItem> FindAndWriteStackDataForInventoryItem(0x3FB430);

RelocAddr<UInt64> checkStackIdFunctorVftable(0x2C5C928);
RelocAddr<UInt64> modifyModDataFunctorVftable(0x2D11060);
RelocAddr<UInt64> standardObjectCompareCallback(0x42F280);

typedef void(*_PostModifyInventoryItem)(TESObjectREFR* refr, TESForm* form, bool unk1);
RelocAddr<_PostModifyInventoryItem> PostModifyInventoryItem(0x1403730);

typedef UInt32 (*_GetInventoryObjectCount)(TESObjectREFR* refr, TESForm* form);
RelocAddr<_GetInventoryObjectCount> GetInventoryObjectCount(0x3FB480);

typedef void (*_RemoveNonRefrItem)(TESObjectREFR* refr, TESForm* form, UInt64 count, bool silent, UInt64 unk1, UInt64 unk2, UInt32 handle, VirtualMachine* vm);
RelocAddr<_RemoveNonRefrItem> RemoveNonRefrItem(0x13FCB30);

void ApplyMatSwap(GFxResult& result, UInt32 modId, UInt32 equipId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* equipForm = LookupFormByID(equipId);
	if (!equipForm)
		return result.SetError("Could not find equipment id");
	
	TESForm* modForm = LookupFormByID(modId);
	if (!modForm)
		return result.SetError("Could not find mod id");

	UInt32 count = GetInventoryObjectCount(selected.refr, equipForm);
	if (count == 0)
		return result.SetError("Could not find equipment");

	//TODO the internal mat swap functions i'm using only work when you have no duplicates so we're removing extras for now
	if (count > 1) {
		VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
		RemoveNonRefrItem(selected.refr, equipForm, count - 1, true, 0, 0, 0, vm);

		//Item might get unequipped during removal so reequip if necessary
		EquipItemFromForm(selected.refr, equipForm);
	}

	CheckStackIdFunctor comparer{ 
		checkStackIdFunctorVftable, 
		0 
	};

	UInt64 out = 1;

	ModifyModDataFunctor writer{
		modifyModDataFunctorVftable,
		1,
		modForm,
		0,
		&out,
		false,
		true,
		false,
		0,
		0
	};

	FindAndWriteStackDataForInventoryItem(selected.refr, equipForm, &comparer, &writer, standardObjectCompareCallback, false);
	PostModifyInventoryItem(selected.refr, equipForm, false);
	UpdateActorModel((Actor*)selected.refr, true);
}

void GetEquipment(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	Actor* actor = (Actor*)selected.refr;
	if (!actor->equipData)
		return result.SetError("Console target is not an actor");

	NaturalSortedUInt32Map map;

	for (int i = 0; i < ActorEquipData::kMaxSlots; ++i) {
		//ignore pipboy
		if (actor->equipData->slots[i].item && i != 31) { 
			const char* name = actor->equipData->slots[i].item->GetFullName();
			if (name && *name) {
				map.emplace(name, actor->equipData->slots[i].item->formID);
			}
		}
	}

	result.CreateMenuItems();

	for (auto& kvp : map) {
		result.PushItem(kvp.first, kvp.second);
	}
}

void RemoveEquipment(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find form id for item");

	auto actor = (Actor*)selected.refr;

	//if (GetGamePaused())
	//	return result.SetError("Cannot remove while game is paused");

	UnequipItemFromForm(selected.refr, form);
	UpdateActorModel(actor, false);
}

void RemoveAllEquipment(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	Actor* actor = (Actor*)selected.refr;
	if (!actor->equipData)
		return result.SetError("Console target is not an actor");

	//if (GetGamePaused())
	//	return result.SetError("Cannot remove while game is paused");

	//collect a set to prevent duplicate removes
	std::unordered_set<UInt32> equipSet;
	for (int i = 0; i < ActorEquipData::kMaxSlots; ++i) {
		//ignore pipboy
		if (i != 31) {
			auto& slot = actor->equipData->slots[i];
			if (slot.item)
				equipSet.insert(slot.item->formID);
		}
	}

	bool updated = false;

	for (auto& formId : equipSet) {
		TESForm* form = LookupFormByID(formId);
		if (form) {
			UnequipItemFromForm(selected.refr, form);
			updated = true;
		}
	}

	if (updated) {
		UpdateActorModel(actor, false);
	}
}