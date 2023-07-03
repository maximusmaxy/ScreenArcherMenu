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
#include "strnatcmp.h"

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

	std::for_each(std::execution::par, span.begin(), span.end(), [&](BGSHeadPart* form) {
		if (form && form->type == headpart && ((form->formID & modMask) == modIndex)) {
			auto fullname = form->fullName.name.c_str();
			if (fullname && *fullname) {
				result.Push(fullname, form->formID);
			}
		}
	});

	result.Sort();
}

void GetLooksHairMods(GFxResult& result) {
	std::vector<const char*> hairMods;
	auto& headparts = (*g_dataHandler)->arrHDPT;
	auto span = std::span<BGSHeadPart*>(headparts.entries, headparts.count);
	SearchSpanForHeadpartMods(span, hairMods, BGSHeadPart::kTypeHair);
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
	for (auto& pair : searchResult.result) {
		result.PushItem(pair.first, pair.second);
	}
}
GFxReq getLooksHairs("GetLooksHairs", [](auto& result, auto args) {
	GetLooksHairs(result, args->args[0].GetString());
});

void GetLooksEyeMods(GFxResult& result) {
	std::vector<const char*> eyeMods;
	auto& headparts = (*g_dataHandler)->arrHDPT;
	auto span = std::span<BGSHeadPart*>(headparts.entries, headparts.count);
	SearchSpanForHeadpartMods(span, eyeMods, BGSHeadPart::kTypeEyes);
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
	for (auto& pair : searchResult.result) {
		result.PushItem(pair.first, pair.second);
	}
}
GFxReq getLooksEyes("GetLooksEyes", [](auto& result, auto args) {
	GetLooksEyes(result, args->args[0].GetString());
});

//typedef void(*_ActorChangeHeadPart)(VirtualMachine* vm, UInt32, TESObjectREFR* actor, BGSHeadPart* headpart, bool remove, bool removeExtra);
//RelocAddr<_ActorChangeHeadPart> ActorChangeHeadPart(0x1387D30);

typedef BGSHeadPart*(*_NPCGetHeadpart)(TESNPC* npc, UInt32 type);
RelocAddr<_NPCGetHeadpart> NPCGetHeadpart(0x5B5730);

typedef void(*_NPCRemoveHeadpart)(TESNPC* npc, BGSHeadPart* headpart, bool removeExtra);
RelocAddr<_NPCRemoveHeadpart> NPCRemoveHeadpart(0x5B5560);

typedef void(*_NPCAddHeadPart)(TESNPC* npc, BGSHeadPart* headpart, bool, bool, bool);
RelocAddr<_NPCAddHeadPart> NPCAddHeadPart(0x5B52E0);

typedef void(*_ActorBaseAddChange)(TESActorBase* actorBase, UInt32 flags);
RelocAddr<_ActorBaseAddChange> ActorBaseAddChange(0x5A2950);

typedef UInt32*(*_GetActorHandle)(Actor* actor, UInt32* handle);
RelocAddr<_GetActorHandle> GetActorHandle(0xD89690);

typedef void(*_QueueReplaceHeadPart)(UInt64 queue, UInt32* actorHandle, BGSHeadPart* oldpart, BGSHeadPart* newpart);
RelocAddr<_QueueReplaceHeadPart> QueueReplaceHeadPart(0xD5F040);

void SetHeadPart(TESObjectREFR* refr, BGSHeadPart* newpart, UInt32 type) {
	auto actor = (Actor*)refr;
	auto npc = (TESNPC*)actor->baseForm;

	auto oldpart = NPCGetHeadpart(npc, type);
	if (oldpart)
		NPCRemoveHeadpart(npc, oldpart, true);
	NPCAddHeadPart(npc, newpart, true, false, true);
	if (oldpart != newpart) {
		ActorBaseAddChange(npc, 0x800);
		UInt32 out;
		UInt32* handle = GetActorHandle(actor, &out);
		QueueReplaceHeadPart(*taskQueueInterface, handle, oldpart, newpart);
		
		//*g_invalidRefHandle
		//QueueActorUpdateModel(*taskQueueInterface, actor, 0xC, 0.0f);
	}
}

void SetLooksHair(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find form for hair");

	BGSHeadPart* hairForm = DYNAMIC_CAST(form, TESForm, BGSHeadPart);
	if (!hairForm)
		return result.SetError("Could not find form for hair");

	SetHeadPart(selected.refr, hairForm, BGSHeadPart::kTypeHair);
	//ActorChangeHeadPart((*g_gameVM)->m_virtualMachine, 0, selected.refr, hairForm, false, false);
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

	SetHeadPart(selected.refr, eyeForm, BGSHeadPart::kTypeEyes);
	//ActorChangeHeadPart((*g_gameVM)->m_virtualMachine, 0, selected.refr, eyeForm, false, false);
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

RelocPtr<tArray<BGSKeyword*>> typedKeywords(0x59DA3C0);

void GetTypedKeywords(GFxResult& result, UInt32 type) {
	auto& keywords = typedKeywords[type];
	if (!keywords.entries)
		return result.SetError("No anims found");

	const std::span<BGSKeyword*> span(keywords.entries, keywords.count);
	std::vector<std::pair<const char*, UInt32>> searchResult;
	std::for_each(span.begin(), span.end(), [&searchResult](BGSKeyword* keyword) {
		if (keyword)
			searchResult.push_back(std::make_pair(keyword->keyword.c_str(), keyword->formID));
	});

	std::sort(searchResult.begin(), searchResult.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.first, rhs.first) < 0;
	});

	result.CreateMenuItems();
	for (auto& [edid, formId] : searchResult) {
		result.PushItem(edid, formId);
	}
}
GFxReq getTypedKeywords("GetAnimFaceArchetypes", [](auto& result, auto args) {
	GetTypedKeywords(result, KeywordType::AnimFace);
});

typedef bool(*_ChangeAnimArchetype)(TESObjectREFR* refr, BGSKeyword* keyword);
RelocAddr<_ChangeAnimArchetype> ChangeAnimArchetype(0x0CA5D20);
typedef void(*_ChangeFaceArchetypeExpression)(TESObjectREFR* refr, BGSKeyword* keyword);
RelocAddr<_ChangeFaceArchetypeExpression> ChangeFaceArchetypeExpression(0xDB93B0);

void SetAnimFaceArchetype(GFxResult& result, UInt32 formId) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return result.SetError("Could not find animfacearchetype");

	BGSKeyword* keyword = DYNAMIC_CAST(form, TESForm, BGSKeyword);
	if (!keyword)
		return result.SetError("Could not find animfacearchetype");

	ChangeFaceArchetypeExpression(selected.refr, keyword);
}
GFxReq setAnimFaceArchetype("SetAnimFaceArchetype", [](auto& result, auto args) {
	SetAnimFaceArchetype(result, args->args[1].GetUInt());
});
GFxReq resetAnimFaceArchetype("ResetAnimFaceArchetype", [](auto& result, auto args) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	ChangeFaceArchetypeExpression(selected.refr, nullptr);
});