#pragma once

#include "f4se/GameForms.h"

#include <vector>
#include <span>

typedef std::vector<std::pair<const char*, UInt32>> FormSearchResult;
typedef std::vector<std::span<TESForm*>> FormsSpan;

template <class T>
std::span<TESForm*> MakeFormsSpan(tArray<T*> forms) {
	return std::span(reinterpret_cast<TESForm**>(forms.entries), forms.count);
};

void SortSearchResult(FormSearchResult& result);

void GetModVectors(std::vector<bool>& esp, std::vector<bool>& esl);
void AddModVectorsToList(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<std::string>& result);
void AddModVectorsToListNoMasters(std::vector<bool>& esp, std::vector<bool>& esl, std::vector<const char*>& result);

void SearchFormsSpanForMods(const FormsSpan& span, std::vector<std::string>& result);
void SearchSpanForMods(const std::span<TESForm*>& span, std::vector<std::string>& result);

void SearchFormsForSubstring(FormSearchResult& searchResult, const FormsSpan& span, const char* search);
void SearchSpanForSubstring(FormSearchResult& searchResult, const std::span<TESForm*>& span, const char* search);