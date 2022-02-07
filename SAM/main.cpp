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

#include "sam.h"
#include "scaleform.h"
#include "pose.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"

#include <shlobj.h>
#include <string>

PluginHandle	g_pluginHandle = kPluginHandle_Invalid;

F4SEScaleformInterface* g_scaleform = nullptr;
F4SEMessagingInterface* g_messaging = nullptr;
F4SEPapyrusInterface* g_papyrus = nullptr;

GFxMovieRoot* samRoot = nullptr;

class SamInputHandler : public BSInputEventUser
{
public:
	SamInputHandler() : BSInputEventUser(true) { }

	virtual void OnButtonEvent(ButtonEvent * inputEvent)
	{
		UInt32	keyCode;
		UInt32	deviceType = inputEvent->deviceType;
		UInt32	keyMask = inputEvent->keyMask;

		// Mouse
		if (deviceType == InputEvent::kDeviceType_Mouse)
			keyCode = InputMap::kMacro_MouseButtonOffset + keyMask; 
		// Gamepad
		else if (deviceType == InputEvent::kDeviceType_Gamepad)
			keyCode = InputMap::GamepadMaskToKeycode(keyMask);
		// Keyboard
		else
			keyCode = keyMask;

		// Valid scancode?
		if (keyCode >= InputMap::kMaxMacros)
			return;

		//BSFixedString	control	= *inputEvent->GetControlID();
		float timer	= inputEvent->timer;

		if (inputEvent->isDown == 1.0f && timer == 0.0f) {
			GFxValue arg(keyCode);
			samRoot->Invoke("root1.Menu_mc.processKeyDown", nullptr, &arg, 1);
		} else if (inputEvent->isDown == 0.0f && timer != 0.0f) {
			GFxValue arg(keyCode);
			samRoot->Invoke("root1.Menu_mc.processKeyUp", nullptr, &arg, 1);
		}
	}
};

SamInputHandler samInputHandler;

void SetInput(bool enable) {
	BSFixedString samMenu("ScreenArcherMenu");
	samInputHandler.enabled = enable;
	_LogCat("input ", enable ? "enabled" : "disabled");
	if (enable) {
		samRoot = (*g_ui)->GetMenu(samMenu)->movie->movieRoot;
	}
}

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
				SetInput(true);
				tArray<BSInputEventUser*>* inputEvents = &((*g_menuControls)->inputEvents);
				BSInputEventUser* inputHandler = &samInputHandler;
				int idx = inputEvents->GetItemIndex(inputHandler);
				if (idx == -1) {
					_DMESSAGE("ScreenArcherMenu Registered for input");
					inputEvents->Push(inputHandler);
				}
				OnMenuOpen();
			} else {
				OnMenuClose();
				SetInput(false);
			}
		} else {
			if ((*g_ui)->IsMenuOpen(samMenu)) {
				if (evn->menuName == consoleMenu) {
					if (evn->isOpen) {
						SetInput(false);
					} else {
						OnConsoleRefUpdate();
						SetInput(true);
					}
				}
			}
		}
		return kEvent_Continue;
	};
};

SamOpenCloseHandler openCloseHandler;

void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch(msg->type)
	{
		case F4SEMessagingInterface::kMessage_GameDataReady:
		{
			if (msg->data) {
				(*g_ui)->menuOpenCloseEventSource.AddEventSink(&openCloseHandler);
			}
		}
		break;
	}
}

void SAFMessageHandler(F4SEMessagingInterface::Message* msg)
{
	switch (msg->type)
	{
		case SAF::kSafMessageManager:
		{
			if (msg->data) 
			{
				_DMESSAGE("Casting adjustment manager");
				safAdjustmentManager = static_cast<SAF::AdjustmentManager*>(msg->data);
				_DMESSAGE("Casted");
				LoadMenuFiles();
			}
		}
		break;
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

	// supported runtime version
	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
	if (g_scaleform) 
		g_scaleform->Register("ScreenArcherMenu", RegisterScaleform);


	if (g_messaging) {
		g_messaging->RegisterListener(g_pluginHandle, "F4SE", F4SEMessageHandler);
		g_messaging->RegisterListener(g_pluginHandle, "SAF", SAFMessageHandler);
	}
		
	_DMESSAGE("Screen Archer Menu Loaded");

	return true;
}

};