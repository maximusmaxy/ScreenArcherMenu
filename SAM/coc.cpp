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
#include "lights.h"

#include <span>
#include <algorithm>
#include <execution>
#include <format>

FormSearchResult teleportSearchResult;

TESWorldSpace* lastWorldspace = nullptr;
std::vector<std::pair<std::string, std::string>> cellResult;

std::vector<std::string> cellFavorites;

EdidList weatherResult;
const char* lastWeatherMod = nullptr;
EdidList imagespaceResult;
const char* lastImagespace = nullptr;

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

void GetWeatherMods(GFxResult& result) {
	auto& weathers = (*g_dataHandler)->arrWTHR;
	std::span<TESWeather*> span((TESWeather**)weathers.entries, weathers.count);
	std::vector<const char*> weatherMods;
	SearchSpanForMods(span, weatherMods);
	AddFormattedModsToResult(result, weatherMods);
}

void GetWeathers(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	if (mod != lastWeatherMod) {
		lastWeatherMod = mod;
		weatherResult.clear();
		FindEdidsFromFile(result, info, ESP::Sig("WTHR"), weatherResult);
		if (result.type == GFxResult::Error)
			return;
	}

	result.CreateMenuItems();
	for (auto& [edid, formId] :weatherResult) {
		result.PushItem(edid.c_str(), formId);
	}
}

//typedef void*(*_GetSkyInstance)();
//RelocAddr<_GetSkyInstance> GetSkyInstance(0x128CB0);
//typedef void(*_SetWeatherExternal)(void* sky, TESWeather* weather, bool, char);
//RelocAddr<_SetWeatherExternal> SetWeatherExternal(0x651A60);
//typedef void(*_ForceWeatherExternal)(void* sky, TESWeather* weather, bool);
//RelocAddr<_ForceWeatherExternal> ForceWeatherExternal(0x651BE0);

typedef void(*_QueueForceWeather)(UInt64 taskQueue, TESWeather* weather, bool);
RelocAddr<_QueueForceWeather> QueueForceWeather(0xD5C8C0);

void SetWeather(GFxResult& result, UInt32 formId) {
	auto form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Failed to find weather form");

	auto weather = DYNAMIC_CAST(form, TESForm, TESWeather);
	if (!weather)
		return result.SetError("Form was not a weather");

	QueueForceWeather(*taskQueueInterface, weather, false);
}

void GetImagespaceMods(GFxResult& result) {
	auto& imagespaces = (*g_dataHandler)->arrIMGS;
	std::span<TESForm*> span((TESForm**)imagespaces.entries, imagespaces.count);
	std::vector<const char*> imagespaceMods;
	SearchSpanForMods(span, imagespaceMods);
	AddFormattedModsToResult(result, imagespaceMods);
}

void GetImagespaces(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	if (mod != lastImagespace) {
		lastImagespace = mod;
		imagespaceResult.clear();
		FindEdidsFromFile(result, info, ESP::Sig("IMGS"), imagespaceResult);
		if (result.type == GFxResult::Error)
			return;
	}

	result.CreateMenuItems();
	for (auto& [edid, formId] : imagespaceResult) {
		result.PushItem(edid.c_str(), formId);
	}
}

//RelocPtr<UInt64> imagespaceManager(0x67220E8);
//
//typedef void(*_ImageSpaceSetOverrideBaseData)(UInt64 imagespaceManager, ImageSpaceBaseData* baseData);
//RelocAddr<_ImageSpaceSetOverrideBaseData> ImagespaceSetOverrideBaseData(0x505930);

typedef void(*_CellSetImageSpace)(TESObjectCELL* cell, TESImageSpace* imagespace);
RelocAddr<_CellSetImageSpace> CellSetImageSpace(0x3B5270);

void SetImagespace(GFxResult& result, UInt32 formId) {
	//If no formId then reset
	//if (!formId) {
	//	ImagespaceSetOverrideBaseData(*imagespaceManager, nullptr);
	//	return;
	//}

	auto form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Failed to find imagespace form");

	auto imagespace = DYNAMIC_CAST(form, TESForm, TESImageSpace);
	if (!imagespace)
		return result.SetError("Form was not an imagespace");

	//ImagespaceSetOverrideBaseData(*imagespaceManager, &imagespace->baseData);

	auto cell = (*g_player)->parentCell;
	if (!cell)
		return result.SetError("Player was not in a cell");

	CellSetImageSpace(cell, imagespace);
}

