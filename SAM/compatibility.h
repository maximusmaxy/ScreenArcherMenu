#pragma once

#include "f4se/PluginAPI.h"

enum FfcType {
	kFfcKeyboard = 1,
	kFfcPad
};

void RegisterCompatibility(F4SEInterface* ifc);
bool GetFfcLock(FfcType type);
void LockFfc(bool locked);
float GetCameraRoll();
void SetCameraRoll(float roll);