#include "coc.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameMenus.h"
#include "gfx.h"
#include "forms.h"
#include "constants.h"
#include "sam.h"
#include "strnatcmp.h"
#include "esp.h"

#include <span>
#include <algorithm>
#include <execution>
#include <format>

FormSearchResult teleportSearchResult;
std::vector<std::pair<std::string, UInt32>> gridResult;

typedef void(*_CenterOnCellInternal)(TESObjectREFR* player, const char* edid, TESObjectCELL* cell);
RelocAddr<_CenterOnCellInternal> CenterOnCellInternal(0xE9A990);

RelocAddr<const char*> aWilderness(0x2C870F0);

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

//void GetWorldspaceCellsFromFile(GFxResult& result, TESWorldSpace* worldspace,
//	std::vector<std::pair<const char*, UInt32>>& cellResult, const ModInfo* info)
//{
//	EspReader esp(info->name);
//	if (esp.Fail())
//		return result.SetError(std::string("Failed to read esp: ") + info->name);
//
//	esp.SkipRecord();
//	if (!esp.SeekToGroup('DLRW'))
//		return result.SetError("No worldspaces in esp");
//
//	auto sig = esp.Get();
//	if (sig != 'DLRW')
//		return result.SetError("No worldspaces in esp");
//
//	UInt32 modIndex;
//	UInt32 mask;
//	GetModIndexAndFormMask(info, &modIndex, &mask);
//
//	while (!esp.Eof()) {
//		auto len = esp.Get();
//		esp.Skip(4);
//		auto formId = (modIndex | (esp.Get() & mask));
//		esp.Skip(0x8 + len);
//		if (formId == worldspace->formID) {
//			esp.Skip(0x18);//grup
//			esp.SkipRecord();//persistent cell
//			esp.SkipGroup(); //persistent cell refs
//			while (!esp.Eof() && (sig = esp.Get()) != 'DLRW') {
//				switch (sig) {
//				case 'PURG':
//					esp.Skip(0x18);
//					break;
//				//TODO
//				}
//			}
//		}
//		else {
//			esp.Skip(4);
//			auto gruplen = esp.Get();
//			esp.Skip(gruplen - 0x8);
//		}
//	}
//
//	result.SetError("Could not find worldspace in esp");
//}

void GetWorldspaceCellsFromData(GFxResult& result, TESWorldSpace* worldspace,
	std::vector<std::pair<const char*, UInt32>>& cellResult) 
{
	const char* wilderness = aWilderness;
	worldspace->cells.ForEach([&cellResult, wilderness](WorldCellItem* item) {
		if (item->cell) {
			auto edid = item->cell->GetEditorID();
			if (edid && *edid && edid != wilderness) {
				cellResult.push_back(std::make_pair(edid, item->cell->formID));
			}
			else {
				const auto [x, y] = WorldCellItem::KeyToGrid(item->key);
				std::string grid = std::format("Grid {}, {}", x, y);
				gridResult.emplace_back(std::make_pair(grid, item->cell->formID));
			}
		}
		return true;
	});
}

void GetWorldspaceCells(GFxResult& result, const char* mod, UInt32 formId) {
	auto form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Failed to find worldspace form");

	auto worldspace = DYNAMIC_CAST(form, TESForm, TESWorldSpace);
	if (!worldspace)
		return result.SetError("Form was not a worldspace");

	//collect seperately to add empty grid cells after named edid cells
	std::vector<std::pair<const char*, UInt32>> cellResult;
	gridResult.clear();
	GetWorldspaceCellsFromData(result, worldspace, cellResult);

	//const std::vector<UInt32> ignoreWorldspaces{
	//	0x3C //Commonwealth
	//};
	
	//auto it = std::find(ignoreWorldspaces.begin(), ignoreWorldspaces.end(), formId);
	//if (it != ignoreWorldspaces.end()) {
	//	GetWorldspaceCellsFromData(result, worldspace, cellResult);
	//}
	//else {
	//	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	//	if (!info)
	//		return result.SetError("Could not find mod");

	//	GetWorldspaceCellsFromFile(result, worldspace, cellResult, info);
	//}
	
	//if (result.type == GFxResult::Error)
	//	return;

	std::sort(cellResult.begin(), cellResult.end(), [](auto& lhs, auto& rhs) {
		return strnatcmp(lhs.first, rhs.first) < 0;
	});
	std::sort(gridResult.begin(), gridResult.end(), [](auto& lhs, auto& rhs) {
		return strnatcmp(lhs.first.c_str(), rhs.first.c_str()) < 0;
	});

	result.CreateMenuItems();
	for (auto& [name, formId] : cellResult) {
		result.PushItem(name, formId);
	}
	for (auto& [name, formId] : gridResult) {
		result.PushItem(name.c_str(), formId);
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

std::string displayedCell;
const char* GetCurrentDisplayedCell() {
	auto player = *g_player;
	if (player) {
		if (player->parentCell) {
			auto edid = player->parentCell->GetEditorID();
			auto worldspace = player->parentCell->worldSpace;
			if (worldspace) {
				if (edid && *edid && edid != aWilderness) {
					return edid;
				}
				else {
					auto heightData = player->parentCell->unk50;
					if (heightData) {
						auto x = (int)heightData->unk00;
						auto y = (int)heightData->unk04;
						auto worldEdid = worldspace->GetEditorID();
						if (worldEdid && *worldEdid) {
							displayedCell = std::format("{} {}, {}", worldEdid, x, y);
						}
						else {
							displayedCell = std::format("Grid {}, {}", x, y);
						}
						return displayedCell.c_str();
					}
				}
			}
			else {
				return edid;
			}
		}
	}

	return "";
}