#pragma once

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "gfx.h"
#include "smartpointer.h"

bool RegisterPapyrus(VirtualMachine* vm);
void CallPapyrusForm(GFxResult& result, const char* id, const char* function, GFxValue& args);
void CallPapyrusGlobal(BSFixedString script, BSFixedString function);
void CallSamGlobal(BSFixedString function);
void ShowHudMessage(const char* msg);

void PapyrusDelete(TESObjectREFR* refr);
void PapyrusPlayGamebryoAnimation(TESObjectREFR* refr, BSFixedString* animation);
void PapyrusAddItem(TESObjectREFR* refr, TESForm* form, UInt32 amount, bool silent);
void PapyrusAttachModToInventoryItem(TESObjectREFR* refr, TESForm* form, TESForm* mod);

template <class T>
struct VMObjectTypeInfoRefCount
{
	static void Acquire(T* a_ptr) { InterlockedIncrement(&a_ptr->m_refCount); }
	static void Release(T* a_ptr)
	{
		if (InterlockedDecrement(&a_ptr->m_refCount) == 0)
			delete a_ptr;
	}
};
typedef BSTSmartPointer<VMObjectTypeInfo, VMObjectTypeInfoRefCount> VMObjectTypeInfoPtr;

template <class T>
struct IFunctionRefCount
{
	static void Acquire(T* a_ptr) { InterlockedIncrement(&a_ptr->refCount); }
	static void Release(T* a_ptr)
	{
		if (InterlockedDecrement(&a_ptr->refCount) == 0)
			delete a_ptr;
	}
};
typedef BSTSmartPointer<IFunction, IFunctionRefCount> IFunctionPtr;

bool GetStaticFunction(const char* script, const char* func, IFunctionPtr* ifunction);
void* GetNativeCallback(const char* script, const char* func);
BGSKeyword* GetSamKeyword();