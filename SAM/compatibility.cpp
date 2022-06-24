#include "compatibility.h"

#include "SAF/util.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusUtilities.h"
#include "f4se/GameMenus.h"

#include "sam.h"

bool** ffcKeyboardInput = nullptr;
bool** ffcPadInput = nullptr;

bool RegisterFfcCompatiblity()
{
	_DMESSAGE("Getting loaded plugin");

	UInt64 ffcHandle = (UInt64)GetModuleHandle("FreeFlyCam Fo4.dll");

	if (ffcHandle) {
		ffcKeyboardInput = (bool**)(ffcHandle + 0x3E2B8);
		ffcPadInput = (bool**)(ffcHandle + 0x3E2C0);
		return true;
	}

	return false;
}

//Start of public variables
#define FFCINPUTPUBLIC 0x16C
//Lock boolean offset
#define FFCINPUTLOCK 0x160

bool GetFfcLock(bool** input) {
	if (!*input)
		return false;

	return *(*input + FFCINPUTPUBLIC + FFCINPUTLOCK);
}

bool GetFfcLock(FfcType type) {
	return GetFfcLock(type == kFfcKeyboard ? ffcKeyboardInput : ffcPadInput);
}

void SetFfcLock(bool** input, bool locked) {
	if (!*input)
		return;

	*(*input + FFCINPUTPUBLIC + FFCINPUTLOCK) = locked;

	//need to zero the public variables if locking
	if (locked)
		memset(*input + FFCINPUTPUBLIC, 0, FFCINPUTLOCK);
}

bool keyboardLocked = false;
bool padLocked = false;

void LockFfc(bool locked)
{
	if (!ffcKeyboardInput || !ffcPadInput)
		return;

	if (locked) {
		//store previous state
		keyboardLocked = GetFfcLock(ffcKeyboardInput);
		padLocked = GetFfcLock(ffcPadInput);

		SetFfcLock(ffcKeyboardInput, true);
		SetFfcLock(ffcPadInput, true);
	}
	else {
		//restore previous state
		SetFfcLock(ffcKeyboardInput, keyboardLocked);
		SetFfcLock(ffcPadInput, padLocked);
	}
}

//class VmScriptFinder : public VirtualMachine::IdentifierItem::IScriptVisitor
//{
//public:
//	UInt64 result;
//
//	VmScriptFinder(const char* name, IObjectHandlePolicy* policy) : name(name), policy(policy), result(0) {}
//		 
//	bool Visit(VMIdentifier* obj) {
//		if (result)
//			return false;
//
//		UInt64 handle = obj->GetHandle();
//		policy->GetName(handle, outStr);
//		if (!_stricmp(reinterpret_cast<const char*>(outStr), name)) {
//			result = handle;
//			return false;
//		}
//		
//		return true;
//	}
//
//private:
//	const char* name;
//	IObjectHandlePolicy* policy;
//	void* outStr;
//};
//
//UInt64 GetScriptHandle(const char* name) {
//	VmScriptFinder finder(name, (*g_gameVM)->m_virtualMachine->GetHandlePolicy());
//
//	BSWriteLocker locker(&(*g_gameVM)->m_virtualMachine->scriptsLock);
//
//	(*g_gameVM)->m_virtualMachine->m_attachedScripts.ForEach([&](VirtualMachine::IdentifierItem& item) {
//		item.ForEachScript(&finder);
//	});
//
//	return finder.result;
//}
//
//UInt32 FindPapyrusPropertyIndex(const char* name, VMIdentifier* identifier) {
//	for (int i = 0; i < identifier->m_typeInfo->memberData.numMembers; ++i) {
//		if (!_stricmp(identifier->m_typeInfo->properties->defs[i].propertyName, name))
//			return i;
//	}
//	return -1;
//}
//
//const char* aafMainQuestScript = "AAF:AAF_MainQuestScript";
//bool aafUnregistered = false;
//
//BSFixedString* GetAAFSwfPath() {
//	UInt64 handle = GetScriptHandle(aafMainQuestScript);
//	if (!handle)
//		return nullptr;
//
//	auto aafScript = (VMIdentifier*)PapyrusVM::GetObjectFromHandle(handle, VMValue::kType_Identifier);
//	if (!aafScript)
//		return nullptr;
//
//	UInt32 index = FindPapyrusPropertyIndex("SWFPath", aafScript);
//	if (index == -1)
//		return nullptr;
//
//	VMValue* value = &aafScript->properties[index];
//	if (value->type.value != VMValue::kType_String)
//		return nullptr;
//
//	return value->data.GetStr();
//}
//
//void GetAAFRegisteredKeys(std::vector<int>& keys) {
//	static BSFixedString hudName("HUDMenu");
//
//	GFxMovieRoot* root = GetRoot(hudName);
//	if (!root)
//		return;
//
//	BSFixedString* aafSwfPath = GetAAFSwfPath();
//	if (!aafSwfPath)
//		return;
//	
//	char pathToFunc[256];
//	strcpy(pathToFunc, aafSwfPath->c_str());
//	strcat(pathToFunc, ".userInput.getKeys");
//	
//	GFxValue result;
//	if (!root->Invoke(pathToFunc, &result, nullptr, 0))
//		return;
//
//	if (!result.IsArray())
//		return;
//
//	int length = result.GetArraySize();
//	GFxValue element;
//
//	for (int i = 0; i < length; ++i) {
//		result.GetElement(i, &element);
//		keys.push_back(element.GetInt());
//	}
//}
//
//void RegisterAAFKeys(bool unregistering) {
//	if (aafUnregistered == unregistering)
//		return;
//
//	UInt64 handle = GetScriptHandle(aafMainQuestScript);
//
//	if (!handle)
//		return;
//
//	VMValue registerValue;
//	BSFixedString registerString(unregistering ? "UnregisterForKey" : "RegisterForKey");
//	VirtualMachine* vm = (*g_gameVM)->m_virtualMachine;
//	
//	GetIdentifier(&registerValue, handle, &registerString, vm);
//
//	if (!registerValue.IsIdentifier())
//		return;
//
//	VMIdentifier* registerId = registerValue.data.id;
//
//	std::vector<int> keys;
//	GetAAFRegisteredKeys(keys);
//
//	for (auto& key : keys) {
//		VMValue* keyValue = new VMValue();
//		keyValue->SetInt(key);
//
//		VMValue args;
//		
//		VMValue::ArrayData* arrayData = nullptr;
//		vm->CreateArray(&args, 1, &arrayData);
//
//		args.type.value = VMValue::kType_VariableArray;
//		args.data.arr = arrayData;
//		arrayData->arr.entries[0].SetVariable(keyValue);
//
//		CallFunctionNoWait_Internal(vm, 0, registerId, &registerString, &args);
//	}
//
//	aafUnregistered = unregistering;
//}