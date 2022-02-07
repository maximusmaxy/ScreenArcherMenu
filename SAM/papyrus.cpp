#include "papyrus.h"

#include "f4se/PluginAPI.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusEvents.h"
#include "f4se/PapyrusValue.h"
#include "f4se/PapyrusArgs.h"
#include "f4se/PapyrusUtilities.h"

#include "f4se/GameReferences.h"
#include "f4se/GameForms.h"

#include "sam.h"

void SetMenuMovement(bool enabled, const char* type, UInt64 handle) {
	VirtualMachine * vm = (*g_gameVM)->m_virtualMachine;
	BSFixedString functionName("SetMovement");

	VMValue receiver;
	BSFixedString typeName(type);
	GetIdentifier(&receiver, handle, &typeName, vm);

	if (!receiver.IsIdentifier()) {
		_DMESSAGE("Identifier for screen archer menu script not found");
		return;
	}
	VMIdentifier* identifier = receiver.data.id;

	VMValue* enableValue = new VMValue();
	enableValue->SetBool(enabled);

	VMValue packedArgs;
	VMValue::ArrayData * arrayData = nullptr;
	vm->CreateArray(&packedArgs, 1, &arrayData);

	packedArgs.type.value = VMValue::kType_VariableArray;
	packedArgs.data.arr = arrayData;
	arrayData->arr.entries[0].SetVariable(enableValue);
		
	CallFunctionNoWait_Internal(vm, 0, identifier, &functionName, &packedArgs);
}