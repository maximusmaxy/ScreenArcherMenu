#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

enum {
	kHackDisabled = 0,
	kHackEnabled = 1,
	kHackUnknown = 2
};

int GetBlinkState();
void SetBlinkState(bool blink);
int GetForceMorphUpdate();
void SetForceMorphUpdate(bool update);
int GetDisableEyecoordUpdate();
void SetDisableEyecoordUpdate(bool disable);