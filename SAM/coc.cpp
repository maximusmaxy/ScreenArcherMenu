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

TESWorldSpace* lastWorldspace = nullptr;
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

void SetCellEdid(GFxResult& result, const char* edid) {
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
	using namespace ESP;

	Reader esp(info);
	if (esp.Fail())
		return result.SetError(std::string("Failed to read esp: ") + info->name);

	esp.ReadHeader();
	Header header;
	if (!esp.SeekToGroup(Sig("WRLD"), header.group))
		return result.SetError("No worldspaces in esp");
	
	esp >> header;
	if (header.group.sig != Sig("WRLD"))
		return result.SetError("No worldspaces in esp");
	
	const int gridSentinel = 0x7FFFFFFF;
	std::string edid;
	std::string buffer;

	while (header.record.sig == Sig("WRLD") && !esp.Eof())
	{
		//world
		esp.Skip(header.record.size);
		if (esp.GetFormId(header.record.formId) == worldspace->formID) {
			esp >> header;
			//world subgroup
			if (header.group.sig == Sig("GRUP")) {
				esp >> header;
				//persistent cell
				if (header.record.sig == Sig("CELL")) {
					esp.Skip(header.record.size); 
					auto cellform = header.record.formId;
					esp >> header;
					//persistent cell refs
					if (header.group.sig == Sig("GRUP") && header.group.value == cellform) {
						esp.SkipGroup(header.group.size); 
						esp >> header;
					}
				}

				//Cell blocks and sublocks
				while (header.record.sig != Sig("WRLD") && !esp.Eof()) {
					switch (header.record.sig) {
					case Sig("GRUP"):
						esp >> header;
						break;
					case Sig("CELL"):
					{

						bool compressed = header.record.IsCompressed();
						bool success = true;
						if (compressed) {
							auto dstLen = esp.Get();
							success = esp.Inflate(header.record.size - 4, dstLen, buffer);
							header.record.size = dstLen;
						}

						if (success) {
							//SInt32 gridx = gridSentinel;
							//SInt32 gridy = gridSentinel;
							//edid.clear();

							//esp.ForEachSig(header.record.size, [&](auto& element) {
							//	switch (element.sig) {
							//	case Sig("EDID"):
							//		esp >> edid;
							//		break;
							//	case Sig("XCLC"):
							//		esp >> gridx >> gridy;
							//		esp.Skip(4);
							//		break;
							//	default:
							//		esp.Skip(element.len);
							//	}
							//});

							//if (edid.size()) {
							//	cellResult.emplace_back(edid, edid);
							//}
							//else if (gridx != gridSentinel && gridy != gridSentinel) {
							//	cellResult.emplace_back(std::format("Grid {}, {}", gridx, gridy), );
							//}

							auto remaining = esp.SeekToElement(header.record.size, Sig("EDID"));
							if (remaining > 0) {
								esp >> edid;
								remaining -= (edid.size() + 1);
								if (edid.size())
									cellResult.emplace_back(edid, edid);
								esp.Skip(remaining);
							}

							if (compressed)
								esp.EndInflate();
						}

						auto cellform = header.record.formId;
						esp >> header;
						//cell refs
						if (header.group.sig == Sig("GRUP") && header.group.value == cellform) {
							esp.SkipGroup(header.group.size);
							esp >> header;
						}
						break;
					}
					case Sig("WRLD"):
						break;
					default:
						//force to break if unrecognized
						header.record.sig = Sig("WRLD");
					}
				}
			}

			return;
		}
		else {
			esp >> header;
			//skip world subgroup
			if (header.group.sig == Sig("GRUP")) {
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

	if (lastWorldspace != worldspace) {
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

		lastWorldspace = worldspace;
	}

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
		result.PushItem(item.c_str(), item.c_str());
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

	auto edid = cell->GetEditorID();
	if (!edid || !*edid || edid == aWilderness)
		return result.SetError("Could not favorite cell. No editor ID");

	const bool hasIdentifier = std::any_of(cellFavorites.begin(), cellFavorites.end(), [&edid](auto& rhs) {
		return _stricmp(edid, rhs.c_str()) == 0;
	});
	if (hasIdentifier)
		return result.SetError("Cell has already been favorited");

	cellFavorites.push_back(edid);
	if (!SaveCellFavorites())
		return result.SetError("Failed to save CellFavorites.txt");

	std::string notif = std::format("{} has been favorited!", edid);
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