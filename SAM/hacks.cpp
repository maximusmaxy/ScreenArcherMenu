#include "hacks.h"

#include "SAF/hacks.h"

void GetHacks(GFxResult& result)
{
	result.CreateValues();
	result.PushValue(GetBlinkState() == 1);
	result.PushValue(GetForceMorphUpdate() == 1);
	result.PushValue(GetDisableEyecoordUpdate() == 1);
}

enum {
	kHackBlink = 0,
	kHackMorphs,
	kHackEyeCoords
};

void SetHack(SInt32 index, bool enabled)
{
	switch (index) {
	case kHackBlink: SetBlinkState(enabled);
	case kHackMorphs: SetForceMorphUpdate(enabled);
	case kHackEyeCoords: SetDisableEyecoordUpdate(enabled);
	}
}

//RelocPtr<UInt8> cursorFlags(0x126ABA8);
//Always on 0x02
//Default 0x00