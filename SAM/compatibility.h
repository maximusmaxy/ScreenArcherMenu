#pragma once

enum FfcType {
	kFfcKeyboard = 1,
	kFfcPad
};

bool RegisterFfcCompatiblity();
bool GetFfcLock(FfcType type);
void LockFfc(bool locked);