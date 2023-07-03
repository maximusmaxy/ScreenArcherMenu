#include "inventory.h"

#include "f4se_common/Relocation.h"
#include "f4se/GameForms.h"
#include "f4se/GameData.h"
#include "f4se/GameObjects.h"
#include "f4se/GameExtraData.h"
#include "f4se/GameRTTI.h"
#include "f4se/PapyrusVM.h"
#include "f4se/NiTypes.h"

#include "SAF/util.h"
#include "SAF/conversions.h"

#include "sam.h"
#include "constants.h"
#include "idle.h"
#include "types.h"
#include "papyrus.h"
#include "types.h"
#include "camera.h"
#include "positioning.h"
#include "strnatcmp.h"
#include "forms.h"
#include "papyrus.h"
#include "functions.h"
#include "esp.h"
#include "positioning.h"

#include <thread>
#include <fstream>
#include <filesystem>
#include <array>
#include <ranges>
#include <algorithm>
#include <execution>
#include <chrono>

FormSearchResult itemSearchResult;
FormSearchResult staticSearchResult;

std::vector<const char*> storedItemMods;
std::vector<const char*> storedStaticMods;
std::string lastSelectedMod;

EdidList staticFileItems;
std::pair<const char*, UInt32> lastStaticFile{ nullptr, 0 };

#define ITEMS_SIZE 9
#define STATICS_SIZE 8
bool availableModGroups[ITEMS_SIZE];
bool availableStaticsGroups[STATICS_SIZE];

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

const char* staticGroupNames[] = {
	"Door",
	"Flora",
	"Furniture",
	"Light",
	"Moveable Static",
	"NPC",
	"Static Collection",
	"Static"
};

typedef bool (*_OpenActorContainerInternal)(TESObjectREFR* refr, UInt32 unk2, bool unk3);
RelocAddr<_OpenActorContainerInternal> OpenActorContainerInternal(0x1264F40);

void OpenActorContainer(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	OpenActorContainerInternal(selected.refr, 3, false);
}

FormsSpan GetItemsSpans() {
	return {
		MakeFormsSpan((*g_dataHandler)->arrARMO),
		MakeFormsSpan((*g_dataHandler)->arrWEAP),
		MakeFormsSpan((*g_dataHandler)->arrALCH),
		MakeFormsSpan((*g_dataHandler)->arrAMMO),
		MakeFormsSpan((*g_dataHandler)->arrBOOK),
		MakeFormsSpan((*g_dataHandler)->arrNOTE),
		MakeFormsSpan((*g_dataHandler)->arrKEYM),
		MakeFormsSpan((*g_dataHandler)->arrOMOD),
		MakeFormsSpan((*g_dataHandler)->arrMISC),
	};
}

FormsSpan GetStaticsSpans() {
	return {
		MakeFormsSpan((*g_dataHandler)->arrDOOR),
		MakeFormsSpan((*g_dataHandler)->arrFLOR),
		MakeFormsSpan((*g_dataHandler)->arrFURN),
		MakeFormsSpan((*g_dataHandler)->arrLIGH),
		MakeFormsSpan((*g_dataHandler)->arrMSTT),
		MakeFormsSpan((*g_dataHandler)->arrNPC_),
		MakeFormsSpan((*g_dataHandler)->arrSCOL),
		MakeFormsSpan((*g_dataHandler)->arrSTAT),
	};
}

void GetItemMods(GFxResult& result) {
	if (storedItemMods.empty())
		SearchFormsSpanForMods(GetItemsSpans(), storedItemMods);
	AddFormattedModsToResult(result, storedItemMods);
}

void SearchForAvailableGroups(const ModInfo* modInfo, const FormsSpan& spans, bool* availableGroups, size_t len)
{
	memset(availableGroups, false, sizeof(availableGroups));

	UInt32 modIndex;
	UInt32 mask;
	GetModIndexAndMask(modInfo, &modIndex, &mask);

	for (int arrIndex = 0; arrIndex < len; ++arrIndex) {
		std::for_each(std::execution::par, spans[arrIndex].begin(), spans[arrIndex].end(),
			[&arrIndex, &modIndex, &mask, &availableGroups](TESForm* form) {
			if (form && ((form->formID & mask) == modIndex))
				availableGroups[arrIndex] = true;
		});
	}
}

