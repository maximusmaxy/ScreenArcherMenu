#include "hacks.h"

#include "f4se/GameReferences.h"
#include "f4se/NiObjects.h"
#include "f4se/NiMaterials.h"

#include "f4se_common/Relocation.h"
#include "f4se_common/SafeWrite.h"

template <class T>
void SafeWriteT(uintptr_t addr, T data)
{
	UInt32	oldProtect;

	VirtualProtect((void *)addr, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect);
	*(T*)addr = data;
	VirtualProtect((void *)addr, sizeof(T), oldProtect, &oldProtect);
}

RelocPtr<UInt64> leftBlinkUpdateAddress(0x668CE0);
RelocPtr<UInt64> rightBlinkUpdateAddress(0x668D08);

UInt64 leftBlinkUpdateInstruction = 0x188744110FF3C028;
UInt64 leftBlinkUpdateSkip = 0x0000441F0F66C028;

UInt64 rightBlinkUpdateInstruction = 0x188744110FF3C128;
UInt64 rightBlinkUpdateSkip = 0x0000441F0F66C128;

int GetBlinkState()
{
	//0 false, 1 true, 2 unknown
	int state = 2;
	if (*leftBlinkUpdateAddress == leftBlinkUpdateSkip && *rightBlinkUpdateAddress == rightBlinkUpdateSkip) {
		state = 1;
	} else if (*leftBlinkUpdateAddress == leftBlinkUpdateInstruction && *rightBlinkUpdateAddress == rightBlinkUpdateInstruction) {
		state = 0;
	}
	return state;
}

void SetBlinkState(bool disable)
{
	if (disable) {
		SafeWriteT<UInt64>(leftBlinkUpdateAddress.GetUIntPtr(), leftBlinkUpdateSkip);
		SafeWriteT<UInt64>(rightBlinkUpdateAddress.GetUIntPtr(), rightBlinkUpdateSkip);
	} else {
		SafeWriteT<UInt64>(leftBlinkUpdateAddress.GetUIntPtr(), leftBlinkUpdateInstruction);
		SafeWriteT<UInt64>(rightBlinkUpdateAddress.GetUIntPtr(), rightBlinkUpdateInstruction);
	}
}

RelocPtr<UInt16> morphUpdateAddress(0x668B32);
	
//initial part of jz check 0F 84
UInt16 morphUpdateInstruction = 0x840F;
//jmp relative 3 bytes EB 03
UInt16 skipMorphUpdateInstruction = 0x03EB;

int GetForceMorphUpdate()
{
	//0 false, 1 true, 2 unknown
	int state = 2;
	if (*morphUpdateAddress == morphUpdateInstruction) {
		state = 0;
	} else if (*morphUpdateAddress == skipMorphUpdateInstruction) {
		state = 1;
	}
	return state;
}

void SetForceMorphUpdate(bool force)
{
	if (force)
		SafeWriteT<UInt16>(morphUpdateAddress.GetUIntPtr(), skipMorphUpdateInstruction);
	else
		SafeWriteT<UInt16>(morphUpdateAddress.GetUIntPtr(), morphUpdateInstruction);
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
		SafeWriteT<UInt32>(eyecoordUpdateAddress.GetUIntPtr(), skipUpdateEyecoordInstruction);
	else
		SafeWriteT<UInt32>(eyecoordUpdateAddress.GetUIntPtr(), updateEyecoordInstruction);
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