struct CellLighting {
	struct Color {
		UInt8 r;
		UInt8 g;
		UInt8 b;
		UInt8 a;
	};

	struct HSV {
		int hue;
		int saturation;
		int value;
	};

	Color ambientColor;
	Color directionalColor;
	Color fogColorNear;
	float fogNear;
	float fogFar;
	UInt32 directionalRotationXY;
	UInt32 directionalRotationZ;
	float directionalFade;
	float fogClipDistance;
	float fogPower;

	struct DirectionalAmbientLighting {
		enum Type {
			XPlus = 0,
			XNegative,
			YPlus,
			YNegative,
			ZPlus,
			ZNegative,
		};

		Color color[6];
		Color specular;
		float scale;
	} dalc;

	Color fogColorFar;
	float fogMax;
	float lightFadeBegin;
	float lightFadeEnd;
	UInt32 inherits;
	float nearHeightMid;
	float nearHeightRange;
	Color fogColorHighNear;
	Color fogColorHighFar;
	float highDensityScale;
	float fogNearScale;
	float fogFarScale;
	float fogHighNearScale;
	float fogHighFarScale;
	float farHeightMid;
	float farHeightRange;
};

CellLighting* GetInteriorCellLighting(GFxResult& result) {
	if (!g_player) {
		result.SetError("Could not find player");
		return nullptr;
	}
	auto cell = (*g_player)->parentCell;
	if (!cell) {
		result.SetError("Could not find current cell");
		return nullptr;
	}
	if ((!cell->flags & 0x1)) {
		result.SetError("Cell is not an interior");
		return nullptr;
	}
	if (!cell->unk50)
		result.SetError("Could not find cell lighting data");
	return (CellLighting*)cell->unk50;
}

void GetLightingMenu(GFxResult& result) {
	auto lighting = GetInteriorCellLighting(result);
	if (!lighting)
		return;

	result.CreateMenuItems();

	const char* itemNames[] = {
		"$SAM_FogDistance",
		"$SAM_FogNearColor",
		"$SAM_FogFarColor",
		"$SAM_FogHighNearColor",
		"$SAM_FogHighFarColor",
		"$SAM_AmbientColor",
		"$SAM_DirectionalColor",
		"$SAM_X+",
		"$SAM_X-",
		"$SAM_Y+",
		"$SAM_Y-",
		"$SAM_Z+",
		"$SAM_Z-"
	};

	SInt32 index = 0;
	for (auto& name : itemNames) {
		result.PushItem(name, index++);
	}
}

enum LightingMenus {
	Distance = 0,
	FogNear,
	FogFar,
	FogHighNear,
	FogHighFar,
	Ambient,
	Directional,
	XPlus,
	XNegative,
	YPlus,
	YNegative,
	ZPlus,
	ZNegative,
};

void SetLightingMenu(GFxResult& result, SInt32 index) {
	auto lighting = GetInteriorCellLighting(result);
	if (!lighting)
		return;

	switch (index) {
	case Distance:
		samManager.PushMenu("LightingDistances");
		break;
	case FogNear:
	case FogFar:
	case FogHighNear:
	case FogHighFar:
		samManager.PushMenu("LightingFogEdit");
		break;
	case Directional:
		samManager.PushMenu("LightingDirectionalEdit");
		break;
	case Ambient:
	case XPlus:
	case XNegative:
	case YPlus:
	case YNegative:
	case ZPlus:
	case ZNegative:
		samManager.PushMenu("LightingColorEdit");
		break;
	}
}

void GetLightingDistance(GFxResult& result) {
	auto lighting = GetInteriorCellLighting(result);
	if (!lighting)
		return;

	result.CreateValues();
	result.PushValue(lighting->fogNear);
	result.PushValue(lighting->fogFar);
	result.PushValue(lighting->nearHeightMid);
	result.PushValue(lighting->nearHeightRange);
	result.PushValue(lighting->farHeightMid);
	result.PushValue(lighting->farHeightRange);
	result.PushValue(lighting->fogPower);
	result.PushValue(lighting->fogMax);
	result.PushValue(lighting->fogClipDistance);
	result.PushValue(lighting->highDensityScale);
	result.PushValue(lighting->lightFadeBegin);
	result.PushValue(lighting->lightFadeEnd);
}

