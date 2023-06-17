#include "forms.h"

#include "f4se/GameData.h"

#include "strnatcmp.h"
#include "SAF/util.h"
#include "SAF/types.h"

#include <ranges>
#include <span>
#include <algorithm>
#include <execution>

void SortSearchResult(FormSearchResult& result) {
	std::sort(result.begin(), result.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.first, rhs.first) < 0;
	});
}

void GetModVectors(std::vector<bool>& esp, std::vector<bool>& esl)
{
	//creating two fixed length boolean vectors to store mods with items
	int last = 0;
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

void AddModVectorsToList(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<std::string>& result)
{
	//Collect the names of available mods
	auto end = (*g_dataHandler)->modList.loadedMods.entries + (*g_dataHandler)->modList.loadedMods.count;
	for (auto it = (*g_dataHandler)->modList.loadedMods.entries; it < end; ++it) {
		if (esp[(*it)->modIndex])
			result.emplace_back((*it)->name);
	}

	end = (*g_dataHandler)->modList.lightMods.entries + (*g_dataHandler)->modList.lightMods.count;
	for (auto it = (*g_dataHandler)->modList.lightMods.entries; it < end; ++it) {
		if (esl[(*it)->lightIndex])
			result.emplace_back((*it)->name);
	}

	std::sort(result.begin(), result.end(), [](auto& lhs, auto& rhs) {
		return strnatcasecmp(lhs.c_str(), rhs.c_str()) < 0;
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

void SearchFormsSpanForMods(const FormsSpan& span, std::vector<std::string>& result) {
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

void SearchSpanForMods(const std::span<TESForm*>& span, std::vector<std::string>& result) {
	std::vector<bool> esp;
	std::vector<bool> esl;
	GetModVectors(esp, esl);
	auto espIt = esp.begin();
	auto eslIt = esl.begin();

	std::for_each(std::execution::par, span.begin(), span.end(), [&espIt, &eslIt](TESForm* form) {
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

	std::mutex mutex;
	auto range = std::ranges::join_view(span);
	std::for_each(std::execution::par, range.begin(), range.end(), [&lowered, &searchResult, &mutex](TESForm* form) {
		if (form && HasInsensitiveSubstring(form->GetFullName(), lowered)) {
			std::lock_guard lock(mutex);
			searchResult.emplace_back(form->GetFullName(), form->formID);
		}
	});

	SortSearchResult(searchResult);
}

void SearchSpanForSubstring(FormSearchResult& searchResult, const std::span<TESForm*>& span, const char* search) {
	char lowered[MAX_PATH];
	GetLoweredCString(lowered, search);

	std::mutex mutex;
	std::for_each(std::execution::par, span.begin(), span.end(), [&lowered, &searchResult, &mutex](TESForm* form) {
		if (form && HasInsensitiveSubstring(form->GetFullName(), lowered)) {
			std::lock_guard lock(mutex);
			searchResult.emplace_back(form->GetFullName(), form->formID);
		}
	});

	SortSearchResult(searchResult);
}
