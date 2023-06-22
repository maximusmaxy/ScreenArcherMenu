#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformTypes.h"
#include "f4se/ScaleformTranslator.h"

typedef void(*_ScaleformRefCountImplAddRef)(GRefCountImplCore*);
extern RelocAddr<_ScaleformRefCountImplAddRef> ScaleformRefCountImplAddRef;
typedef void(*_ScaleformRefCountImplRelease)(GRefCountImplCore*);
extern RelocAddr<_ScaleformRefCountImplRelease> ScaleformRefCountImplRelease;

class TranslatorWrapper {
public:
	BSScaleformTranslator* translator{};
	TranslatorWrapper(GFxMovieRoot* root);
	~TranslatorWrapper();
};

void FilterMenuNamesBySubstring(GFxMovieRoot* root, GFxValue* names, const char* search, GFxValue* result);

bool GetMenusHidden();
void SetMenusHidden(bool hidden);
bool ToggleMenusHidden();