void SetLightingDistance(GFxResult& result, SInt32 index, double value) {
	enum LightingDistance {
		Near = 0,
		Far,
		NearHeightMid,
		NearHeightRange,
		FarHeightMid,
		FarHeightRange,
		Power,
		Max,
		ClipDistance,
		HighDensityScale,
		LightFadeBegin,
		LightFadeEnd,
	};

	auto lighting = GetInteriorCellLighting(result);
	if (!lighting)
		return;

	const float distance = (float)value;

	switch (index) {
	case Near: lighting->fogNear = distance; break;
	case Far: lighting->fogFar = distance; break;
	case NearHeightMid: lighting->nearHeightMid = distance; break;
	case NearHeightRange: lighting->nearHeightRange = distance; break;
	case FarHeightMid: lighting->farHeightMid = distance; break;
	case FarHeightRange: lighting->farHeightRange = distance; break;
	case Power: lighting->fogPower = distance; break;
	case Max: lighting->fogMax = distance; break;
	case ClipDistance: lighting->fogClipDistance = distance; break;
	case HighDensityScale: lighting->highDensityScale = distance; break;
	case LightFadeBegin: lighting->lightFadeBegin = distance; break;
	case LightFadeEnd: lighting->lightFadeEnd = distance; break;
	default: return result.SetError("Index out of range");
	}
}

CellLighting::HSV RGBToHSV(const CellLighting::Color& color) {
	const double r = color.r * (1 / 255.0);
	const double g = color.g * (1 / 255.0);
	const double b = color.b * (1 / 255.0);
	const double cmax = (std::max)((std::max)(r, g), b);
	const double cmin = (std::min)((std::min)(r, g), b);
	const double dif = cmax - cmin;

	constexpr double sqrt3over2 = 0.86602540378444;

	const double alpha = (2.0 * r - g - b) / 2.0;
	const double beta = (g - b) * sqrt3over2;
	const double h = std::atan2(beta, alpha);
	const double s = cmax == 0 ? 0.0 : dif / cmax;

	constexpr double rad2degree = 180.0 / 3.14159265358979323846;
	constexpr double twopi = 3.14159265358979323846 * 2.0;

	return {
		(int)std::round((h < 0.0 ? h + twopi : h) * rad2degree),
		(int)std::round(s * 100),
		(int)std::round(cmax * 100)
	};
}

CellLighting::Color HSVToRGB(const CellLighting::HSV& hsv) {
	const double h = std::fmod(hsv.hue, 360.0);
	const double s = std::clamp((double)hsv.saturation, 0.0, 100.0) / 100.0;
	const double v = std::clamp((double)hsv.value, 0.0, 100.0) / 100.0;
	const double c = v * s;
	const double hdash = h / 60.0;
	const int sextant = (int)std::floor(hdash);
	const double x = c * (1.0 - std::abs(std::fmod(hdash, 2.0) - 1.0));
	double r, g, b;
	switch (sextant) {
	case 0:
		r = c;
		g = x;
		b = 0;
		break;
	case 1:
		r = x;
		g = c;
		b = 0;
		break;
	case 2:
		r = 0;
		g = c;
		b = x;
		break;
	case 3:
		r = 0;
		g = x;
		b = c;
		break;
	case 4:
		r = x;
		g = 0;
		b = c;
		break;
	case 5:
		r = c;
		g = 0;
		b = x;
		break;
	default:
		r = 0;
		g = 0;
		b = 0;
	}
	const double m = v - c;
	return {
		(UInt8)std::round((r + m) * 255.0),
		(UInt8)std::round((g + m) * 255.0),
		(UInt8)std::round((b + m) * 255.0),
	};
}

CellLighting::Color& GetLightingColorType(CellLighting* lighting, SInt32 type) {
	switch (type) {
	case FogNear: return lighting->fogColorNear;
	case FogFar: return lighting->fogColorFar;
	case FogHighNear: return lighting->fogColorHighNear;
	case FogHighFar: return lighting->fogColorHighFar;
	case Ambient: return lighting->ambientColor;
	case Directional: return lighting->directionalColor;
	case XPlus:
	case XNegative: 
	case YPlus: 
	case YNegative: 
	case ZPlus:
	case ZNegative:
		return lighting->dalc.color[type - XPlus - 1];
	}
	return lighting->ambientColor;
}

