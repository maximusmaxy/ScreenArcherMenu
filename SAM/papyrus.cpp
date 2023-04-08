#include "papyrus.h"

#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusUtilities.h"
#include "f4se/PapyrusScaleformAdapter.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/GameMenus.h"
#include "f4se/CustomMenu.h"

#include "SAF/util.h"

#include "sam.h"
#include "positioning.h"
#include "constants.h"
#include "io.h"
#include "data.h"

#include "json/json.h"

#include <filesystem>

typedef void (*_PapyrusDeleteInternal)(VirtualMachine* vm, UInt64 unk2, TESObjectREFR* refr);
RelocAddr<_PapyrusDeleteInternal> PapyrusDeleteInternal(0x1404960);

typedef void (*_PapyrusPlayGamebryoAnimationInternal)(VirtualMachine* vm, UInt64 unk2, TESObjectREFR* refr, BSFixedString* animation, bool unk5);
RelocAddr<_PapyrusPlayGamebryoAnimationInternal> PapyrusPlayGamebryoAnimationInternal(0x140BD60);

typedef void (*_PapyrusAttachModToInventoryItemInternal)(VirtualMachine* vm, UInt64 unk2, TESObjectREFR* refr, TESForm* form, TESForm* mod);
RelocAddr<_PapyrusAttachModToInventoryItemInternal> PapyrusAttachModToInventoryItemInternal(0x1403BF0);

struct Vmref {
	TESForm* form;
	UInt16 type;
	UInt16 pad;
	UInt32 pad2;
};

typedef void (*_PapyrusAddItemInternal)(VirtualMachine* vm, UInt64 unk2, TESObjectREFR* refr, Vmref* vmref, UInt32 amount, bool silent);
RelocAddr<_PapyrusAddItemInternal> PapyrusAddItemInternal(0x1402B40);

typedef bool (*_VMGetStaticFunction)(VirtualMachine* vm, const char* script, const char* func, VMObjectTypeInfoPtr* typeInfo, IFunctionPtr* function);
RelocAddr<_VMGetStaticFunction> VMGetStaticFunction(0x273D730);

bool GetStaticFunction(const char* script, const char* func, IFunctionPtr* function)
{
	VMObjectTypeInfoPtr typeInfo;
	return VMGetStaticFunction((*g_gameVM)->m_virtualMachine, script, func, &typeInfo, function);
}

void* GetNativeCallback(const char* script, const char* func)
{
	IFunctionPtr function;
	
	if (!GetStaticFunction(script, func, &function))
		return nullptr;
	
	//gets the protected callback
	return (void*)(((UInt64)function.get()) + 0x50);
}

BGSKeyword* GetSamKeyword()
{
	UInt32 formId = GetFormId(SAM_ESP, 0x803);
	return (BGSKeyword*)LookupFormByID(formId);
}

void CallPapyrusForm(GFxResult& result, const char* id, const char* function, GFxValue& args)
{
	try
	{
		UInt32 formId = HexStringToUInt32(id);
		TESForm* form = LookupFormByID(formId);
		if (form) {

			VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
			if (vm) {
				VMArray<VMVariable> arguments;

				for (int i = 0; i < args.GetArraySize(); ++i) {
					VMVariable var;
					GFxValue value;
					args.GetElement(i, &value);
					GFxToVMVariable(&value, &var);
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

typedef void (*_ShowHudMessageInternal)(const char* msg, const char* unk1, bool unk2, bool unk3);
RelocAddr<_ShowHudMessageInternal> ShowHudMessageInternal(0xAE1E90);

void ShowHudMessage(const char* msg) {
	ShowHudMessageInternal(msg, nullptr, true, true);
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

void PapyrusAddItem(TESObjectREFR* refr, TESForm* item, UInt32 amount, bool silent)
{
	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;

	Vmref itemRef{ item, 0 };
	PapyrusAddItemInternal(vm, 0, refr, &itemRef, amount, silent);
}

void PapyrusAttachModToInventoryItem(TESObjectREFR* refr, TESForm* armor, TESForm* mod)
{
	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;

	PapyrusAttachModToInventoryItemInternal(vm, 0, refr, armor, mod);
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
	samManager.ShowNotification(msg.c_str(), false);
}

void PapyrusSetTitle(StaticFunctionTag*, BSFixedString title)
{
	samManager.SetTitle(title.c_str());
}

void PapyrusSetNotification(StaticFunctionTag*, BSFixedString msg)
{
	samManager.SetNotification(msg.c_str());
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
	samManager.ToggleMenu();
}

void PapyrusReregisterMenu(StaticFunctionTag*) {
	BSFixedString menuName(SAM_MENU_NAME);
	(*g_ui)->UnregisterMenu(menuName, true);
	(*g_ui)->Register(menuName.c_str(), CreateCustomMenu);
}

void PapyrusLogMenu(StaticFunctionTag*, BSFixedString menuName) 
{
	auto menu = GetCachedMenu(menuName.c_str());

	if (!menu)
		return;

	Json::StyledWriter writer;
	std::string styled = writer.write(*menu);
	_DMESSAGE(styled.c_str());
}

void PapyrusReloadMenus(StaticFunctionTag*) {
	ReloadJsonMenus();
}

void PapyrusForceQuit(StaticFunctionTag*) {
	samManager.ForceQuit();
}

bool RegisterPapyrus(VirtualMachine* vm) {
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("OpenMenu", "SAM", PapyrusOpenMenu, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("PushMenu", "SAM", PapyrusPushMenu, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("PopMenu", "SAM", PapyrusPopMenu, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("PopMenuTo", "SAM", PapyrusPopMenuTo, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("ShowNotification", "SAM", PapyrusShowNotification, vm));

	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMArray<BSFixedString>>("SetNames", "SAM", PapyrusSetMenuNames, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, VMArray<VMVariable>>("SetValues", "SAM", PapyrusSetMenuValues, vm));
	vm->RegisterFunction(new NativeFunction2<StaticFunctionTag, void, VMArray<BSFixedString>, VMArray<VMVariable>>("SetItems", "SAM", PapyrusSetMenuItems, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("SetNotification", "SAM", PapyrusSetNotification, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("SetTitle", "SAM", PapyrusSetTitle, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("SetSuccess", "SAM", PapyrusSetSuccess, vm));
	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("SetError", "SAM", PapyrusSetError, vm));

	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, TESObjectREFR*>("GetRefr", "SAM", PapyrusGetRefr, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, TESObjectREFR*>("GetNonActorRefr", "SAM", PapyrusGetRefr, vm));

	vm->RegisterFunction(new NativeFunction1<StaticFunctionTag, void, BSFixedString>("LogMenu", "SAM", PapyrusLogMenu, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ReloadMenus", "SAM", PapyrusReloadMenus, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ForceQuit", "SAM", PapyrusForceQuit, vm));

	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ToggleMenu", "SAM", PapyrusToggleMenu, vm));
	vm->RegisterFunction(new NativeFunction0<StaticFunctionTag, void>("ReregisterMenu", "SAM", PapyrusReregisterMenu, vm));

	return true;
}