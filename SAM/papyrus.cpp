#include "papyrus.h"

#include "f4se/PapyrusNativeFunctions.h"

#include "sam.h"

RelocAddr<_PapyrusDeleteInternal> PapyrusDeleteInternal(0x1404960);
RelocAddr<_PapyrusPlayGamebryoAnimationInternal> PapyrusPlayGamebryoAnimationInternal(0x140BD60);

void PapyrusDelete(TESObjectREFR* refr)
{
	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;

	PapyrusDeleteInternal(vm, 0, refr);
}

void PapyrusPlayGamebryoAnimation(TESObjectREFR* refr, BSFixedString* str)
{
	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;

	PapyrusPlayGamebryoAnimationInternal(vm, 0, refr, str, true);
}

void PapyrusToggleMenu(StaticFunctionTag*) {
	ToggleMenu();
}

bool RegisterPapyrus(VirtualMachine* vm) {
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ToggleMenu", "SAM", PapyrusToggleMenu, vm));

	return true;
}