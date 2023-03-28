#pragma once

#include "f4se/GameReferences.h"

#include "gfx.h"

#include <mutex>

class FormSearchResult {
private:
	std::mutex mutex;
public:
	std::vector<std::pair<const char*, UInt32>> list;

	void Push(const char* name, UInt32 formId);
	void Sort();
	void Clear();
};

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

void ShowLooksMenu(GFxResult& result);