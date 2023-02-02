#pragma once

#include "SAF/types.h"
#include "SAF/io.h"

#include "gfx.h"
#include "types.h"
#include "strnatcmp.h"
#include "sam.h"

#include "json/json.h"

typedef std::map<std::string, Json::Value, SAF::CaseInsensitiveCompareStr> JsonCache;

extern JsonCache menuDataCache;
extern SAF::InsensitiveStringSet extensionSet;

extern MenuCache poseMenuCache;
extern MenuCache morphsMenuCache;
extern MenuCache groupsMenuCache;
extern MenuCache exportMenuCache;

extern MenuCategoryList lightsMenuCache;
extern MenuCategoryList tongueMenuCache;

MenuCategoryList* GetMenu(SelectedRefr* refr, MenuCache* cache);

void LoadMenuFiles();
void ReloadJsonMenus();
Json::Value* GetCachedMenu(const char* name);
void GetMenuData(GFxResult& result, const char* name);

void GetExtensionMenus(GFxResult& result);

bool isDotOrDotDot(const char* cstr);
void GetSortedFilesAndFolders(const char* path, const char* ext, NaturalSortedMap& files, NaturalSortedMap& folders);
void GetFolder(GFxResult& result, const char* path, const char* ext);

void GetPathStem(GFxResult& result, const char* path);
void GetPathRelative(GFxResult& result, const char* root, const char* ext, const char* path);

template <class Type>
Type readstream(std::ifstream& stream)
{
	Type n;
	stream.read(reinterpret_cast<char*>(&n), sizeof(Type));
	return n;
}