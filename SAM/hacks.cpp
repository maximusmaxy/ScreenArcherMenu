#include "hacks.h"

#include "f4se/GameReferences.h"
#include "f4se/NiObjects.h"
#include "f4se/NiMaterials.h"

#include "f4se_common/Relocation.h"
#include "f4se_common/SafeWrite.h"

//Atomic versions of the memcpy safe writes if aligned (I hope)
void SafeWriteUInt16(uintptr_t addr, UInt16 data)
{
	UInt32	oldProtect;

	VirtualProtect((void *)addr, 2, PAGE_EXECUTE_READWRITE, &oldProtect);
	(*(UInt16*)addr) = data;
	VirtualProtect((void *)addr, 2, oldProtect, &oldProtect);
}

void SafeWriteUInt32(uintptr_t addr, UInt32 data)
{
	UInt32	oldProtect;

	VirtualProtect((void *)addr, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	(*(UInt32*)addr) = data;
	VirtualProtect((void *)addr, 4, oldProtect, &oldProtect);
}

//RelocAddr<bool> blinkByte(0x6860E1);
RelocPtr<float> fFGBlinkClosedValue(0x37333E0);
RelocPtr<UInt32> uFGBlinkLeftEyeMorphIndex(0x37333F8);
RelocPtr<UInt32> uFGBlinkRightEyeMorphIndex(0x3733410);

UInt32 leftEyeIndex = 0x12;
UInt32 rightEyeIndex = 0x29;
UInt32 emptyIndex = 0x35; 

int GetBlinkState()
{
	//0 false, 1 true, 2 unknown
	int state = 2;
	if (*uFGBlinkLeftEyeMorphIndex == emptyIndex && *uFGBlinkRightEyeMorphIndex == emptyIndex) {
		state = 1;
	} else if (*uFGBlinkLeftEyeMorphIndex == leftEyeIndex && *uFGBlinkRightEyeMorphIndex == rightEyeIndex) {
		state = 0;
	}
	return state;
}

void SetBlinkState(bool disable)
{
	if (disable) {
		*fFGBlinkClosedValue = 0.0;
		*uFGBlinkLeftEyeMorphIndex = emptyIndex;
		*uFGBlinkRightEyeMorphIndex = emptyIndex;
	} else {
		*fFGBlinkClosedValue = 1.0;
		*uFGBlinkLeftEyeMorphIndex = leftEyeIndex;
		*uFGBlinkRightEyeMorphIndex = rightEyeIndex;
	}
	//SafeWrite8(blinkByte.GetUIntPtr(), blink);
}

RelocPtr<UInt16> blinkUpdateAddress(0x668B32);
	
//initial part of jz check 0F 84
UInt16 blinkUpdateInstruction = 0x840F;
//jmp relative 3 bytes EB 03
UInt16 skipBlinkUpdateInstruction = 0x03EB;

int GetForceMorphUpdate()
{
	//0 false, 1 true, 2 unknown
	int state = 2;
	if (*blinkUpdateAddress == blinkUpdateInstruction) {
		state = 0;
	} else if (*blinkUpdateAddress == skipBlinkUpdateInstruction) {
		state = 1;
	}
	return state;
}

void SetForceMorphUpdate(bool force)
{
	if (force)
		SafeWriteUInt16(blinkUpdateAddress.GetUIntPtr(), skipBlinkUpdateInstruction);
	else
		SafeWriteUInt16(blinkUpdateAddress.GetUIntPtr(), blinkUpdateInstruction);
}

RelocPtr<UInt32> eyecoordUpdateAddress(0x9C0744);

//Initial part of jz check 0F 84 AB 02
UInt32 updateEyecoordInstruction = 0x02AB840F;
//jmp relative 2AC bytes E9 AC 02 00 00
UInt32 skipUpdateEyecoordInstruction = 0x0002ACE9;

int GetDisableEyecoordUpdate()
{
	//0 false, 1 true, 2 unknown
	int state = 2;
	if (*eyecoordUpdateAddress == updateEyecoordInstruction) {
		state = 0;
	} else if (*eyecoordUpdateAddress == skipUpdateEyecoordInstruction) {
		state = 1;
	}
	return state;
}

void SetDisableEyecoordUpdate(bool disable)
{
	if (disable)
		SafeWriteUInt32(eyecoordUpdateAddress.GetUIntPtr(), skipUpdateEyecoordInstruction);
	else
		SafeWriteUInt32(eyecoordUpdateAddress.GetUIntPtr(), updateEyecoordInstruction);
}

void GetHacksGFx(GFxMovieRoot* root, GFxValue* hacks)
{
	root->CreateArray(hacks);
	GFxValue blinkState(GetBlinkState() == 1);
	hacks->PushBack(&blinkState);

	GFxValue forceMorphUpdate(GetForceMorphUpdate() == 1);
	hacks->PushBack(&forceMorphUpdate);

	GFxValue eyeState(GetDisableEyecoordUpdate() == 1);
	hacks->PushBack(&eyeState);
}