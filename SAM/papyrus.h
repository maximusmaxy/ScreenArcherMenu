#pragma once

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusEvents.h"

bool RegisterPapyrus(VirtualMachine* vm);
void CallPapyrusGlobal(BSFixedString script, BSFixedString function);
void CallSamGlobal(BSFixedString function);

typedef void (*_PapyrusDeleteInternal)(VirtualMachine* vm, UInt64 unk2, TESObjectREFR* refr);
extern RelocAddr<_PapyrusDeleteInternal> PapyrusDeleteInternal;

typedef void (*_PapyrusPlayGamebryoAnimationInternal)(VirtualMachine* vm, UInt64 unk2, TESObjectREFR* refr, BSFixedString* animation, bool unk5);
extern RelocAddr<_PapyrusPlayGamebryoAnimationInternal> PapyrusPlayGamebryoAnimationInternal;

void PapyrusDelete(TESObjectREFR* refr);
void PapyrusPlayGamebryoAnimation(TESObjectREFR* refr, BSFixedString* animation);