#include "sam.h"

#include "f4se/GameMenus.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"

#include "hacks.h"
#include "eyes.h"
#include "pose.h"

#include <regex>

SelectedRefr selected;

MenuCache poseMenuCache;
MenuCache morphsMenuCache;

MenuCategoryList* GetMenu(MenuCache* cache)
{
	if (!selected.refr) return nullptr;

	if (cache->count(selected.key))
		return &(*cache)[selected.key];
	if (selected.isFemale && cache->count(selected.race))
		return &(*cache)[selected.race];

	//Default to human
	UInt32 race = 0x00013746;
	UInt64 key = race;
	if (selected.isFemale)
		key |= 0x100000000;

	if (cache->count(key))
		return &(*cache)[key];
	if (selected.isFemale && cache->count(race))
		return &(*cache)[race];
}

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible)
{
	if ((*g_ui)->IsMenuOpen(menuName)) {
		GFxMovieRoot * root = (*g_ui)->GetMenu(menuName)->movie->movieRoot;
		root->SetVariable(visiblePath, &GFxValue(visible));
	}
}

TESObjectREFR * GetRefr() {
	UInt32 handle = (*g_consoleHandle);
	NiPointer<TESObjectREFR> refr;
	if (handle == 0 || handle == *g_invalidRefHandle) {
		refr = *g_player;
	} else {
		LookupREFRByHandle(handle, refr);
		if (refr->formType != kFormType_ACHR)
			return nullptr;
	}
	return refr;
}

void SelectedRefr::Update(TESObjectREFR* newRefr) {
	if (!newRefr) {
		Clear();
		return;
	}
	refr = newRefr;
	eyeNode = GetEyeNode(refr);
	TESNPC* npc = (TESNPC*)refr->baseForm;
	isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);
	race = npc->race.race->formID;
	key = race;
	if (isFemale)
		key += 0x100000000;
}

void SelectedRefr::Clear() {
	refr = nullptr;
	eyeNode = nullptr;
}

void OnMenuOpen() {
	_DMESSAGE("Menu opened");

	static BSFixedString samMenu("ScreenArcherMenu");
	static BSFixedString photoMenu("PhotoMenu");

	TESObjectREFR* refr = GetRefr();
	selected.Update(refr);

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", false);

	GFxMovieRoot * root = (*g_ui)->GetMenu(samMenu)->movie->movieRoot;
	GFxValue data;
	root->CreateObject(&data);

	GFxValue delayClose((*g_ui)->IsMenuOpen(photoMenu));
	data.SetMember("delayClose", &delayClose);

	root->Invoke("root1.Menu_mc.menuOpened", nullptr, &data, 1);
}

void OnMenuClose() {
	_DMESSAGE("Menu closed");

	static BSFixedString photoMenu("PhotoMenu");

	selected.Clear();

	SetMenuVisible(photoMenu, "root1.Menu_mc.visible", true);
}

void OnConsoleRefUpdate() {
	static BSFixedString samMenu("ScreenArcherMenu");

	_DMESSAGE("Console ref updated");

	TESObjectREFR * refr = GetRefr();
	if (selected.refr != refr) {
		selected.Update(refr);

		GFxMovieRoot * root = (*g_ui)->GetMenu(samMenu)->movie->movieRoot;
		GFxValue data;
		root->CreateObject(&data);

		GFxValue menuState;
		root->GetVariable(&menuState, "root1.Menu_mc.state");

		bool isReset = false;

		//todo hotswap pose/morphs
		switch (menuState.GetInt()) {
			case 8: //eye
			{
				GFxValue eyeX(0.0);
				GFxValue eyeY(0.0);
				float coords[2];
				if (GetEyecoords(coords)) {
					eyeX.SetNumber(coords[0]);
					eyeY.SetNumber(coords[1]);
				}
				data.SetMember("eyeX", &eyeX);
				data.SetMember("eyeY", &eyeY);
				break;
			}
			case 9: //hack
			{
				GFxValue hacks;
				GetHacksGFx(root, &hacks);
				data.SetMember("hacks", &hacks);
				break;
			}
			default: isReset = true;
		}

		GFxValue reset(isReset);
		data.SetMember("reset", &reset);

		root->Invoke("root1.Menu_mc.consoleRefUpdated", nullptr, &data, 1);
	}
}

enum {
	kMenuHeaderRace = 0,
	kMenuHeaderMod,
	kMenuHeaderSex,
	kMenuHeaderType
};

enum {
	kMenuTypePose = 1,
	kMenuTypeMorphs
};

std::regex menuCategoryRegex("([^\\t]+)\\t+([^\\t]+)");
std::unordered_map<std::string, UInt32> menuHeaderMap = {
	{"race", kMenuHeaderRace},
	{"Race", kMenuHeaderRace},
	{"mod", kMenuHeaderMod},
	{"Mod", kMenuHeaderMod},
	{"sex", kMenuHeaderSex},
	{"Sex", kMenuHeaderSex},
	{"type", kMenuHeaderType},
	{"Type", kMenuHeaderType}
};

std::unordered_map<std::string, UInt32> menuTypeMap = {
	{"pose", kMenuTypePose},
	{"Pose", kMenuTypePose},
	{"morphs", kMenuTypeMorphs},
	{"Morphs", kMenuTypeMorphs},
};

bool LoadMenuFile(std::string path) {
	IFileStream file;

	if (!file.Open(path.c_str())) {
		_DMESSAGE("File not found");
		return false;
	}

	char buf[512];
	std::cmatch match;
	std::string categoryIdentifier = "Category";
	std::string header[4];

	//header
	for (int i = 0; i < 4; ++i) {
		file.ReadString(buf, 512, '\n', '\r');
		bool matched = std::regex_match(buf, match, menuCategoryRegex);
		if (matched && menuHeaderMap.count(match[1].str())) {
			header[menuHeaderMap[match[1].str()]] = match[2].str();
		}
		else {
			_LogCat("Failed to read header ", path);
			file.Close();
			return false;
		}
	}

	UInt64 key = GetFormID(header[kMenuHeaderMod], header[kMenuHeaderRace]);
	if (header[kMenuHeaderSex] == "female" || header[kMenuHeaderSex] == "Female")
		key |= 0x100000000;

	MenuCache* cache;
	UInt32 menuType = menuTypeMap[header[kMenuHeaderType]];
	if (!menuType) {
		_LogCat("Unknown menu type: ", header[kMenuHeaderType]);
		return false;
	}

	switch (menuType) {
		case kMenuTypePose: cache = &poseMenuCache; break;
		case kMenuTypeMorphs: cache = &morphsMenuCache; break;
	}

	//categories
	while (!file.HitEOF()) {
		file.ReadString(buf, 512, '\n', '\r');
		if (std::regex_match(buf, match, menuCategoryRegex)) {
			if (match[1].str() == categoryIdentifier) {
				MenuList list;
				(*cache)[key].push_back(std::make_pair(match[2].str(), list));
			}
			else {
				(*cache)[key].back().second.push_back(std::make_pair(match[2].str(), match[1].str()));
			}
		}
	}

	file.Close();

	return true;
}

void LoadMenuFiles() {
	for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAM\\Menus", "*.txt"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		LoadMenuFile(path);
	}
}