void GetLightingColor(GFxResult& result, SInt32 selected) {
	auto lighting = GetInteriorCellLighting(result);
	if (!lighting)
		return;

	if (selected < 0 || selected > ZNegative)
		return result.SetError("Index out of range");

	result.CreateMenuItems();

	const auto& color = GetLightingColorType(lighting, selected);
	result.PushItem("Red", (SInt32)color.r);
	result.PushItem("Green", (SInt32)color.g);
	result.PushItem("Blue", (SInt32)color.b);
	const auto hsv = RGBToHSV(color);
	result.PushItem("Hue", (SInt32)hsv.hue);
	result.PushItem("Saturation", (SInt32)hsv.saturation);
	result.PushItem("Value", (SInt32)hsv.value);

	switch (selected) {
	case FogNear: result.PushItem("Scale", lighting->fogNearScale); break;
	case FogFar: result.PushItem("Scale", lighting->fogFarScale); break;
	case FogHighNear: result.PushItem("Scale", lighting->fogHighNearScale); break;
	case FogHighFar: result.PushItem("Scale", lighting->fogHighFarScale); break;
	case Directional: 
		result.PushItem("Rotation XY", lighting->directionalRotationXY);
		result.PushItem("Rotation Z", lighting->directionalRotationZ);
		result.PushItem("Fade", lighting->directionalFade);
		break;
	}
}

void SetLightingColor(GFxResult& result, SInt32 index, GFxValue& value, SInt32 selected) {
	auto lighting = GetInteriorCellLighting(result);
	if (!lighting)
		return;

	enum RGBHSV {
		Red = 0,
		Green,
		Blue,
		Hue,
		Saturation,
		Value,
		Scale,
		Rotation,
		Fade
	};

	if (index <= Value) {
		auto& color = GetLightingColorType(lighting, selected);

		if (index <= Blue) {
			switch (index) {
			case Red: color.r = value.GetUInt(); break;
			case Green: color.g = value.GetUInt(); break;
			case Blue: color.b = value.GetUInt(); break;
			}
		}
		else {
			auto hsv = RGBToHSV(color);
			switch (index) {
			case Hue: hsv.hue = value.GetInt(); break;
			case Saturation: hsv.saturation = value.GetInt(); break;
			case Value: hsv.value = value.GetInt(); break;
			}
			color = HSVToRGB(hsv);
		}

		if (selected == Ambient) 
		{
			CellLighting::Color halfColor{
				(UInt8)(color.r / 2),
				(UInt8)(color.g / 2),
				(UInt8)(color.b / 2)
			};
			using Type = CellLighting::DirectionalAmbientLighting::Type;
			lighting->dalc.color[Type::XPlus] = halfColor;
			lighting->dalc.color[Type::YPlus] = halfColor;
			lighting->dalc.color[Type::ZPlus] = halfColor;
			lighting->dalc.color[Type::XNegative] = color;
			lighting->dalc.color[Type::YNegative] = color;
			lighting->dalc.color[Type::ZNegative] = color;
		}
	}
	else {
		switch (selected) {
		case FogNear: lighting->fogNearScale = (float)value.GetNumber(); break;
		case FogFar: lighting->fogFarScale = (float)value.GetNumber(); break;
		case FogHighNear: lighting->fogHighNearScale = (float)value.GetNumber(); break;
		case FogHighFar: lighting->fogHighFarScale = (float)value.GetNumber(); break;
		case Directional:
			switch (index) {
			case Scale: lighting->directionalRotationXY = value.GetUInt(); break;
			case Rotation: lighting->directionalRotationZ = value.GetUInt(); break;
			case Fade: lighting->directionalFade = (float)value.GetNumber(); break;
			}
			break;
		}
	}
}

