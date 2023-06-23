#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformTypes.h"
#include "f4se/ScaleformTranslator.h"

#include "gfx.h"

typedef void(*_ScaleformRefCountImplAddRef)(GRefCountImplCore*);
extern RelocAddr<_ScaleformRefCountImplAddRef> ScaleformRefCountImplAddRef;
typedef void(*_ScaleformRefCountImplRelease)(GRefCountImplCore*);
extern RelocAddr<_ScaleformRefCountImplRelease> ScaleformRefCountImplRelease;

class Translator {
public:
	BSScaleformTranslator* translator;
	Translator(GFxMovieRoot* root);
	~Translator();
	const BSScaleformTranslator* operator->() { return translator; }
};

void GetTranslation(GFxMovieRoot* root, GFxValue* result, const char* str);
void GetTranslations(GFxMovieRoot* root, GFxValue* arr);
void FilterMenuNamesBySubstring(GFxMovieRoot* root, GFxValue* names, const char* search, GFxValue* result);

bool GetMenusHidden();
void SetMenusHidden(bool hidden);
bool ToggleMenusHidden();