#include "scaleform.h"

#include "f4se/GameMenus.h"
#include "f4se/NiTextures.h"
#include "f4se/ScaleformLoader.h"
#include "f4se_common/Relocation.h"
#include "SAF/util.h"

RelocAddr<_ScaleformRefCountImplAddRef> ScaleformRefCountImplAddRef(0x210EBF0);
RelocAddr<_ScaleformRefCountImplRelease> ScaleformRefCountImplRelease(0x210EC90);

Translator::Translator(GFxMovieRoot* root) {
	auto stateBag = (GFxStateBag*)(*((UInt64*)root + 0x2) + 0x10);
	translator = (BSScaleformTranslator*)stateBag->GetStateAddRef(GFxState::kInterface_Translator);
}

Translator::~Translator() {
	ScaleformRefCountImplRelease(translator);
}

void GetTranslation(GFxMovieRoot* root, GFxValue* result, const char* str) {
	if (!str)
		return result->SetString("");
	if (*str != '$')
		return result->SetString(str);

	Translator translator(root);

	std::wstring wide;
	CStringToWide(str, wide);
	BSFixedString wideStr(wide.c_str());
	auto translation = translator->translations.Find(&wideStr);
	if (!translation)
		return result->SetString(str);

	std::string managed;
	WideToString(translation->translation.wc_str(), managed);
	root->CreateString(result, managed.c_str());
}

void GetTranslations(GFxMovieRoot* root, GFxValue* arr) {
	Translator translator(root);
	std::wstring wide;
	std::string managed;

	auto size = arr->GetArraySize();
	for (UInt32 i = 0; i < size; ++i) {
		GFxValue element;
		arr->GetElement(i, &element);
		auto str = element.GetString();
		if (str && *str == '$') {
			CStringToWide(str, wide);
			BSFixedString wideStr(wide.c_str());
			auto translation = translator->translations.Find(&wideStr);
			if (translation) {
				WideToString(translation->translation.wc_str(), managed);
				root->CreateString(&element, managed.c_str());
			}
		}
	}
}

void FilterMenuNamesBySubstring(GFxMovieRoot* root, GFxValue* names, const char* search, GFxValue* result) {
	root->CreateArray(result);
	if (!search)
		return;

	//Ignore first slash
	if (*search == '/')
		search++;

	//if empty just grab everything
	auto size = names->GetArraySize();
	if (!*search) {
		for (SInt32 i = 0; i < size; ++i) {
			GFxValue idx(i);
			result->PushBack(&idx);
		}
		return;
	}

	char searchTerm[MAX_PATH];
	GetLoweredCString(searchTerm, search);

	Translator translator(root);
	std::wstring wide;
	
	for (SInt32 i = 0; i < size; ++i) {
		GFxValue name;
		names->GetElement(i, &name);
		const char* nameStr = name.GetString();
		if (nameStr[0] == '$') {

			BSFixedString wideStr(wide.c_str());
			auto translation = translator->translations.Find(&wideStr);
			if (translation) {
				auto wideTranslation = translation->translation.wc_str();
				if (wideTranslation && *wideTranslation && HasInsensitiveSubstring(wideTranslation, searchTerm)) {
					GFxValue idx(i);
					result->PushBack(&idx);
				}
			}
			else if (*nameStr && HasInsensitiveSubstring(nameStr, searchTerm)) {
				GFxValue idx(i);
				result->PushBack(&idx);
			}
		}
		else {
			if (*nameStr && HasInsensitiveSubstring(nameStr, searchTerm)) {
				GFxValue idx(i);
				result->PushBack(&idx);
			}
		}
	}
}

typedef void(*_ToggleMenusInternal)();
RelocAddr<_ToggleMenusInternal> ToggleMenusInternal(0x517FF0);

typedef void(*_SetMenusDisabledInternal)(bool hidden);
RelocAddr<_SetMenusDisabledInternal> SetMenusDisabledInternal(0xAE5BB0);

#define uiVisible (reinterpret_cast<bool*>(g_ui.GetUIntPtr()) + 0x248)

bool GetMenusHidden() {
	return !(*uiVisible);
}

void SetMenusHidden(bool hidden) {
	if (hidden != GetMenusHidden())
		ToggleMenusInternal();
}

bool ToggleMenusHidden() {
	ToggleMenusInternal();
	return GetMenusHidden();
}