#include "looks.h"

#include "f4se/GameReferences.h"
#include "f4se/GameData.h"
#include "f4se/GameStreams.h"
#include "f4se_common/Utilities.h"

#include "sam.h"
#include "forms.h"
#include "constants.h"

#include <algorithm>
#include <execution>

std::vector<const char*> hairMods;
std::vector<const char*> eyeMods;

typedef UInt32(*_ShowLooksMenuInternal)(TESObjectREFR* refr, UInt32 unk1, TESObjectREFR* unk2, TESObjectREFR* unk3, TESObjectREFR* unk4);
RelocAddr<_ShowLooksMenuInternal> ShowLooksMenuInternal(0x0B42C40);

void ShowLooksMenu(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	ShowLooksMenuInternal(selected.refr, 0, nullptr, nullptr, nullptr);
}

void SearchSpanForHeadpart(const std::span<BGSHeadPart*>& span, std::vector<const char*>& result, UInt32 type) {
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

	AddModVectorsToListNoMasters(esp, esl, result);
}

void GetLooksHairMods(GFxResult& result) {
	if (hairMods.empty()) {
		auto& headparts = (*g_dataHandler)->arrHDPT;
		auto span = std::span<BGSHeadPart*>(headparts.entries, headparts.count);
		SearchSpanForHeadpart(span, hairMods, BGSHeadPart::kTypeHair);
	}
		
	result.CreateNames();

	for (auto& mod : hairMods) {
		auto ext = strrchr(mod, '.');
		if (ext) {
			std::string str(mod, ext - mod);
			result.PushItem(str.c_str(), mod);
		}
		else {
			result.PushItem(mod, mod);
		}
	}
}

void GetLooksHairs(GFxResult& result, const char* mod) {
	
}

void GetLooksEyeMods(GFxResult& result) {

}

void GetLooksEyes(GFxResult& result, const char* mod) {

}

RelocAddr<const char*> facialBoneRegionUIRemap(0x2D34D70);

void GetLooksFaceSliderCategories(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto actor = (Actor*)selected.refr;
	//auto npc = (TESNPC*)actor->baseForm;
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
}

void GetLooksFaceSliders(GFxResult& result, SInt32 index) {
	
}