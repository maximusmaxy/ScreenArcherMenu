#include "papyrus.h"

#include "f4se/PapyrusNativeFunctions.h"

#include "sam.h"

void PapyrusToggleMenu(StaticFunctionTag*) {
	ToggleMenu();
}

bool RegisterPapyrus(VirtualMachine* vm) {
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ToggleMenu", "SAM", PapyrusToggleMenu, vm));

	return true;
}