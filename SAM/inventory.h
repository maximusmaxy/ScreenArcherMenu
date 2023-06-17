#pragma once

#include "f4se/GameReferences.h"

#include "gfx.h"

void OpenActorContainer(GFxResult& result);

void GetItemMods(GFxResult& result);
void GetItemGroups(GFxResult& result, const char* modInfoStr);
void GetItemList(GFxResult& result, const char* modInfoStr, SInt32 groupIndex);
void AddItem(GFxResult& result, UInt32 formId, bool equip);

void GetLastSearchResult(GFxResult& result);
void SearchItems(GFxResult& result, const char* search);

void GetMatSwapEquipment(GFxResult& result);
void GetMatSwaps(GFxResult& result, UInt32 formId);
void ApplyMatSwap(GFxResult& result, UInt32 modId, UInt32 formId);

void GetEquipment(GFxResult& result);
void RemoveEquipment(GFxResult& result, UInt32 formId);
void RemoveAllEquipment(GFxResult& result);

void GetStaticMods(GFxResult& result);
void GetStaticGroups(GFxResult& result, const char* modInfoStr);
void GetStaticItems(GFxResult& result, const char* modName, SInt32 groupIndex);
void GetLastSearchResultStatic(GFxResult& result);
void SearchStatics(GFxResult& result, const char* search);
void PlaceAtSelected(GFxResult& result, UInt32 formId);