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
#include "f4se/PluginManager.h"
#include "f4se_common/f4se_version.h"

#include "sam.h"
#include "scaleform.h"
#include "pose.h"
#include "idle.h"
#include "console.h"
#include "compatibility.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"

#include <shlobj.h>
#include <string>
#include <mutex>

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

F4SEScaleformInterface* g_scaleform = nullptr;
F4SEMessagingInterface* g_messaging = nullptr;
F4SEPapyrusInterface* g_papyrus = nullptr;
F4SEInterface* g_f4se = nullptr;

class SamInputHandler : public BSInputEventUser
{
public:
	SamInputHandler() : BSInputEventUser() { }

	virtual void OnButtonEvent(ButtonEvent * inputEvent)
	{
		BSFixedString samMenu("ScreenArcherMenu");

		IMenu* menu = (*g_ui)->GetMenu(samMenu);
		if (menu) {

			UInt32	keyCode;
			UInt32	deviceType = inputEvent->deviceType;
			UInt32	keyMask = inputEvent->keyMask;

			// Mouse
			if (deviceType == InputEvent::kDeviceType_Mouse) 
			{
				//Need to capture mouse scroll 0x800 = up, 0x900 = down
				if (keyMask == 0x800)
					keyCode = InputMap::kMaxMacros;
				else if (keyMask == 0x900)
					keyCode = InputMap::kMaxMacros + 1;
				else 
					keyCode = InputMap::kMacro_MouseButtonOffset + keyMask;
			}
			// Gamepad
			else if (deviceType == InputEvent::kDeviceType_Gamepad)
				keyCode = InputMap::GamepadMaskToKeycode(keyMask);
			// Keyboard
			else
				keyCode = keyMask;

			// Valid scancode? Add two for mouse scroll
			if (keyCode >= InputMap::kMaxMacros + 2)
				return;

			//BSFixedString	control	= *inputEvent->GetControlID();
			float timer = inputEvent->timer;

			if (inputEvent->isDown == 1.0f && timer == 0.0f) {
				GFxValue arg(keyCode);
				menu->movie->movieRoot->Invoke("root1.Menu_mc.processKeyDown", nullptr, &arg, 1);
			}
			else if (inputEvent->isDown == 0.0f && timer != 0.0f) {
				GFxValue arg(keyCode);
				menu->movie->movieRoot->Invoke("root1.Menu_mc.processKeyUp", nullptr, &arg, 1);
			}
		}
	}
};

SamInputHandler samInputHandler;

class SamOpenCloseHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	virtual ~SamOpenCloseHandler() { };
	virtual	EventResult	ReceiveEvent(MenuOpenCloseEvent * evn, void * dispatcher) override
	{
		BSFixedString samMenu("ScreenArcherMenu");
		BSFixedString photoMenu("PhotoMenu");
		BSFixedString consoleMenu("Console");

		if (evn->menuName == samMenu) {
			if (evn->isOpen) {
				samInputHandler.enabled = true;
				BSInputEventUser* inputHandler = &samInputHandler;
				int idx = (*g_menuControls)->inputEvents.GetItemIndex(inputHandler);
				if (idx == -1) {
					_DMESSAGE("ScreenArcherMenu Registered for input");
					(*g_menuControls)->inputEvents.Push(inputHandler);
				}
				OnMenuOpen();
			} else {
				OnMenuClose();
				samInputHandler.enabled = false;
			}
		} else {
			if ((*g_ui)->IsMenuOpen(samMenu)) {
				if (evn->menuName == consoleMenu) {
					if (evn->isOpen) {
						samInputHandler.enabled = false;
					} else {
						OnConsoleRefUpdate();
						samInputHandler.enabled = true;
					}
				}
			}
		}
		return kEvent_Continue;
	};
};

SamOpenCloseHandler openCloseHandler;

class SAMEventReciever :
	public BSTEventSink<TESLoadGameEvent>
{
public:
	EventResult	ReceiveEvent(TESLoadGameEvent* evn, void* dispatcher)
	{
		RegisterSam();
		return kEvent_Continue;
	}
};

SAMEventReciever samEventReciever;

void SAFMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case SAF::kSafAdjustmentManager:
		safAdjustmentManager = static_cast<SAF::AdjustmentManager*>(msg->data);
		LoadMenuFiles();
		break;
	case SAF::kSafAdjustmentActor:
		safMessageDispatcher.actorAdjustments = *(std::shared_ptr<SAF::ActorAdjustments>*)msg->data;
		break;
	case SAF::kSafResult:
		safMessageDispatcher.result = *(bool*)msg->data;
		break;
	}
}

void SafCreateAdjustment(UInt32 formId, const char* name) {
	SAF::AdjustmentCreateMessage message{ formId, name, "ScreenArcherMenu.esp", false, false, nullptr };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentCreate, &message, sizeof(uintptr_t), "SAF");
}

void SafSaveAdjustment(UInt32 formId, const char* filename, UInt32 handle)
{
	SAF::AdjustmentSaveMessage message{ formId, filename, handle };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentSave, &message, sizeof(uintptr_t), "SAF");
}

