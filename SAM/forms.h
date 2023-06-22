#pragma once

#include "f4se/GameData.h"
#include "f4se/GameForms.h"
#include "f4se/GameReferences.h"
#include "f4se/NiTypes.h"
#include "f4se_common/Relocation.h"

#include "SAF/util.h"
#include "gfx.h"

#include <vector>
#include <span>
#include <mutex>
#include <algorithm>
#include <execution>

//out, actor, form, 1, 0, 0, false, false
typedef UInt32* (*_PlaceAtMeInternal)(TESObjectREFR** out, TESObjectREFR* actor, TESForm* form, int unk4, int unk5, int unk6, bool unk7, bool unk8);
extern RelocAddr<_PlaceAtMeInternal> PlaceAtMeInternal;
typedef void (*_GetREFRFromHandle)(UInt32* handle, NiPointer<TESObjectREFR>& refr);
extern RelocAddr<_GetREFRFromHandle> GetREFRFromHandle;
typedef void(*_SetREFRLocation)(TESObjectREFR* refr, const NiPoint3& point);
extern RelocAddr<_SetREFRLocation> SetREFRLocation;
typedef void(*_SetREFROrientation)(TESObjectREFR* refr, const NiMatrix43& matrix);
extern RelocAddr<_SetREFROrientation> SetREFROrientation;
typedef void(*_SetREFRScale)(TESObjectREFR* refr, float scale);
extern RelocAddr<_SetREFRScale> SetREFRScale;
typedef void(*_SetActorPosition)(TESObjectREFR* actor, const NiPoint3& point, bool havok);
extern RelocAddr<_SetActorPosition> SetActorPosition;

typedef void (*_QueueActorUpdateModel)(UInt64 taskQueueInterface, Actor* actor, UInt16 flags, float unk);
extern RelocAddr<_QueueActorUpdateModel> QueueActorUpdateModel;
extern RelocPtr<UInt64> taskQueueInterface;

void SetREFRTransform(TESObjectREFR* refr, const NiTransform& transform);

class FormSearchResult {
private:
	std::mutex mutex;
public:
	std::vector<std::pair<const char*, UInt32>> result;
	void Push(const char* name, UInt32 formId);
	void Sort();
};

typedef std::vector<std::span<TESForm*>> FormsSpan;

template <class T>
std::span<TESForm*> MakeFormsSpan(tArray<T*> forms) {
	return std::span(reinterpret_cast<TESForm**>(forms.entries), forms.count);
};

void GetModIndexAndMask(const ModInfo* modInfo, UInt32* modIndex, UInt32* mask);
void AddFormattedModsToResult(GFxResult& result, const std::vector<const char*>& searchResult);

void GetModVectors(std::vector<bool>& esp, std::vector<bool>& esl);
void AddModVectorsToList(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<const char*>& result);
void AddModVectorsToListNoMasters(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<const char*>& result);

void SearchFormsSpanForMods(const FormsSpan& span, std::vector<const char*>& result);

template <class T>
void SearchSpanForMods(const std::span<T*>& span, std::vector<const char*>& result)
{
	std::vector<bool> esp;
	std::vector<bool> esl;
	GetModVectors(esp, esl);
	auto espIt = esp.begin();
	auto eslIt = esl.begin();

	std::for_each(std::execution::par, span.begin(), span.end(), [&espIt, &eslIt](T* form) {
		if (form) {
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

void SearchFormsForSubstring(FormSearchResult& searchResult, const FormsSpan& span, const char* search);
void SearchFormEdidsForSubstring(FormSearchResult& searchResult, const FormsSpan& span, const char* search);
void SearchSpanForSubstring(FormSearchResult& searchResult, const std::span<TESForm*>& span, const char* search);

template <class T>
void SearchSpanForEdidSubstring(FormSearchResult& searchResult, const std::span<T*>& span, const char* search, bool sort = true) {
	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	std::for_each(std::execution::par, span.begin(), span.end(), [&lowered, &searchResult](T* form) {
		if (form) {
			auto edid = form->GetEditorID();
			if (edid && *edid && HasInsensitiveSubstring(edid, lowered)) {
				searchResult.Push(edid, form->formID);
			}
		}
	});

	if (sort)
		searchResult.Sort();
}

template <class T>
void SearchModForFullnameForms(const ModInfo* info, std::span<T*>& span, FormSearchResult& result) {
	UInt32 modIndex;
	UInt32 modMask;
	GetModIndexAndMask(info, &modIndex, &modMask);

	std::for_each(std::execution::par, span.begin(), span.end(), [&](T* form) {
		if (form && ((form->formID & modMask) == modIndex)) {
			auto fullname = form->GetFullName();
			if (fullname && *fullname) {
				result.Push(fullname, form->formID);
			}
		}
	});

	result.Sort();
}

template <class T>
void SearchModForEdidForms(const ModInfo* info, std::span<T*>& span, FormSearchResult& result) {
	UInt32 modIndex;
	UInt32 modMask;
	GetModIndexAndMask(info, &modIndex, &modMask);

	std::for_each(std::execution::par, span.begin(), span.end(), [&](T* form) {
		if (form && ((form->formID & modMask) == modIndex)) {
			auto edid = form->GetEditorID();
			if (edid && *edid) {
				result.Push(edid, form->formID);
			}
		}
	});

	result.Sort();
}