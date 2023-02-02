#pragma once

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusEvents.h"

#include "gfx.h"

bool RegisterPapyrus(VirtualMachine* vm);
void CallPapyrusForm(GFxResult& result, const char* id, const char* function, GFxValue& args);
void CallPapyrusGlobal(BSFixedString script, BSFixedString function);
void CallSamGlobal(BSFixedString function);

void PapyrusDelete(TESObjectREFR* refr);
void PapyrusPlayGamebryoAnimation(TESObjectREFR* refr, BSFixedString* animation);
void PapyrusAddItem(TESObjectREFR* refr, TESForm* form, UInt32 amount, bool silent);
void PapyrusAttachModToInventoryItem(TESObjectREFR* refr, TESForm* form, TESForm* mod);