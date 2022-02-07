#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

int GetBlinkState();
void SetBlinkState(bool blink);
int GetForceMorphUpdate();
void SetForceMorphUpdate(bool update);
int GetDisableEyecoordUpdate();
void SetDisableEyecoordUpdate(bool disable);
void GetHacksGFx(GFxMovieRoot* root, GFxValue* hacks);