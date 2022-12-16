#include "compatibility.h"

#include "SAF/util.h"
#include "SAF/conversions.h"

using namespace SAF;

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusUtilities.h"
#include "f4se/GameMenus.h"
#include "f4se_common/f4se_version.h"

#include "sam.h"

bool** ffcKeyboardInput = nullptr;
bool** ffcPadInput = nullptr;

float* pmCameraRoll = nullptr;
float* ffcCameraRoll = nullptr;

void RegisterCompatibility(F4SEInterface* ifc)
{
	//Only register compatibility on f4se version 0.6.23 or greater
	if (ifc->f4seVersion >= MAKE_EXE_VERSION(0, 6, 23)) {

		//FFC version EXACTLY V12
		const PluginInfo* ffcInfo = ifc->GetPluginInfo("Free Fly Cam");
		if (ffcInfo && ffcInfo->version == 12) {

			UInt64 ffcHandle = (UInt64)GetModuleHandle("FreeFlyCam Fo4.dll");

			if (ffcHandle) {
				ffcKeyboardInput = (bool**)(ffcHandle + 0x3E2B8);
				ffcPadInput = (bool**)(ffcHandle + 0x3E2C0);
				ffcCameraRoll = (float*)(ffcHandle + 0x3E3D8);
			}
		}

		//Photo mode v1.03
		const PluginInfo* pmInfo = ifc->GetPluginInfo("f4pm");
		if (pmInfo && pmInfo->version == 1) {

			UInt64 pmHandle = (UInt64)GetModuleHandle("f4pm.dll");

			if (pmHandle) {
				pmCameraRoll = (float*)(pmHandle + 0x5CF48);
			}
		}
	}
}

//Start of public variables
#define FFCINPUTPUBLIC 0x16C
//Lock boolean offset
#define FFCINPUTLOCK 0x160

bool GetFfcLock(bool** input) {
	if (!*input)
		return false;

	return *(*input + FFCINPUTPUBLIC + FFCINPUTLOCK);
}

bool GetFfcLock(FfcType type) {
	return GetFfcLock(type == kFfcKeyboard ? ffcKeyboardInput : ffcPadInput);
}

void SetFfcLock(bool** input, bool locked) {
	if (!*input)
		return;

	*(*input + FFCINPUTPUBLIC + FFCINPUTLOCK) = locked;

	//need to zero the public variables if locking
	if (locked)
		memset(*input + FFCINPUTPUBLIC, 0, FFCINPUTLOCK);
}

bool keyboardLocked = false;
bool padLocked = false;

void LockFfc(bool locked)
{
	if (!ffcKeyboardInput || !ffcPadInput)
		return;

	if (locked) {
		//store previous state
		keyboardLocked = GetFfcLock(ffcKeyboardInput);
		padLocked = GetFfcLock(ffcPadInput);

		SetFfcLock(ffcKeyboardInput, true);
		SetFfcLock(ffcPadInput, true);
	}
	else {
		//restore previous state
		SetFfcLock(ffcKeyboardInput, keyboardLocked);
		SetFfcLock(ffcPadInput, padLocked);
	}
}

float GetCameraRoll()
{
	if (ffcCameraRoll)
	{
		return *ffcCameraRoll;
	}
	else if (pmCameraRoll)
	{
		return *pmCameraRoll;
	}
	return 0.0f;
}

void SetCameraRoll(float roll)
{
	if (ffcCameraRoll)
	{
		*ffcCameraRoll = roll;
	}
	else if (pmCameraRoll)
	{
		*pmCameraRoll = roll;
	}
}