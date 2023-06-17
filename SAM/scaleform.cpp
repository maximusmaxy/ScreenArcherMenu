#include "scaleform.h"

#include "f4se/NiTextures.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/ScaleformTranslator.h"
#include "SAF/util.h"

RelocPtr<BSScaleformManager> scaleformManager(0x58DE410);

void FilterMenuNamesBySubstring(GFxMovieRoot* root, GFxValue* names, const char* search, GFxValue* result)
{
	root->CreateArray(result);
	if (!search)
		return;

	//Ignore first slash
	if (*search == '/')
		search++;

	char searchTerm[MAX_PATH];
	GetLoweredCString(searchTerm, search);

	BSScaleformTranslator* translator = (BSScaleformTranslator*)scaleformManager->stateBag->GetStateAddRef(GFxState::kInterface_Translator);
	std::wstring wide;

	auto size = names->GetArraySize();
	for (int i = 0; i < size; ++i) {
		GFxValue name;
		names->GetElement(i, &name);
		const char* nameStr = name.GetString();
		if (nameStr[0] == '$') {
			auto len = strlen(nameStr);
			wide.resize(len);
			for (int c = 0; c < len; ++c) {
				wide[c] = (wchar_t)nameStr[c];
			}
			BSFixedString wideStr(wide.c_str());
			auto translation = translator->translations.Find(&wideStr);
			if (HasInsensitiveSubstring(translation->translation.wc_str(), searchTerm)) {
				GFxValue idx(i);
				result->PushBack(&idx);
			}
		}
		else {
			if (!*searchTerm || HasInsensitiveSubstring(nameStr, searchTerm)) {
				GFxValue idx(i);
				result->PushBack(&idx);
			}
		}
	}
}