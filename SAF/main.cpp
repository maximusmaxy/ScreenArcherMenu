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
#include "util.h"

#include <shlobj.h>
#include <string>

PluginHandle g_pluginHandle = kPluginHandle_Invalid;

F4SEMessagingInterface* g_messaging = nullptr;
F4SEPapyrusInterface* g_papyrus = nullptr;
F4SESerializationInterface* g_serialization = nullptr;

const std::string& GetSafConfigPath()
{
	static std::string safConfigPath;

	if (safConfigPath.empty())
	{
		std::string	runtimePath = GetRuntimeDirectory();
		if (!runtimePath.empty())
		{
			safConfigPath = runtimePath + "Data\\F4SE\\Plugins\\SAF.ini";

			//_MESSAGE("config path = %s", safConfigPath.c_str());
		}
	}

	return safConfigPath;
}

std::string GetSafConfigOption(const char* section, const char* key)
{
	std::string	result;

	const std::string& configPath = GetSafConfigPath();
	if (!configPath.empty())
	{
		char	resultBuf[256];
		resultBuf[0] = 0;

		UInt32	resultLen = GetPrivateProfileString(section, key, NULL, resultBuf, sizeof(resultBuf), configPath.c_str());

		result = resultBuf;
	}

	return result;
}

bool GetSafConfigOption_UInt32(const char* section, const char* key, UInt32* dataOut)
{
	std::string	data = GetSafConfigOption(section, key);
	if (data.empty())
		return false;

	return (sscanf_s(data.c_str(), "%u", dataOut) == 1);
}

class SAFEventReciever :
	public BSTEventSink<TESObjectLoadedEvent>,
	public BSTEventSink<TESLoadGameEvent>,
	public BSTEventSink< TESInitScriptEvent>
{
public:
	EventResult	ReceiveEvent(TESObjectLoadedEvent* evn, void* dispatcher) 
	{
		if (!SAF::g_adjustmentManager.gameLoaded) return kEvent_Continue;

		TESForm* form = LookupFormByID(evn->formId);
		Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
		if (!actor)
			return kEvent_Continue;

		SAF::g_adjustmentManager.ActorLoaded(actor, evn->loaded);

		return kEvent_Continue;
	}

	EventResult	ReceiveEvent(TESInitScriptEvent* evn, void* dispatcher)
	{
		Actor* actor = DYNAMIC_CAST(evn->reference, TESForm, Actor);
		if (!actor)
			return kEvent_Continue;

		SAF::g_adjustmentManager.ActorLoaded(actor, true);

		return kEvent_Continue;
	}

	EventResult	ReceiveEvent(TESLoadGameEvent* evn, void* dispatcher)
	{
		SAF::g_adjustmentManager.GameLoaded();
		
		//_DMESSAGE("Game loaded");

		return kEvent_Continue;
	}
};

SAFEventReciever safEventReciever;

void SAFMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case SAF::kSafAdjustmentCreate:
	{
		auto data = (SAF::AdjustmentCreateMessage*)msg->data;
		SAF::g_adjustmentManager.CreateNewAdjustment(data->formId, data->name, data->esp, data->persistent, data->hidden);
		break;
	}
	case SAF::kSafAdjustmentSave:
	{
		auto data = (SAF::AdjustmentSaveMessage*)msg->data;
		SAF::g_adjustmentManager.SaveAdjustment(data->formId, data->filename, data->handle);
		break;
	}
	case SAF::kSafAdjustmentLoad:
	{
		auto data = (SAF::AdjustmentCreateMessage*)msg->data;
		bool result = SAF::g_adjustmentManager.LoadAdjustment(data->formId, data->name, data->esp, data->persistent, data->hidden);
		g_messaging->Dispatch(g_pluginHandle, SAF::kSafResult, &result, sizeof(uintptr_t), data->mod);
		break;
	}
	case SAF::kSafAdjustmentErase:
	{
		auto data = (SAF::AdjustmentMessage*)msg->data;
		SAF::g_adjustmentManager.RemoveAdjustment(data->formId, data->handle);
		break;
	}
	case SAF::kSafAdjustmentReset:
	{
		auto data = (SAF::AdjustmentMessage*)msg->data;
		SAF::g_adjustmentManager.ResetAdjustment(data->formId, data->handle);
		break;
	}
	case SAF::kSafAdjustmentTransform:
	{
		auto data = (SAF::AdjustmentTransformMessage*)msg->data;
		SAF::g_adjustmentManager.SetTransform(data);
		break;
	}
	case SAF::kSafAdjustmentActor:
	{
		auto data = (SAF::AdjustmentActorMessage*)msg->data;
		std::shared_ptr<SAF::ActorAdjustments> adjustments = SAF::g_adjustmentManager.CreateActorAdjustment(data->formId);
		g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentActor, &adjustments, sizeof(uintptr_t), data->mod);
		break;
	}
	//case SAF::kSafAdjustmentNegate:
	//{
	//	auto data = (SAF::AdjustmentNegateMessage*)msg->data;
	//	SAF::g_adjustmentManager.NegateAdjustments(data->formId, data->handle, data->group);
	//	break;
	//}
	case SAF::kSafPoseLoad:
	{
		auto data = (SAF::PoseMessage*)msg->data;
		bool result = SAF::g_adjustmentManager.LoadPose(data->formId, data->filename);
		g_messaging->Dispatch(g_pluginHandle, SAF::kSafResult, &result, sizeof(uintptr_t), data->mod);
		break;
	}
	case SAF::kSafPoseReset:
	{
		auto data = (SAF::PoseMessage*)msg->data;
		SAF::g_adjustmentManager.ResetPose(data->formId);
		break;
	}
	case SAF::kSafDefaultAdjustmentLoad:
	{
		auto data = (SAF::SkeletonMessage*)msg->data;
		SAF::g_adjustmentManager.LoadDefaultAdjustment(data->raceId, data->isFemale, data->filename, data->npc, data->clear, data->enable);
		break;
	}
	}
}

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case F4SEMessagingInterface::kMessage_PostLoad:
		if (g_messaging)
			g_messaging->RegisterListener(g_pluginHandle, nullptr, SAFMessageHandler);
		break;
	case F4SEMessagingInterface::kMessage_GameLoaded:
		GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&safEventReciever);
		GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&safEventReciever);
		//GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(&safEventReciever);
		break;
	case F4SEMessagingInterface::kMessage_GameDataReady:
		SAF::g_adjustmentManager.overridePostfix = GetSafConfigOption("skeleton", "override");
		SAF::g_adjustmentManager.offsetPostfix = GetSafConfigOption("skeleton", "offset");
		SAF::g_adjustmentManager.LoadFiles();
		g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentManager, &SAF::g_adjustmentManager, sizeof(uintptr_t), nullptr);
		break;
	}
}


void SAFSaveCallback(const F4SESerializationInterface* ifc) {
	//_DMESSAGE("Serializing save");
	SAF::g_adjustmentManager.SerializeSave(ifc);
}

void SAFLoadCallback(const F4SESerializationInterface* ifc) {
	//_DMESSAGE("Serializing load");
	SAF::g_adjustmentManager.SerializeLoad(ifc);
}

void SAFRevertCallback(const F4SESerializationInterface* ifc) {
	//_DMESSAGE("Serializing revert");
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
		g_serialization->SetSaveCallback(g_pluginHandle, SAFSaveCallback);
		g_serialization->SetLoadCallback(g_pluginHandle, SAFLoadCallback);
		g_serialization->SetRevertCallback(g_pluginHandle, SAFRevertCallback);
	}

	_DMESSAGE("SAF Loaded");

	return true;
}

};