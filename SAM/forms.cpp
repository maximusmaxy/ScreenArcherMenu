#include "forms.h"

#include "f4se/NiNodes.h"
#include "strnatcmp.h"
#include "SAF/types.h"

#include <ranges>

RelocAddr<_GetFormByEditorId> GetFormByEditorId(0x152EB0);
RelocAddr<_GetREFRFromHandle> GetREFRFromHandle(0xAC90);

RelocAddr<_PlaceAtMeInternal> PlaceAtMeInternal(0x5121D0);

RelocAddr<_SetREFRLocation> SetREFRLocation(0x40C060);
RelocAddr<_SetREFROrientation> SetREFROrientation(0x408700);
RelocAddr<_SetREFRScale> SetREFRScale(0x3F85B0);
RelocAddr<_SetActorPosition> SetActorPosition(0xD77690);

RelocAddr<_QueueActorUpdateModel> QueueActorUpdateModel(0xD59AA0);
RelocPtr<UInt64> taskQueueInterface(0x5AC64F0);

void SetREFRTransform(TESObjectREFR* refr, const NiTransform& transform) {
	SetREFRLocation(refr, transform.pos);
	SetREFROrientation(refr, transform.rot);
	SetREFRScale(refr, transform.scale);
}

void FormSearchResult::Push(const char* name, UInt32 formId) {
	std::lock_guard lock(mutex);
	result.push_back(std::make_pair(name, formId));
}

void FormSearchResult::Sort() {
	std::sort(result.begin(), result.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.first, rhs.first) < 0;
	});
}

void FormSearchResult::Sort(vec::iterator start, vec::iterator end) {
	std::sort(start, end, [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.first, rhs.first) < 0;
	});
}

void GetModIndexAndMask(const ModInfo* modInfo, UInt32* modIndex, UInt32* mask)
{
	if (modInfo->IsLight()) {
		*modIndex = (0xFE000000 | (modInfo->lightIndex << 12));
		*mask = 0xFFFFF000;
	}
	else {
		*modIndex = modInfo->modIndex << 24;
		*mask = 0xFF000000;
	}
}

void GetModIndexAndFormMask(const ModInfo* modInfo, UInt32* modIndex, UInt32* mask)
{
	if (modInfo->IsLight()) {
		*modIndex = (0xFE000000 | (modInfo->lightIndex << 12));
		*mask = 0x00000FFF;
	}
	else {
		*modIndex = modInfo->modIndex << 24;
		*mask = 0x00FFFFFF;
	}
}

void AddFormattedModsToResult(GFxResult& result, const std::vector<const char*>& searchResult) {
	result.CreateMenuItems();

	for (auto& mod : searchResult) {
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

void GetModVectors(std::vector<bool>& esp, std::vector<bool>& esl)
{
	//creating two fixed length boolean vectors to store mods with items
	size_t last = 0;
	auto end = (*g_dataHandler)->modList.loadedMods.entries + (*g_dataHandler)->modList.loadedMods.count;
	for (auto it = (*g_dataHandler)->modList.loadedMods.entries; it < end; ++it) {
		if ((*it)->modIndex > last)
			last = (*it)->modIndex;
	}
	esp.resize(last + 1, false);

	last = 0;
	end = (*g_dataHandler)->modList.lightMods.entries + (*g_dataHandler)->modList.lightMods.count;
	for (auto it = (*g_dataHandler)->modList.lightMods.entries; it < end; ++it) {
		if ((*it)->lightIndex > last)
			last = (*it)->lightIndex;
	}
	esl.resize(last + 1, false);
}

void AddModVectorsToList(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<const char*>& result)
{
	//Collect the names of available mods
	auto end = (*g_dataHandler)->modList.loadedMods.entries + (*g_dataHandler)->modList.loadedMods.count;
	for (auto it = (*g_dataHandler)->modList.loadedMods.entries; it < end; ++it) {
		if (esp[(*it)->modIndex])
			result.push_back((*it)->name);
	}

	end = (*g_dataHandler)->modList.lightMods.entries + (*g_dataHandler)->modList.lightMods.count;
	for (auto it = (*g_dataHandler)->modList.lightMods.entries; it < end; ++it) {
		if (esl[(*it)->lightIndex])
			result.push_back((*it)->name);
	}

	std::sort(result.begin(), result.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs, rhs) < 0;
	});
}

void AddModVectorsToListNoMasters(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<const char*>& result)
{
	const SAF::InsensitiveStringSet idleExclude = {
		"Fallout4.esm",
		"DLCRobot.esm",
		"DLCworkshop01.esm",
		"DLCworkshop02.esm",
		"DLCworkshop03.esm",
		"DLCCoast.esm",
		"DLCNukaWorld.esm",
		//"ScreenArcherMenu.esp",
	};

	auto it = (*g_dataHandler)->modList.loadedMods.entries;
	auto end = (*g_dataHandler)->modList.loadedMods.entries + (*g_dataHandler)->modList.loadedMods.count;

	//Skip base game mods
	while (it != end && idleExclude.count((*it)->name))
		++it;

	for (; it != end; ++it) {
		if (esp[(*it)->modIndex])
			result.push_back((*it)->name);
	}

	end = (*g_dataHandler)->modList.lightMods.entries + (*g_dataHandler)->modList.lightMods.count;
	for (it = (*g_dataHandler)->modList.lightMods.entries; it != end; ++it) {
		if (esl[(*it)->lightIndex])
			result.push_back((*it)->name);
	}

	std::sort(result.begin(), result.end(), [](const char* lhs, const char* rhs) {
		return strnatcasecmp(lhs, rhs) < 0;
	});
}

void SearchFormsSpanForMods(const FormsSpan& span, std::vector<const char*>& result) {
	std::vector<bool> esp;
	std::vector<bool> esl;
	GetModVectors(esp, esl);
	auto espIt = esp.begin();
	auto eslIt = esl.begin();

	auto range = std::ranges::join_view(span);
	std::for_each(std::execution::par, range.begin(), range.end(), [&espIt, &eslIt](TESForm* form) {
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

void SearchFormsForSubstring(FormSearchResult& searchResult, const FormsSpan& span, const char* search) {
	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	auto range = std::ranges::join_view(span);
	std::for_each(std::execution::par, range.begin(), range.end(), [&lowered, &searchResult](TESForm* form) {
		if (form) {
			auto fullname = form->GetFullName();
			if (fullname && *fullname && HasInsensitiveSubstring(fullname, lowered)) {
				searchResult.Push(fullname, form->formID);
			}
		}
	});

	searchResult.Sort();
}

void SearchFormEdidsForSubstring(FormSearchResult& searchResult, const FormsSpan& span, const char* search) {
	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	auto range = std::ranges::join_view(span);
	std::for_each(std::execution::par, range.begin(), range.end(), [&lowered, &searchResult](TESForm* form) {
		if (form) {
			auto edid = form->GetEditorID();
			if (edid && *edid && HasInsensitiveSubstring(edid, lowered)) {
				searchResult.Push(edid, form->formID);
			}
		}
	});

	searchResult.Sort();
}

void SearchSpanForSubstring(FormSearchResult& searchResult, const std::span<TESForm*>& span, const char* search) {
	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	std::for_each(std::execution::par, span.begin(), span.end(), [&lowered, &searchResult](TESForm* form) {
		if (form) {
			auto fullname = form->GetFullName();
			if (fullname && *fullname && HasInsensitiveSubstring(fullname, lowered)) {
				searchResult.Push(fullname, form->formID);
			}
		}
	});

	searchResult.Sort();
}