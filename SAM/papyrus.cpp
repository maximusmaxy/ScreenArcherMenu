#include "papyrus.h"

#include "f4se/PapyrusNativeFunctions.h"

#include "sam.h"

RelocAddr<_PapyrusDeleteInternal> PapyrusDeleteInternal(0x1404960);
RelocAddr<_PapyrusPlayGamebryoAnimationInternal> PapyrusPlayGamebryoAnimationInternal(0x140BD60);

void CallPapyrusGlobal(BSFixedString script, BSFixedString function)
{
	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;

	VMValue packedArgs;
	VMValue::ArrayData* arrayData = nullptr;
	vm->CreateArray(&packedArgs, 0, &arrayData);
	packedArgs.type.value = VMValue::kType_VariableArray;
	packedArgs.data.arr = arrayData;
	
	CallGlobalFunctionNoWait_Internal(vm, 0, 0, &script, &function, &packedArgs);
}

void CallSamGlobal(BSFixedString function)
{
	static BSFixedString samMenu(SAM_MENU_NAME);
	CallPapyrusGlobal(samMenu, function);
}

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

void PapyrusReloadMenus(StaticFunctionTag*) {
	ReloadJsonMenus();
}

bool RegisterPapyrus(VirtualMachine* vm) {
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ToggleMenu", "SAM", PapyrusToggleMenu, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ReloadMenus", "SAM", PapyrusReloadMenus, vm));

	return true;
}