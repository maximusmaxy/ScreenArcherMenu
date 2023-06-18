#include "coc.h"

#include "f4se/GameData.h"
#include "f4se/GameForms.h"
#include "f4se/GameRTTI.h"
#include "gfx.h"
#include "functions.h"
#include "forms.h"
#include "constants.h"

#include <span>
#include <algorithm>
#include <execution>

FormSearchResult teleportSearchResult;

void SearchSpanForCellMods(std::span<TESObjectCELL*>& span, std::vector<const char*>& result) {
	std::vector<bool> esp;
	std::vector<bool> esl;
	GetModVectors(esp, esl);
	auto espIt = esp.begin();
	auto eslIt = esl.begin();

	std::for_each(std::execution::par, span.begin(), span.end(), [&espIt, &eslIt](TESObjectCELL* form) {
		if (form && form->GetEditorID()) {
			const UInt32 formId = form->formID;
			if ((formId & 0xFE000000) == 0xFE000000) {
				*(eslIt + ((formId >> 12) & 0xFFF)) = true;
			}
			else {
				*(espIt + (formId >> 24)) = true;
			}
		}
	});

	AddModVectorsToList(esp, esl, result);
}

void GetTeleportMods(GFxResult& result) {
	auto cells = (tArray<TESObjectCELL*>*)(void*)(&((*g_dataHandler)->arrCELL));
	std::span<TESObjectCELL*> span(cells->entries, cells->count);
	std::vector<const char*> cellMods;
	SearchSpanForCellMods(span, cellMods);

	result.CreateMenuItems();
	AddFormattedModsToResult(result, cellMods);
}
GFxReq getTeleportMods("GetTeleportMods", [](auto& result, auto args) {
	GetTeleportMods(result);
});

void SearchModForCells(const ModInfo* info, FormSearchResult& result) {
	UInt32 modIndex;
	UInt32 modMask;
	GetModIndexAndMask(info, &modIndex, &modMask);

	auto cells = (tArray<TESObjectCELL*>*)(void*)(&((*g_dataHandler)->arrCELL));
	std::span<TESObjectCELL*> span(cells->entries, cells->count);

	std::mutex mutex;
	std::for_each(std::execution::par, span.begin(), span.end(), [&](TESObjectCELL* form) {
		if (form && ((form->formID & modMask) == modIndex)) {
			auto edid = form->GetEditorID();
			if (edid && *edid) {
				std::lock_guard lock(mutex);
				result.emplace_back(edid, form->formID);
			}
		}
	});

	SortSearchResult(result);
}

void GetTeleports(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	FormSearchResult searchResult;
	SearchModForCells(info, searchResult);

	result.CreateMenuItems();
	for (auto& pair : searchResult) {
		result.PushItem(pair.first, pair.second);
	}
}
GFxReq getTeleports("GetTeleports", [](auto& result, auto args) {
	GetTeleports(result, args->args[2].GetString());
});

typedef void(*_CenterOnCell)(TESObjectREFR* player, const char* edid, TESObjectCELL* cell);
RelocAddr<_CenterOnCell> CenterOnCell(0xE9A990);

void SetTeleport(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto form = LookupFormByID(formId);
	if (!form)
		result.SetError("Failed to find cell form");

	auto cell = DYNAMIC_CAST(form, TESForm, TESObjectCELL);
	if (!cell)
		result.SetError("Form was not a cell");

	CenterOnCell(selected.refr, nullptr, cell);
}
GFxReq setTeleport("SetTeleport", [](auto& result, auto args) {
	SetTeleport(result, args->args[1].GetUInt());
});

void GetLastSearchResultTeleport(GFxResult& result) {
	result.CreateMenuItems();

	for (auto& kvp : teleportSearchResult) {
		if (kvp.first && *kvp.first)
			result.PushItem(kvp.first, kvp.second);
	}
}
GFxReq getLastSearchResultTeleport("GetLastSearchResultTeleport", [](auto& result, auto args) {
	GetLastSearchResultTeleport(result);
});

void SearchTeleports(GFxResult& result, const char* search) {
	if (!search || !*search)
		return result.SetError("No search term found");

	int len = strlen(search);

	if (len < 2)
		return result.SetError("Search term must be longer than 1 character");

	if (len >= MAX_PATH - 1)
		return result.SetError("Search term is too big! 0_0");

	auto& cells = (*g_dataHandler)->arrCELL;
	std::span<TESForm*> span(cells.entries, cells.count);

	teleportSearchResult.clear();
	SearchSpanForSubstring(teleportSearchResult, span, search);

	if (teleportSearchResult.empty())
		return result.SetError("Search returned no results");
}