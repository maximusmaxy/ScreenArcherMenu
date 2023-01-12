#include "hacks.h"

#include "SAF/hacks.h"

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

//RelocPtr<UInt8> cursorFlags(0x126ABA8);
//Always on 0x02
//Default 0x00