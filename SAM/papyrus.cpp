#include "papyrus.h"

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusUtilities.h"
#include "f4se/PapyrusScaleformAdapter.h"

#include "SAF/util.h"

#include "sam.h"
#include "positioning.h"
#include "constants.h"
#include "io.h"

#include "json/json.h"

#include <filesystem>

RelocAddr<_PapyrusDeleteInternal> PapyrusDeleteInternal(0x1404960);
RelocAddr<_PapyrusPlayGamebryoAnimationInternal> PapyrusPlayGamebryoAnimationInternal(0x140BD60);

void CallPapyrusForm(GFxResult& result, const char* id, const char* function, GFxValue& args)
{
	try
	{
		UInt32 formId = std::stoul(id, nullptr, 16);
		TESForm* form = LookupFormByID(formId);
		if (form) {

			VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
			if (vm) {
				VMArray<VMVariable> arguments;

				for (int i = 0; i < args.GetArraySize(); ++i) {
					VMValue vmvalue;
					GFxValue value;
					args.GetElement(i, &value);
					PlatformAdapter::ConvertScaleformValue(&vmvalue, &value, vm);
					VMVariable var;
					var.PackVariable(&vmvalue);
					arguments.Push(&var);
				}

				CallFunctionNoWait<TESForm>(form, BSFixedString(function), arguments);
				return result.SetWaiting();
			}
		}
	}
	catch (...) {}

	return result.SetError(PAPYRUS_FUNCTION_ERROR);
}

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

TESObjectREFR* PapyrusGetRefr(StaticFunctionTag*) 
{
	return selected.refr;
}

TESObjectREFR* PapyrusGetNonActorRefr(StaticFunctionTag*)
{
	return selectedNonActor.refr;
}

void PapyrusOpenMenu(StaticFunctionTag*, BSFixedString name)
{
	samManager.OpenExtensionMenu(name.c_str());
}

void PapyrusPushMenu(StaticFunctionTag*, BSFixedString name)
{
	samManager.PushMenu(name.c_str());
}

void PapyrusPopMenu(StaticFunctionTag*)
{
	samManager.PopMenu();
}

void PapyrusPopMenuTo(StaticFunctionTag*, BSFixedString name)
{
	samManager.PopMenuTo(name.c_str());
}

void PapyrusShowNotification(StaticFunctionTag*, BSFixedString msg)
{
	samManager.ShowNotification(msg.c_str());
}

void PapyrusSetTitle(StaticFunctionTag*, BSFixedString title)
{
	samManager.SetTitle(title.c_str());
}

void PapyrusSetMenuValues(StaticFunctionTag*, VMArray<VMVariable> values) {
	samManager.SetMenuValues(values);
}

void PapyrusSetMenuItems(StaticFunctionTag*, VMArray<BSFixedString> names, VMArray<VMVariable> values) {
	samManager.SetMenuItems(names, values);
}

void PapyrusSetMenuNames(StaticFunctionTag*, VMArray<BSFixedString> names) {
	samManager.SetMenuNames(names);
}

void PapyrusSetString(StaticFunctionTag*, BSFixedString msg)
{
	samManager.SetString(msg.c_str());
}

void PapyrusSetSuccess(StaticFunctionTag*)
{
	samManager.SetSuccess();
}

void PapyrusSetError(StaticFunctionTag*, BSFixedString error)
{
	samManager.SetError(error.c_str());
}

BSFixedString PapyrusGetFilename(BSFixedString path) {
	return BSFixedString(std::filesystem::path(path.c_str()).stem().string().c_str());
}

BSFixedString PapyrusGetRelativePath(BSFixedString root, BSFixedString extension, BSFixedString path) {
	return BSFixedString(GetRelativePath(strlen(root), strlen(extension), path.c_str()).c_str());
}

void PapyrusToggleMenu(StaticFunctionTag*) {
	ToggleMenu();
}

void PapyrusLogMenu(StaticFunctionTag*, BSFixedString menuName) 
{
	auto menu = GetCachedMenu(menuName.c_str());

	Json::StyledWriter writer;
	std::string styled = writer.write(*menu);
	_DMESSAGE(styled.c_str());
}

void PapyrusReloadMenus(StaticFunctionTag*) {
	static BSFixedString samMenuName(SAM_MENU_NAME);
	samManager.CloseMenu(samMenuName);
	samManager.ClearData();
	ReloadJsonMenus();
}

void PapyrusForceQuit(StaticFunctionTag*) {
	static BSFixedString samMenuName(SAM_MENU_NAME);
	samManager.CloseMenu(samMenuName);
	samManager.ClearData();
}

bool RegisterPapyrus(VirtualMachine* vm) {
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("OpenMenu", "SAM", PapyrusOpenMenu, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("PushMenu", "SAM", PapyrusPushMenu, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("PopMenu", "SAM", PapyrusPopMenu, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("PopMenuTo", "SAM", PapyrusPopMenuTo, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("ShowNotification", "SAM", PapyrusShowNotification, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("SetTitle", "SAM", PapyrusSetTitle, vm));

	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMArray<BSFixedString>>("SetNames", "SAM", PapyrusSetMenuNames, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMArray<VMVariable>>("SetValues", "SAM", PapyrusSetMenuValues, vm));
	vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, VMArray<BSFixedString>, VMArray<VMVariable>>("SetItems", "SAM", PapyrusSetMenuItems, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("SetString", "SAM", PapyrusSetString, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("SetSuccess", "SAM", PapyrusSetSuccess, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("SetError", "SAM", PapyrusSetError, vm));

	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, TESObjectREFR*>("GetRefr", "SAM", PapyrusGetRefr, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, TESObjectREFR*>("GetNonActorRefr", "SAM", PapyrusGetRefr, vm));

	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("LogMenu", "SAM", PapyrusLogMenu, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ReloadMenus", "SAM", PapyrusReloadMenus, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ForceQuit", "SAM", PapyrusForceQuit, vm));

	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ToggleMenu", "SAM", PapyrusToggleMenu, vm));

	return true;
}