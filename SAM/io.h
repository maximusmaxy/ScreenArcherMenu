#pragma once

#include "SAF/types.h"
#include "SAF/io.h"

#include "gfx.h"
#include "types.h"
#include "strnatcmp.h"
#include "sam.h"

#include "json/json.h"

#include <ostream>

typedef std::map<std::string, Json::Value, SAF::CaseInsensitiveCompareStr> JsonCache;

extern JsonCache menuDataCache;
extern SAF::InsensitiveStringSet extensionSet;

extern MenuCache poseMenuCache;
extern MenuCache morphsMenuCache;
extern MenuCache groupsMenuCache;
extern MenuCache exportMenuCache;
extern MenuCache filterMenuCache;

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

bool SaveOptionsFile(const char* path);

template <class Type>
Type Read(std::ifstream& stream)
{
	Type n;
	stream.read(reinterpret_cast<char*>(&n), sizeof(Type));
	return n;
}

template <class Type>
void Write(std::ostream& stream, Type n)
{
	stream.write(reinterpret_cast<char*>(&n), sizeof(Type));
}