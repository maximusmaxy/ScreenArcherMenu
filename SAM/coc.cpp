#include "coc.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameMenus.h"
#include "gfx.h"
#include "forms.h"
#include "constants.h"
#include "sam.h"

#include <span>
#include <algorithm>
#include <execution>

FormSearchResult teleportSearchResult;

typedef void(*_CenterOnCellInternal)(TESObjectREFR* player, const char* edid, TESObjectCELL* cell);
RelocAddr<_CenterOnCellInternal> CenterOnCellInternal(0xE9A990);

bool CenterOnCell(const char* edid, TESObjectCELL* cell) {
	if (!g_player)
		return false;

	CenterOnCellInternal(*g_player, edid, cell);
	return true;
}

void GetCellMods(GFxResult& result) {
	auto& cells = (*g_dataHandler)->cellList;
	std::span<TESObjectCELL*> span(cells.m_data, cells.m_emptyRunStart);
	std::vector<const char*> cellMods;
	SearchSpanForMods(span, cellMods);
	AddFormattedModsToResult(result, cellMods);
}

void GetCells(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	auto& cells = (*g_dataHandler)->cellList;
	std::span<TESObjectCELL*> span(cells.m_data, cells.m_emptyRunStart);
	FormSearchResult searchResult;
	SearchModForEdidForms(info, span, searchResult);

	result.CreateMenuItems();
	for (auto& pair : searchResult.result) {
		result.PushItem(pair.first, pair.second);
	}
}

void SetCell(GFxResult& result, UInt32 formId) {
	auto form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Failed to find cell form");

	auto cell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
	if (!cell)
		return result.SetError("Form was not a cell");

	//if (!CenterOnCell(nullptr, cell))
	//	return result.SetError("Could not find player for teleport");
	samManager.storedCoc = cell;
	samManager.OpenOrCloseMenu(nullptr);
}

void GetWorldspaceMods(GFxResult& result) {
	auto& worldspaces = (*g_dataHandler)->arrWRLD;
	std::span<TESForm*> span(worldspaces.entries, worldspaces.count);
	std::vector<const char*> worldspaceMods;
	SearchSpanForMods(span, worldspaceMods);
	AddFormattedModsToResult(result, worldspaceMods);
}

void GetWorldspacesFromMod(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	auto& worldspaces = (*g_dataHandler)->arrWRLD;
	std::span<TESForm*> span(worldspaces.entries, worldspaces.count);
	FormSearchResult searchResult;
	SearchModForEdidForms(info, span, searchResult);

	result.CreateMenuItems();
	for (auto& pair : searchResult.result) {
		result.PushItem(pair.first, pair.second);
	}
}

RelocAddr<const char*> aWilderness(0x2C870F0);

void GetWorldspaceCells(GFxResult& result, UInt32 formId) {
	auto form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Failed to find worldspace form");

	auto worldspace = DYNAMIC_CAST(form, TESForm, TESWorldSpace);
	if (!worldspace)
		return result.SetError("Form was not a worldspace");

	const char* wilderness = aWilderness;
	result.CreateMenuItems();

	FormSearchResult searchResult;
	worldspace->cells.ForEach([&searchResult, wilderness](WorldCellItem* item) {
		if (item->cell) {
			auto edid = item->cell->GetEditorID();
			if (edid && *edid && edid != wilderness)
				searchResult.Push(edid, item->cell->formID);
		}
		return true;
	});

	searchResult.Sort();

	result.CreateMenuItems();
	for (auto& pair : searchResult.result) {
		result.PushItem(pair.first, pair.second);
	}
}

void GetLastSearchResultCell(GFxResult& result) {
	result.CreateMenuItems();

	for (auto& kvp : teleportSearchResult.result) {
		result.PushItem(kvp.first, kvp.second);
	}
}

void SearchCells(GFxResult& result, const char* search) {
	if (!search || !*search)
		return result.SetError("No search term found");

	int len = strlen(search);

	if (len < 2)
		return result.SetError("Search term must be longer than 1 character");

	if (len >= MAX_PATH - 1)
		return result.SetError("Search term is too big! 0_0");

	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	auto& cells = (*g_dataHandler)->cellList;
	const std::span<TESObjectCELL*> cellSpan(cells.m_data, cells.m_emptyRunStart);

	teleportSearchResult.result.clear();
	SearchSpanForEdidSubstring<TESObjectCELL>(teleportSearchResult, cellSpan, search, false);
	
	auto& worldspaces = (*g_dataHandler)->arrWRLD;
	const std::span<TESWorldSpace*> worldSpan((TESWorldSpace**)worldspaces.entries, worldspaces.count);
	const char* wilderness = aWilderness;
	std::for_each(std::execution::par, worldSpan.begin(), worldSpan.end(), [wilderness, lowered](TESWorldSpace* form) {
		form->cells.ForEach([wilderness, lowered](WorldCellItem* item) {
			if (item->cell) {
				auto edid = item->cell->GetEditorID();
				if (edid && *edid && edid != wilderness && HasInsensitiveSubstring(edid, lowered)) {
					teleportSearchResult.Push(edid, item->cell->formID);
				}
			}
			return true;
		});
	});

	if (teleportSearchResult.result.empty())
		return result.SetError("Search returned no results");

	teleportSearchResult.Sort();
}