void SafLoadAdjustment(UInt32 formId, const char* filename) {
	SAF::AdjustmentCreateMessage message{ formId, filename, "ScreenArcherMenu.esp", true, false, "ScreenArcherMenu" };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentLoad, &message, sizeof(uintptr_t), "SAF");
}

void SafRemoveAdjustment(UInt32 formId, UInt32 handle) {
	SAF::AdjustmentMessage message{ formId, handle };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentErase, &message, sizeof(uintptr_t), "SAF");
}

void SafResetAdjustment(UInt32 formId, UInt32 handle) {
	SAF::AdjustmentMessage message{ formId, handle };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentReset, &message, sizeof(uintptr_t), "SAF");
}

void SafTransformAdjustment(UInt32 formId, UInt32 handle, const char* key, UInt32 type, float a, float b, float c) {
	SAF::AdjustmentTransformMessage message{ formId, handle, key, type, a, b, c };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentTransform, &message, sizeof(uintptr_t), "SAF");
}

void SafCreateActorAdjustments(UInt32 formId) {
	SAF::AdjustmentActorMessage message{ formId, "ScreenArcherMenu" };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentActor, &message, sizeof(uintptr_t), "SAF");
}

void SafNegateAdjustmentGroup(UInt32 formId, UInt32 handle, const char* group) {
	SAF::AdjustmentNegateMessage message{ formId, handle, group };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentNegate, &message, sizeof(uintptr_t), "SAF");
}

void SafLoadPose(UInt32 formId, const char* filename) {
	SAF::PoseMessage message{ formId, filename, "ScreenArcherMenu" };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafPoseLoad, &message, sizeof(uintptr_t), "SAF");
}

void SafResetPose(UInt32 formId) {
	SAF::PoseMessage message{ formId, nullptr, nullptr };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafPoseReset, &message, sizeof(uintptr_t), "SAF");
}

void SafLoadDefaultAdjustment(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable) {
	SAF::SkeletonMessage message { raceId, isFemale, filename, npc, clear, enable };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafDefaultAdjustmentLoad, &message, sizeof(uintptr_t), "SAF");
}

void SafMoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex) {
	SAF::MoveMessage message { formId, fromIndex, toIndex };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentMove, &message, sizeof(uintptr_t), "SAF");
}

void SafRenameAdjustment(UInt32 formId, UInt32 handle, const char* name) {
	SAF::AdjustmentSaveMessage message{ formId, name, handle };
	g_messaging->Dispatch(g_pluginHandle, SAF::kSafAdjustmentRename, &message, sizeof(uintptr_t), "SAF");
}

void RegisterSafMessageDispatcher()
{
	g_messaging->RegisterListener(g_pluginHandle, "SAF", SAFMessageHandler);
	safMessageDispatcher.createActorAdjustments = SafCreateActorAdjustments;
	safMessageDispatcher.createAdjustment = SafCreateAdjustment;
	safMessageDispatcher.saveAdjustment = SafSaveAdjustment;
	safMessageDispatcher.loadAdjustment = SafLoadAdjustment;
	safMessageDispatcher.removeAdjustment = SafRemoveAdjustment;
	safMessageDispatcher.resetAdjustment = SafResetAdjustment;
	safMessageDispatcher.transformAdjustment = SafTransformAdjustment;
	safMessageDispatcher.negateAdjustments = SafNegateAdjustmentGroup;
	safMessageDispatcher.loadPose = SafLoadPose;
	safMessageDispatcher.resetPose = SafResetPose;
	safMessageDispatcher.loadDefaultAdjustment = SafLoadDefaultAdjustment;
	safMessageDispatcher.moveAdjustment = SafMoveAdjustment;
	safMessageDispatcher.renameAdjustment = SafRenameAdjustment;
}

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
	case F4SEMessagingInterface::kMessage_PostLoad:
	{
		if (g_messaging)
			RegisterSafMessageDispatcher();

		if (g_f4se) {	
			//Try register ffc if RUNTIME_VERSION_1_10_163
			//if (g_f4se->runtimeVersion == RUNTIME_VERSION_1_10_163)
			//	RegisterFfcCompatiblity();
		}

		break;
	}
	case F4SEMessagingInterface::kMessage_GameDataReady:
	{
		if (msg->data)
			(*g_ui)->menuOpenCloseEventSource.AddEventSink(&openCloseHandler);
		break;
	}
	case F4SEMessagingInterface::kMessage_GameLoaded:
	{
		GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&samEventReciever);
	}
	}
}

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\sam.log");
	_DMESSAGE("Screen Archer Menu");

	// populate info structure
	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"ScreenArcherMenu";
	info->version =		1;

	// store plugin handle so we can identify ourselves later
	g_pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}

	g_f4se = (F4SEInterface*)f4se;

	g_scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
	if (!g_scaleform)
	{
		_FATALERROR("couldn't get scaleform interface");
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
		_WARNING("couldn't get papyrus interface");
	}

	samObScriptInit();

	// supported runtime version
	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
	if (g_scaleform) 
		g_scaleform->Register("ScreenArcherMenu", RegisterScaleform);

	if (g_messaging)
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);

	samObScriptCommit();
		
	_DMESSAGE("Screen Archer Menu Loaded");

	return true;
}

};