void ResetLighting(GFxResult& result) {
	if (!g_player)
		return result.SetError("Could not find player");

	auto cell = (*g_player)->parentCell;
	if (!cell)
		return result.SetError("Could not find current cell");

	if ((!cell->flags & 0x1))
		return result.SetError("Cell is not an interior");

	if (!cell->unk50)
		return result.SetError("Could not find cell lighting data");
	auto lighting = (CellLighting*)cell->unk50;

	const auto modInfo = GetModInfo(cell->formID);
	if (!modInfo)
		return result.SetError("Could not find esp for cell");

	using namespace ESP;

	Reader esp(modInfo);
	if (esp.Fail())
		return result.SetError("Failed to read esp");

	esp.ReadHeader();
	Record record;
	if (!esp.SeekToFormId(cell->formID, Sig("CELL"), record))
		return result.SetError("Failed to find form in esp");

	if (record.version != 131)
		return result.SetError("Form version not supported for reset");

	UInt32 count = esp.SeekToElement(record.size, Sig("XCLL"));
	if (!count)
		return result.SetError("Form had no lighting data");

	esp >> *lighting;
}

struct Sun {
	void* vftable;
	NiNode* root;
	NiNode* billboard;
	NiNode* billboard2;
	BSTriShape* shape;
	BSTriShape* shape2;
	BSTriShape* shape3;
	NiDirectionalLight* light;
	NiDirectionalLight* light2;
};

struct Sky {
	void* vftable;
	NiNode* unkNode08;
	NiNode* unkNode10;
	void* unk18;
	void* unk20;
	void* unk28;
	void* unk30;
	void* unk38;
	void* climate;
	TESWeather* weather48;
	void* unk50;
	TESWeather* weather58;
	void* unk60;
	void* unk68;
	void* atmosphere;
	void* stars;
	Sun* sun;
	void* clouds;
	void* unk90;
	void* unk98;
	void* unkA0;
	NiColor unkA8;
	NiColor unkB4;
};

RelocPtr<Sky*> skyInstance(0x59DB980);

RelocPtr<float> fSunXExtreme(0x3719210);
RelocPtr<float> fSunYExtreme(0x3719228);
RelocPtr<float> fSunZExtreme(0x3719240);

//this gets overridden by something from Sky::UpdateColors
RelocPtr<NiColorA> sunColor(0x6722D20);

void GetSunExtreme(GFxResult& result) {
	result.CreateValues();

	result.PushValue((double)*fSunXExtreme);
	result.PushValue((double)*fSunYExtreme);
	result.PushValue((double)*fSunZExtreme);
	//result.PushValue((double)sunColor->r);
	//result.PushValue((double)sunColor->g);
	//result.PushValue((double)sunColor->b);
	const auto sun = (*skyInstance)->sun;
	const auto light = sun ? sun->light : nullptr;
	result.PushValue(light ? (double)light->dimmer : 0.0);
}

void SetSunExtreme(GFxResult& result, SInt32 index, double value) {
	enum SunType {
		X = 0,
		Y,
		Z,
		//Red,
		//Green,
		//Blue,
		Strength,
		Rotation
	};

	switch (index) {
	case X: *fSunXExtreme = value; break;
	case Y: *fSunYExtreme = value; break;
	case Z: *fSunZExtreme = value; break;
	//case Red: sunColor->r = value; break;
	//case Green: sunColor->g = value; break;
	//case Blue: sunColor->b = value; break;
	case Strength:
	{
		const auto sun = (*skyInstance)->sun;
		const auto light = sun ? sun->light : nullptr;
		if (light)
			light->dimmer = value;
		break;
	}
	case Rotation:
	{
		const double xDif = *fSunXExtreme;
		const double yDif = *fSunYExtreme;
		const double distance = std::sqrt((xDif * xDif) + (yDif * yDif));

		double angle;
		if (yDif < 0.0) {
			angle = std::asin(-xDif / distance) + MATH_PI;
		}
		else {
			angle = std::asin(xDif / distance);
		}
		angle += (value * (MATH_PI / 180));

		*fSunXExtreme = (float)(std::sin(angle) * distance);
		*fSunYExtreme = (float)(std::cos(angle) * distance);
		break;
	}
	}
}

void ResetSunExtreme(GFxResult& result) {
	//Should get the defaults dynamically but cbf
	*fSunXExtreme = 600.0f;
	*fSunYExtreme = -325.0f;
	*fSunZExtreme = -150.0f;
}