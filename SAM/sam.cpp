#include "sam.h"

#include "f4se/GameMenus.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "hacks.h"
#include "eyes.h"
#include "pose.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"

SelectedRefr selected;

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

	GFxValue morphArray;
	root->CreateArray(&morphArray);

	if (selected.refr) {
		GetGFxMorphArray(root, &morphArray);
	}
	
	data.SetMember("morphArray", &morphArray);

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

		GFxValue isFemale(false);
		GFxValue morphArray;
		GFxValue eyeX(0.0);
		GFxValue eyeY(0.0);

		root->CreateArray(&morphArray);
		if (selected.refr) {
			GetGFxMorphArray(root, &morphArray);
			isFemale.SetBool(selected.isFemale);
			float coords[2];
			if (GetEyecoords(coords)) {
				eyeX.SetNumber(coords[0]);
				eyeY.SetNumber(coords[1]);
			}
		}

		data.SetMember("morphArray", &morphArray);
		data.SetMember("eyeX", &eyeX);
		data.SetMember("eyeY", &eyeY);

		GFxValue hacks;
		GetHacksGFx(root, &hacks);
		data.SetMember("hacks", &hacks);

		root->Invoke("root1.Menu_mc.consoleRefUpdated", nullptr, &data, 1);
	}
}