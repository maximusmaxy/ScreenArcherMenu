#include "f4se/PluginAPI.h"
#include "f4se/PapyrusNativeFunctions.h"

#include "f4se/GameData.h"
#include "f4se/GameSettings.h"
#include "f4se/GameReferences.h"
#include "f4se/GameTypes.h"
#include "f4se/GameMenus.h"

#include "f4se/NiNodes.h"
#include "f4se/NiTypes.h"
#include "f4se/NiObjects.h"
#include "f4se/InputMap.h"
#include "f4se/GameRTTI.h"

#include "papyrus.h"
#include "serialization.h"
#include "adjustments.h"

#include <shlobj.h>
#include <string>

PluginHandle g_pluginHandle = kPluginHandle_Invalid;

F4SEMessagingInterface* g_messaging = nullptr;
F4SEPapyrusInterface* g_papyrus = nullptr;
F4SESerializationInterface* g_serialization = nullptr;

class SAFEventReciever :
	public BSTEventSink<TESInitScriptEvent>,
	public BSTEventSink<TESObjectLoadedEvent>
{
public:
	EventResult	ReceiveEvent(TESObjectLoadedEvent* evn, void* dispatcher) 
	{
		TESForm* form = LookupFormByID(evn->formId);
		Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
		if (!actor)
			return kEvent_Continue;

		TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if (!npc)
			return kEvent_Continue;

		SAF::g_adjustmentManager.UpdateActor(actor, npc, evn->loaded);

		return kEvent_Continue;
	}
};

SAFEventReciever safEventReciever;

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch(msg->type)
	{
		case F4SEMessagingInterface::kMessage_GameLoaded:
		{
			GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&safEventReciever);
		}
		break;
		case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			if (msg->data) {
				_DMESSAGE("Loading");
				SAF::g_adjustmentManager.Load();
				_DMESSAGE("Dispatching");
				g_messaging->Dispatch(g_pluginHandle, SAF::kSafMessageManager, &SAF::g_adjustmentManager, 8, nullptr);
			}
			break;
		}
		break;
	}
}

void SAFSaveCallback(const F4SESerializationInterface* ifc) {
	_DMESSAGE("Serializing save");
	SAF::g_adjustmentManager.SerializeSave(ifc);
}

void SAFLoadCallback(const F4SESerializationInterface* ifc) {
	_DMESSAGE("Serializing load");
	SAF::g_adjustmentManager.SerializeLoad(ifc);
}

void SAFRevertCallback(const F4SESerializationInterface* ifc) {
	_DMESSAGE("Serializing revert");
	SAF::g_adjustmentManager.SerializeRevert(ifc);
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\saf.log");
	_DMESSAGE("SAF");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"SAF";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}

	g_messaging = (F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);
	if(!g_messaging)
	{
		_FATALERROR("couldn't get messaging interface");
		return false;
	}

	g_papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
	if(!g_papyrus)
	{
		_FATALERROR("couldn't get papyrus interface");
		return false;
	}

	g_serialization = (F4SESerializationInterface*)f4se->QueryInterface(kInterface_Serialization);
	if (!g_serialization) {
		_FATALERROR("couldn't get serialization interface");
		return false;
	}

	// supported runtime version
	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
	if (g_messaging)
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);
	
	if (g_papyrus)
		g_papyrus->Register(SAF::RegisterPapyrus);

	if (g_serialization) {
		g_serialization->SetUniqueID(g_pluginHandle, 'SAF'); 
		g_serialization->SetRevertCallback(g_pluginHandle, SAFSaveCallback);
		g_serialization->SetLoadCallback(g_pluginHandle, SAFLoadCallback);
		g_serialization->SetSaveCallback(g_pluginHandle, SAFRevertCallback);
	}

	_DMESSAGE("SAF Loaded");

	return true;
}

};