void GetItemGroups(GFxResult& result, const char* modName)
{
	const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(modName);
	if (!modInfo)
		return result.SetError("Mod info was not found");

	//if mod has changed
	if (_stricmp(modName, lastSelectedMod.c_str()))
	{
		SearchForAvailableGroups(modInfo, GetItemsSpans(), availableModGroups, ITEMS_SIZE);
		lastSelectedMod = modName;
	}

	result.CreateMenuItems();

	for (SInt32 i = 0; i < ITEMS_SIZE; ++i) {
		if (availableModGroups[i])
			result.PushItem(modGroupNames[i], i);
	}
}

void GetItemList(GFxResult& result, const char* modName, SInt32 groupIndex)
{
	const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(modName);
	if (!modInfo)
		return result.SetError("Mod info was not found");

	std::span<TESForm*> formSpan;
	switch (groupIndex) {
	case 0: formSpan = MakeFormsSpan((*g_dataHandler)->arrARMO); break;
	case 1: formSpan = MakeFormsSpan((*g_dataHandler)->arrWEAP); break;
	case 2: formSpan = MakeFormsSpan((*g_dataHandler)->arrALCH); break;
	case 3: formSpan = MakeFormsSpan((*g_dataHandler)->arrAMMO); break;
	case 4: formSpan = MakeFormsSpan((*g_dataHandler)->arrBOOK); break;
	case 5: formSpan = MakeFormsSpan((*g_dataHandler)->arrNOTE); break;
	case 6: formSpan = MakeFormsSpan((*g_dataHandler)->arrKEYM); break;
	case 7: formSpan = MakeFormsSpan((*g_dataHandler)->arrOMOD); break;
	case 8: formSpan = MakeFormsSpan((*g_dataHandler)->arrMISC); break;
	default: return result.SetError("Group index was out of range");
	}

	FormSearchResult searchResult;
	SearchModForFullnameForms(modInfo, formSpan, searchResult);


	result.CreateMenuItems();
	for (auto& kvp : searchResult.result) {
		result.PushItem(kvp.first, kvp.second);
	}
}

typedef void (*_AddInventoryItem)(TESObjectREFR* refr, TESForm* form, UInt64* extraDataList, UInt64 unk1, UInt64 unk2, UInt64 unk3, UInt64 unk4);
RelocAddr<_AddInventoryItem> AddInventoryItem(0x3FBB00);

