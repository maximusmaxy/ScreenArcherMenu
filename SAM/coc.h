#pragma once

#include "gfx.h"
#include "f4se/GameForms.h"

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
};

class TESWorldSpace : public TESForm {
public:
	TESFullName fullname;
	TESTexture unkTexture;
	tHashSet<WorldCellItem, UInt32> cells;
	TESObjectCELL* defaultCell;
	void* unk78;
	void* climate;
};

bool CenterOnCell(const char* edid, TESObjectCELL* cell);
void GetCellMods(GFxResult&);
void GetCells(GFxResult&, const char*);
void GetWorldspaceMods(GFxResult& result);
void GetWorldspacesFromMod(GFxResult& result, const char* mod);
void GetWorldspaceCells(GFxResult& result, UInt32 formId);
void SetCell(GFxResult&, UInt32);
void GetLastSearchResultCell(GFxResult&);
void SearchCells(GFxResult& result, const char* search);