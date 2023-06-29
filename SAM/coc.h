#pragma once

#include "gfx.h"
#include "f4se/GameForms.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

struct WorldCellItem {
	UInt32 key;
	UInt32 pad;
	TESObjectCELL* cell;

	bool operator==(const UInt32 rhs) const { return key == rhs; }
	operator UInt32() const { return key; }
	static inline UInt32 GetHash(const UInt32* key)
	{
		UInt32 hash;
		CalculateCRC32_32(&hash, *key, 0);
		return hash;
	}

	static inline const std::pair<SInt16, SInt16> KeyToGrid(UInt32 key) {
		return std::make_pair(key >> 16, key & 0xFFFF);
	}

	static inline UInt32 GridToKey(SInt16 x, SInt16 y) {
		return ((UInt32)x << 16) | (UInt32)y;
	}
};

class TESWorldSpace : public TESForm {
public:
	TESFullName fullname;
	TESTexture unkTexture;
	tHashSet<WorldCellItem, UInt32> cells;
	TESObjectCELL* persistentCell;
	void* unk78;
	void* climate;
	//...
};

bool CenterOnCell(const char* edid, TESObjectCELL* cell);
void GetCellMods(GFxResult&);
void GetCells(GFxResult&, const char*);
void GetWorldspaceMods(GFxResult& result);
void GetWorldspacesFromMod(GFxResult& result, const char* mod);
void GetWorldspaceCells(GFxResult& result, const char* mod, UInt32 formId);
void SetCell(GFxResult&, UInt32);
void SetCellEdid(GFxResult&, const char*);
void GetLastSearchResultCell(GFxResult&);
void SearchCells(GFxResult& result, const char* search);
const char* GetCurrentDisplayedCell();

void GetCellFavorites(GFxResult& result);
bool SaveCellFavorites();
void AppendCellFavorite(GFxResult& result);
void RemoveCellFavorite(GFxResult& result, SInt32 index);
bool LoadCellFavorites();