void AddItemFromForm(TESObjectREFR* refr, TESForm* form)
{
	UInt64 extraData = 0;

	AddInventoryItem(refr, form, &extraData, 1, 0, 0, 0);
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
	BGSObjectInstance instance{ form, nullptr };

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

	for (auto& kvp : itemSearchResult.result) {
		result.PushItem(kvp.first, kvp.second);
	}
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

	itemSearchResult.result.clear();
	SearchFormsForSubstring(itemSearchResult, GetItemsSpans(), search);

	if (itemSearchResult.result.empty())
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
	auto omods = std::span((*g_dataHandler)->arrOMOD.entries, (*g_dataHandler)->arrOMOD.count);
	auto item = GetEquippedFromForm(selected.refr, formId);
	if (!item)
		return result.SetError("Could not find equipped item");
	auto extraData = item->stack->extraData;

	std::for_each(std::execution::par, omods.begin(), omods.end(), [&](BGSMod::Attachment::Mod* mod) {
		if (mod && mod->targetType == BGSMod::Attachment::Mod::kTargetType_Armor && CanBeUsedOnForm(mod, form, extraData)) {
			auto containerData = GetModContainerMatSwapData(mod);
			if (containerData) {
				auto fullname = mod->GetFullName();
				if (fullname && *fullname)
					searchResult.Push(fullname, mod->formID);
			}	
		}
	});

	searchResult.Sort();

	result.CreateMenuItems();

	for (auto& kvp : searchResult.result) {
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

typedef UInt32(*_GetInventoryObjectCount)(TESObjectREFR* refr, TESForm* form);
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

void GetStaticMods(GFxResult& result) {
	if (storedStaticMods.empty())
		SearchFormsSpanForMods(GetStaticsSpans(), storedStaticMods);
	AddFormattedModsToResult(result, storedStaticMods);
}

void GetStaticGroups(GFxResult& result, const char* modName)
{
	const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(modName);
	if (!modInfo)
		return result.SetError("Mod info was not found");

	//if mod has changed
	if (_stricmp(modName, lastSelectedMod.c_str()))
	{
		SearchForAvailableGroups(modInfo, GetStaticsSpans(), availableStaticsGroups, STATICS_SIZE);
		lastSelectedMod = modName;
	}

	result.CreateMenuItems();

	for (SInt32 i = 0; i < STATICS_SIZE; ++i) {
		if (availableStaticsGroups[i])
			result.PushItem(staticGroupNames[i], i);
	}
}

void GetStaticItems(GFxResult& result, const char* modName, SInt32 groupIndex)
{
	const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(modName);
	if (!modInfo)
		return result.SetError("Mod info was not found");

	//std::span<TESForm*> formSpan;
	//switch (groupIndex) {
	//case 0: formSpan = MakeFormsSpan((*g_dataHandler)->arrDOOR); break;
	//case 1: formSpan = MakeFormsSpan((*g_dataHandler)->arrFLOR); break;
	//case 2: formSpan = MakeFormsSpan((*g_dataHandler)->arrFURN); break;
	//case 3: formSpan = MakeFormsSpan((*g_dataHandler)->arrLIGH); break;
	//case 4: formSpan = MakeFormsSpan((*g_dataHandler)->arrMSTT); break;
	//case 5: formSpan = MakeFormsSpan((*g_dataHandler)->arrNPC_); break;
	//case 6: formSpan = MakeFormsSpan((*g_dataHandler)->arrSCOL); break;
	//case 7: formSpan = MakeFormsSpan((*g_dataHandler)->arrSTAT); break;
	//default: return result.SetError("Group index was out of range");
	//}

	//FormSearchResult searchResult;
	//SearchModForFullnameForms(modInfo, formSpan, searchResult);

	auto pair = std::make_pair(modName, (UInt32)groupIndex);
	if (lastStaticFile != pair) {
		lastStaticFile = pair;

		UInt32 signature;
		switch (groupIndex) {
		case 0: signature = ESP::Sig("DOOR"); break;
		case 1:	signature = ESP::Sig("FLOR"); break;
		case 2:	signature = ESP::Sig("FURN"); break;
		case 3:	signature = ESP::Sig("LIGH"); break;
		case 4:	signature = ESP::Sig("MSTT"); break;
		case 5:	signature = ESP::Sig("NPC_"); break;
		case 6:	signature = ESP::Sig("SCOL"); break;
		case 7:	signature = ESP::Sig("STAT"); break;
		default: return result.SetError("Group index was out of range");
		}

		staticFileItems.clear();
		FindEdidsFromFile(result, modInfo, signature, staticFileItems);
		if (result.type == GFxResult::Error)
			return;
	}

	result.CreateMenuItems();
	for (auto& kvp : staticFileItems) {
		result.PushItem(kvp.first.c_str(), kvp.second);
	}
}

void GetLastSearchResultStatic(GFxResult& result) {
	result.CreateMenuItems();

	for (auto& kvp : staticSearchResult.result) {
		result.PushItem(kvp.first, kvp.second);
	}
}

void SearchStatics(GFxResult& result, const char* search) {
	if (!search || !*search)
		return result.SetError("No search term found");

	int len = strlen(search);

	if (len < 2)
		return result.SetError("Search term must be longer than 1 character");

	if (len >= MAX_PATH - 1)
		return result.SetError("Search term is too big! 0_0");

	staticSearchResult.result.clear();

	SearchFormsForSubstring(staticSearchResult, GetStaticsSpans(), search);

	if (staticSearchResult.result.empty())
		return result.SetError("Search returned no results");
}


struct StaticAction {
	enum Type {
		Add = 0,
		Remove,
		Swap
	};

	UInt32 refr;
	UInt32 formId;
	Type type;
	NiTransform transform;
};

struct PlacedStaticManager {
	SInt32 index;
	std::vector<StaticAction> actions;
	TESObjectCELL* lastCell;

	PlacedStaticManager() : index(-1), lastCell(nullptr) {}

	auto SetCell(TESObjectCELL* cell) {
		index = -1;
		actions.clear();
		lastCell = cell;
	}

	void UpdateCell() {
		auto player = (*g_player);
		if (!player)
			return SetCell(nullptr);

		auto cell = player->parentCell;
		if (cell != lastCell)
			return SetCell(cell);
	}

	TESObjectREFR* GetRefr(UInt32 refrId) {
		auto form = LookupFormByID(refrId);
		if (!form)
			return nullptr;

		auto refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
		if (!refr)
			return nullptr;

		return refr;
	}

	void SetTransform(TESObjectREFR* refr, StaticAction& action) {
		auto root = refr->GetObjectRootNode();
		if (root)
			action.transform = root->m_worldTransform;
		else
			action.transform = SAF::TransformIdentity();
	}

	bool PlaceFromAction(StaticAction& action) {
		auto form = LookupFormByID(action.formId);
		if (!form)
			return false;

		TESObjectREFR* out = nullptr;
		UInt32* handle = PlaceAtMeInternal(&out, *g_player, form, 1, 0, 0, false, false);
		if (!handle)
			return false;

		NiPointer<TESObjectREFR> refr;
		GetREFRFromHandle(handle, refr);
		if (!refr)
			return false;

		action.refr = refr->formID;
		SetREFRTransform(refr, action.transform);

		return true;
	}

	bool UpdateActionTransform(StaticAction& action) {
		if (SAF::TransformIsDefault(action.transform)) {
			auto playerRoot = (*g_player)->GetActorRootNode(false);
			if (!playerRoot)
				return false;
			const NiTransform mod{
				SAF::MatrixIdentity(),
				{0, 100.0f, 0},
				1.0f
			};
			action.transform = SAF::MultiplyNiTransform(playerRoot->m_worldTransform, mod);
		}

		return true;
	}

	TESObjectREFR* Place(UInt32 formId) {
		auto form = LookupFormByID(formId);
		if (!form)
			return nullptr;

		auto root = selected.refr->GetActorRootNode(false);
		if (!root)
			return nullptr;

		TESObjectREFR* out = nullptr;
		UInt32* handle = PlaceAtMeInternal(&out, selected.refr, form, 1, 0, 0, false, false);
		if (!handle)
			return nullptr;

		NiPointer<TESObjectREFR> refr;
		GetREFRFromHandle(handle, refr);
		if (!refr)
			return nullptr;

		const NiTransform mod{
			SAF::MatrixIdentity(),
			{0, 100.0f, 0},
			1.0f
		};
		const auto transform = SAF::MultiplyNiTransform(root->m_worldTransform, mod);
		SetREFRTransform(refr, transform);

		return refr;
	}

	void Push(TESObjectREFR* refr, bool add) {
		UpdateCell();
		actions.resize(++index, {});
		StaticAction::Type type = add ? StaticAction::Add : StaticAction::Swap;
		
		if (index <= 0) {
			type = StaticAction::Add;
		}
		else {
			auto& prevAction = actions.at(index - 1);
			if (prevAction.type == StaticAction::Remove)
				type = StaticAction::Add;

			if (type == StaticAction::Swap) {
				auto refr = GetRefr(prevAction.refr);
				if (refr) {
					SetTransform(refr, prevAction);
					PapyrusDelete(refr);
				}
			}
		}

		actions.push_back({ refr->formID, refr->baseForm->formID, type });
	}

	void Undo(GFxResult& result) {
		UpdateCell();
		if (index < 0)
			return result.SetError("");

		auto& action = actions.at(index--);

		switch (action.type) {
		case StaticAction::Add:
		{
			auto refr = GetRefr(action.refr);
			if (!refr)
				return result.SetError("Unable to find refr");

			SetTransform(refr, action);
			PapyrusDelete(refr);
			break;
		}
		case StaticAction::Swap:
		{
			auto refr = GetRefr(action.refr);
			if (!refr)
				return result.SetError("Unable to find refr");

			SetTransform(refr, action);
			PapyrusDelete(refr);

			if (index >= 0) {
				auto& prevAction = actions.at(index);
				if (UpdateActionTransform(prevAction)) {
					PlaceFromAction(prevAction);
				}
			}
			break;
		}
		case StaticAction::Remove:
		{
			auto form = LookupFormByID(action.formId);
			if (!form)
				return result.SetError("Unable to find form");

			auto newRefr = Place(action.formId);
			if (index >= 0) {
				auto& prevAction = actions.at(index);
				prevAction.refr = newRefr->formID;
			}
			break;
		}
		}
	}

	void Redo(GFxResult& result) {
		UpdateCell();
		if (index + 1 >= (SInt32)actions.size())
			return result.SetError("");

		auto& action = actions.at(++index);
		switch (action.type) {
		case StaticAction::Add:
		{
			if (!UpdateActionTransform(action))
				return result.SetError("Failed to find a location to spawn object");

			if (!PlaceFromAction(action))
				return result.SetError("Failed to spawn object");
			break;
		}
		case StaticAction::Swap:
		{
			if (index > 0) {
				auto& prevAction = actions.at(index - 1);
				auto refr = GetRefr(prevAction.refr);
				if (refr) {
					SetTransform(refr, prevAction);
					PapyrusDelete(refr);
				}
			}
			
			if (!UpdateActionTransform(action))
				return result.SetError("Failed to find a location to spawn object");

			if (!PlaceFromAction(action))
				return result.SetError("Failed to spawn object");
			break;
		}
		case StaticAction::Remove:
		{
			if (index > 0) {
				auto& prevAction = actions.at(index - 1);
				auto refr = GetRefr(prevAction.refr);
				if (refr) {
					SetTransform(refr, prevAction);
					PapyrusDelete(refr);
				}
			}
			break;
		}
		}
	}

	bool Delete() {
		UpdateCell();
		if (index < 0)
			return false;

		auto& action = actions.at(index);
		if (action.type == StaticAction::Remove)
			return false;

		auto refr = GetRefr(action.refr);
		if (!refr)
			return false;

		StaticAction newAction{ 0, action.formId, StaticAction::Remove };
		SetTransform(refr, newAction);
		PapyrusDelete(refr);

		actions.resize(++index, {});
		actions.emplace_back(newAction);
		return true;
	}

	TESObjectREFR* GetCurrentRefr() {
		if (index < 0)
			return nullptr;

		auto& action = actions.at(index);
		if (action.type == StaticAction::Remove)
			return nullptr;

		return GetRefr(action.refr);
	}
};

PlacedStaticManager placedStaticManager;

GFxReq undoPlacedStatic("UndoPlacedStatic", [](auto& result, auto args) {
	placedStaticManager.Undo(result);
});
GFxReq redoPlacedStatic("RedoPlacedStatic", [](auto& result, auto args) {
	placedStaticManager.Redo(result);
});

void PlaceAtSelected(GFxResult& result, UInt32 formId, bool add) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto refr = placedStaticManager.Place(formId);
	if (!refr)
		return result.SetError("Failed to spawn object");

	placedStaticManager.Push(refr, add);
}
GFxReq placeStatic("PlaceStatic", [](auto& result, auto args) {
	PlaceAtSelected(result, args->args[1].GetUInt(), args->args[2].GetBool());
});

void DeleteStatic(GFxResult& result) {
	if (!placedStaticManager.Delete())
		return result.SetError("");
}
GFxReq deleteStatic("DeleteStatic", [](auto& result, auto args) {
	DeleteStatic(result);
});

GFxReq rotateStatic("RotateStatic", [](auto& result, auto args) {
	auto refr = placedStaticManager.GetCurrentRefr();
	if (!refr)
		return;

	AdjustRefrPosition(refr, kAdjustRotationZ, args->args[0].GetNumber());
});

void RevertInventory() {
	placedStaticManager.SetCell(nullptr);
}