#include "coc.h"

#include "common/IFileStream.h"
#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameMenus.h"
#include "SAF/io.h"
#include "SAF/util.h"
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
std::vector<std::pair<std::string, std::string>> cellResult;
std::vector<std::string> cellFavorites;

typedef void(*_CenterOnCellInternal)(TESObjectREFR* player, const char* edid, TESObjectCELL* cell);
RelocAddr<_CenterOnCellInternal> CenterOnCellInternal(0xE9A990);

RelocAddr<const char*> aWilderness(0x2C870F0);

std::string displayedCell;

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
	samManager.closeMessage = "$SAM_COCMessage";
	samManager.OpenOrCloseMenu();
}

void SetWorldspace(GFxResult& result, const char* edid) {
	samManager.storedEdid = edid;
	samManager.closeMessage = "$SAM_COCMessage";
	samManager.OpenOrCloseMenu();
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

void GetWorldspaceCellsFromFile(GFxResult& result, TESWorldSpace* worldspace, const ModInfo* info)
{
	ESP::Reader esp(info);
	if (esp.Fail())
		return result.SetError(std::string("Failed to read esp: ") + info->name);

	esp.ReadHeader();
	if (!esp.SeekToGroup('DLRW'))
		return result.SetError("No worldspaces in esp");

	ESP::Header header;
	esp >> header;
	if (header.group.sig != 'DLRW')
		return result.SetError("No worldspaces in esp");
	
	const int gridSentinel = 0x7FFFFFFF;
	std::string edid;

	while (header.record.sig == 'DLRW' && !esp.Eof()) 
	{
		//world
		esp.Skip(header.record.size);
		if (esp.GetFormId(header.record.formId) == worldspace->formID) {
			esp >> header;
			//world subgroup
			if (header.group.sig == 'PURG') {
				esp >> header;
				//persistent cell
				if (header.record.sig == 'LLEC') {
					esp.Skip(header.record.size); 
					auto cellform = header.record.formId;
					esp >> header;
					//persistent cell refs
					if (header.group.sig == 'PURG' && header.group.value == cellform) {
						esp.SkipGroup(header.group.size); 
						esp >> header;
					}
				}

				//Cell blocks and sublocks
				while (header.record.sig != 'DLRW' && !esp.Eof()) {
					switch (header.record.sig) {
					case 'PURG':
						esp >> header;
						break;
					case 'LLEC':
					{
						bool compressed = header.record.flags & (1 << 18);
						if (compressed)
							esp.Inflate(header.record.size);

						SInt32 gridx = gridSentinel;
						SInt32 gridy = gridSentinel;
						edid.clear();

						esp.ForEachSig(header.record.size, [&](auto elementsig, auto elementlen) {
							switch (elementsig) {
							case 'DIDE':
								esp >> edid;
								break;
							case 'CLCX':
								esp >> gridx >> gridy;
								esp.Skip(4);
								break;
							default:
								esp.Skip(elementlen);
							}
						});

						auto cellform = header.record.formId;
						if (edid.size()) {
							cellResult.emplace_back(edid, edid);
						}
						//else if (gridx != gridSentinel && gridy != gridSentinel) {
						//	cellResult.emplace_back(std::format("Grid {}, {}", gridx, gridy), );
						//}

						if (compressed)
							esp.EndInflate();

						esp >> header;
						//cell refs
						if (header.group.sig == 'PURG' && header.group.value == cellform) {
							esp.SkipGroup(header.group.size);
							esp >> header;
						}
						break;
					}
					case 'DLRW':
						break;
					default:
						//force to break if unrecognized
						header.record.sig = 'DLRW';
					}
				}
			}

			return;
		}
		else {
			esp >> header;
			//skip world subgroup
			if (header.group.sig == 'PURG') {
				esp.SkipGroup(header.group.size);
				esp >> header;
			}
		}
	}

	result.SetError("Could not find worldspace in esp");
}

void GetWorldspaceCellsFromData(GFxResult& result, TESWorldSpace* worldspace) 
{
	const char* wilderness = aWilderness;
	worldspace->cells.ForEach([&wilderness](WorldCellItem* item) {
		if (item->cell) {
			auto edid = item->cell->GetEditorID();
			if (edid && *edid && edid != wilderness) {
				cellResult.push_back(std::make_pair(edid, edid));
			}
			//else {
			//	const auto [x, y] = WorldCellItem::KeyToGrid(item->key);
			//	std::string grid = std::format("Grid {}, {}", x, y);
			//	gridResult.emplace_back(std::make_pair(grid, item->cell->formID));
			//}
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
	cellResult.clear();
	//gridResult.clear();

	const std::vector<UInt32> ignoreWorldspaces{
		0x3C //Commonwealth
	};
	auto ignore = std::find(ignoreWorldspaces.begin(), ignoreWorldspaces.end(), formId) != ignoreWorldspaces.end();

	if (ignore) {
		GetWorldspaceCellsFromData(result, worldspace);
	}
	else {
		const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
		if (!info)
			return result.SetError("Could not find mod");

		GetWorldspaceCellsFromFile(result, worldspace, info);
	}
	
	if (result.type == GFxResult::Error)
		return;

	auto SortCells = [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.first.c_str(), rhs.first.c_str()) < 0;
	};
	std::sort(cellResult.begin(), cellResult.end(), SortCells);
	//std::sort(gridResult.begin(), gridResult.end(), SortCells);

	result.CreateMenuItems();
	for (auto& [name, edid] : cellResult) {
		result.PushItem(name.c_str(), edid.c_str());
	}
	//for (auto& [name, edid] : gridResult) {
	//	result.PushItem(name.c_str(), edid.c_str());
	//}
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

const char* GetCurrentDisplayedCell() {
	displayedCell.clear();

	auto player = *g_player;
	if (player) {
		if (player->parentCell) {
			auto modname = GetModName(player->parentCell->formID);
			if (modname && *modname) {
				displayedCell.append(modname);
				displayedCell.append(": ");
			}
			auto edid = player->parentCell->GetEditorID();
			auto worldspace = player->parentCell->worldSpace;
			if (worldspace) {
				if (edid && *edid && edid != aWilderness) {
					displayedCell.append(edid);
				}
				else {
					auto heightData = player->parentCell->unk50;
					if (heightData) {
						auto x = (int)heightData->unk00;
						auto y = (int)heightData->unk04;
						auto worldEdid = worldspace->GetEditorID();
						if (worldEdid && *worldEdid) {
							displayedCell.append(std::format("{} {}, {}", worldEdid, x, y));
						}
						else {
							displayedCell.append(std::format("Grid {}, {}", modname, x, y));
						}
					}
				}
			}
			else {
				displayedCell.append(edid);
			}
		}
	}

	return displayedCell.c_str();
}

void GetCellFavorites(GFxResult& result)
{
	std::sort(cellFavorites.begin(), cellFavorites.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.c_str(), rhs.c_str()) < 0;
	});

	result.CreateMenuItems();
	for (auto& item : cellFavorites) {
		auto formId = FromFormIdentifier(item.c_str());
		if (formId) {
			auto form = LookupFormByID(formId);
			if (form) {
				auto edid = form->GetEditorID();
				result.PushItem((edid && *edid && edid != aWilderness) ? edid : item.c_str(), formId);
			}
		}
	}
}

bool SaveCellFavorites()
{
	SAF::OutStreamWrapper wrapper(CELL_FAVORITES);
	if (wrapper.fail)
		return false;

	for (auto& favorite : cellFavorites) {
		wrapper.stream << favorite << std::endl;
	}

	return true;
}

void AppendCellFavorite(GFxResult& result)
{
	auto player = *g_player;
	if (!player)
		return result.SetError("Could not find player");

	auto cell = player->parentCell;
	if (!cell)
		return result.SetError("Could not find current cell");

	auto identifier = ToFormIdentifier(cell->formID);
	const bool hasIdentifier = std::any_of(cellFavorites.begin(), cellFavorites.end(), [&identifier](auto& rhs) {
		return _stricmp(identifier.c_str(), rhs.c_str()) == 0;
	});
	if (hasIdentifier)
		return result.SetError("Cell has already been favorited");

	cellFavorites.push_back(identifier);

	if (!SaveCellFavorites())
		return result.SetError("Failed to save CellFavorites.txt");

	std::string notif;
	auto edid = cell->GetEditorID();
	if (edid && *edid && edid != aWilderness)
		notif.append(edid);
	else
		notif.append("Cell");
	notif.append(" has been favorited!");
	samManager.ShowNotification(notif.c_str(), false);
}

void RemoveCellFavorite(GFxResult& result, SInt32 index) {
	if (index < 0 || index >= cellFavorites.size())
		return result.SetError("cell index out of range");

	cellFavorites.erase(cellFavorites.begin() + index);

	if (!SaveCellFavorites())
		return result.SetError("Failed to save CellFavorites.txt");
}

bool LoadCellFavorites()
{
	if (!std::filesystem::exists(CELL_FAVORITES)) {
		IFileStream::MakeAllDirs(CELL_FAVORITES);
		IFileStream file;
		if (!file.Create(CELL_FAVORITES)) {
			_Log("Failed to create cell favorites: ", CELL_FAVORITES);
			return false;
		}
		file.Close();
	}

	std::ifstream stream;
	stream.open(CELL_FAVORITES);

	if (stream.fail()) {
		_Log("Failed to read cell favorites: ", CELL_FAVORITES);
		return false;
	}

	cellFavorites.clear();

	std::string line;
	while (std::getline(stream, line, '\n'))
	{
		if (line.back() == '\r')
			line.pop_back();
		cellFavorites.push_back(line);
	}

	return true;
}