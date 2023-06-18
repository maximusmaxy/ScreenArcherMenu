#include "looks.h"

#include "f4se/GameReferences.h"
#include "f4se/GameData.h"
#include "f4se/GameStreams.h"
#include "f4se/GameRTTI.h"
#include "f4se_common/Utilities.h"

#include "sam.h"
#include "forms.h"
#include "constants.h"
#include "functions.h"

#include <algorithm>
#include <execution>

typedef UInt32(*_ShowLooksMenuInternal)(TESObjectREFR* refr, UInt32 unk1, TESObjectREFR* unk2, TESObjectREFR* unk3, TESObjectREFR* unk4);
RelocAddr<_ShowLooksMenuInternal> ShowLooksMenuInternal(0x0B42C40);

void ShowLooksMenu(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	ShowLooksMenuInternal(selected.refr, 0, nullptr, nullptr, nullptr);
}
GFxReq showLooksMenu("ShowLooksMenu", [](auto& result, auto args) {
	ShowLooksMenu(result);
});

void SearchSpanForHeadpartMods(const std::span<BGSHeadPart*>& span, std::vector<const char*>& result, UInt32 type) {
	std::vector<bool> esp;
	std::vector<bool> esl;
	GetModVectors(esp, esl);
	auto espIt = esp.begin();
	auto eslIt = esl.begin();

	std::for_each(std::execution::par, span.begin(), span.end(), [&espIt, &eslIt, type](BGSHeadPart* form) {
		if (form && form->type == type) {
			const UInt32 formId = form->formID;
			if ((formId & 0xFE000000) == 0xFE000000) {
				*(eslIt + ((formId >> 12) & 0xFFF)) = true;
			}
			else {
				*(espIt + (formId >> 24)) = true;
			}
		}
	});

	AddModVectorsToList(esp, esl, result);
}

void SearchModForHeadparts(const ModInfo* info, FormSearchResult& result, UInt32 headpart) {
	UInt32 modIndex;
	UInt32 modMask;
	GetModIndexAndMask(info, &modIndex, &modMask);

	auto& headparts = (*g_dataHandler)->arrHDPT;
	auto span = std::span<BGSHeadPart*>(headparts.entries, headparts.count);

	std::mutex mutex;
	std::for_each(std::execution::par, span.begin(), span.end(), [&](BGSHeadPart* form) {
		if (form && form->type == headpart && ((form->formID & modMask) == modIndex)) {
			auto fullname = form->fullName.name.c_str();
			if (fullname && *fullname) {
				std::lock_guard lock(mutex);
				result.emplace_back(fullname, form->formID);
			}
		}
	});

	SortSearchResult(result);
}

void GetLooksHairMods(GFxResult& result) {
	std::vector<const char*> hairMods;
	auto& headparts = (*g_dataHandler)->arrHDPT;
	auto span = std::span<BGSHeadPart*>(headparts.entries, headparts.count);
	SearchSpanForHeadpartMods(span, hairMods, BGSHeadPart::kTypeHair);
		
	result.CreateNames();
	AddFormattedModsToResult(result, hairMods);
}
GFxReq getLooksHairMods("GetLooksHairMods", [](auto& result, auto args) {
	GetLooksHairMods(result);
});

void GetLooksHairs(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	FormSearchResult searchResult;
	SearchModForHeadparts(info, searchResult, BGSHeadPart::kTypeHair);
	
	result.CreateMenuItems();
	for (auto& pair : searchResult) {
		result.PushItem(pair.first, pair.second);
	}
}
GFxReq getLooksHairs("GetLooksHairs", [](auto& result, auto args) {
	GetLooksHairs(result, args->args[2].GetString());
});

void GetLooksEyeMods(GFxResult& result) {
	std::vector<const char*> eyeMods;
	auto& headparts = (*g_dataHandler)->arrHDPT;
	auto span = std::span<BGSHeadPart*>(headparts.entries, headparts.count);
	SearchSpanForHeadpartMods(span, eyeMods, BGSHeadPart::kTypeEyes);

	result.CreateNames();
	AddFormattedModsToResult(result, eyeMods);
}
GFxReq getLooksEyeMods("GetLooksEyeMods", [](auto& result, auto args) {
	GetLooksEyeMods(result);
});

void GetLooksEyes(GFxResult& result, const char* mod) {
	const ModInfo* info = (*g_dataHandler)->LookupModByName(mod);
	if (!info)
		return result.SetError("Could not find mod");

	FormSearchResult searchResult;
	SearchModForHeadparts(info, searchResult, BGSHeadPart::kTypeEyes);

	result.CreateMenuItems();
	for (auto& pair : searchResult) {
		result.PushItem(pair.first, pair.second);
	}
}
GFxReq getLooksEyes("GetLooksEyes", [](auto& result, auto args) {
	GetLooksEyes(result, args->args[1].GetString());
});

typedef void(*_ActorChangeHeadPart)(VirtualMachine* vm, UInt32, TESObjectREFR* actor, BGSHeadPart* headpart, bool remove, bool removeExtra);
RelocAddr<_ActorChangeHeadPart> ActorChangeHeadPart(0x1387D30);

void SetLooksHair(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find form for hair");

	BGSHeadPart* hairForm = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
	if (!hairForm)
		return result.SetError("Could not find form for hair");

	ActorChangeHeadPart((*g_gameVM)->m_virtualMachine, 0, selected.refr, hairForm, false, false);
}
GFxReq setLooksHair("SetLooksHair", [](auto& result, auto args) {
	SetLooksHair(result, args->args[1].GetUInt());
});

void SetLooksEyes(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find form for hair");

	BGSHeadPart* eyeForm = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
	if (!eyeForm)
		return result.SetError("Could not find form for hair");

	ActorChangeHeadPart((*g_gameVM)->m_virtualMachine, 0, selected.refr, eyeForm, false, false);
}
GFxReq setLooksEyes("SetLooksEyes", [](auto& result, auto args) {
	SetLooksEyes(result, args->args[1].GetUInt());
});

void GetLooksFaceSliderCategories(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto actor = (Actor*)selected.refr;
	auto npc = (TESNPC*)actor->baseForm;
	auto race = actor->race;

	//BSFaceChargenUtils::
	//BSCharacterMorph::FacialBoneRegion <- figure out how to get name from this 0x28?
	//TESRace::GetFacialBoneRegion and slider and maybe face morph

	//char buffer[0x104];
	//sprintf_s(buffer, 0x104, facialBoneRegionUIRemap, race->editorId, (CALL_MEMBER_FN(npc, GetSex)() == 1 ? "Female" : "Male"));
	//BSResourceNiBinaryStream stream(buffer);
	//if (!stream.IsValid())
	//	return result.SetError("Failed to read facial bone region file");

	//char line[0x400];
	//memset(line, 0, sizeof(line));
	//stream.ReadLine(line, 0x400, 0x10); //header
	//while (stream.ReadLine(line, 0x400, 0x10)) {

	//}

	//TODO
	result.CreateMenuItems();
}
GFxReq getLooksFaceSliderCategories("GetLooksFaceSliderCategories", [](auto& result, auto args) {
	GetLooksFaceSliderCategories(result);
});

void GetLooksFaceSliders(GFxResult& result, SInt32 index) {
	//TODO
	result.CreateValues();
}
GFxReq getLooksFaceSliders("GetLooksFaceSliders", [](auto& result, auto args) {
	GetLooksFaceSliders(result, args->args[2].GetInt());
});

void SetLooksFaceSlider(GFxResult& result, SInt32 index, double value) {
	//TODO
}
GFxReq setLooksFaceSlider("SetLooksFaceSlider", [](auto& result, auto args) {
	SetLooksFaceSlider(result, args->args[0].GetInt(), args->args[1].GetNumber());
});