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
#include "messaging.h"

#include <shlobj.h>
#include <string>

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\saf.log");
	_DMESSAGE("SAF");

	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		"SAF";
	info->version =		1;

	SAF::safMessaging.pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}

	SAF::safMessaging.messaging = (F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);

	if(!SAF::safMessaging.messaging)
	{
		_FATALERROR("couldn't get messaging interface");
		return false;
	}

	SAF::safMessaging.papyrus = (F4SEPapyrusInterface*)f4se->QueryInterface(kInterface_Papyrus);
	if(!SAF::safMessaging.papyrus)
	{
		_FATALERROR("couldn't get papyrus interface");
		return false;
	}

	SAF::safMessaging.serialization = (F4SESerializationInterface*)f4se->QueryInterface(kInterface_Serialization);
	if (!SAF::safMessaging.serialization) {
		_FATALERROR("couldn't get serialization interface");
		return false;
	}

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
	if (SAF::safMessaging.messaging)
		SAF::safMessaging.messaging->RegisterListener(SAF::safMessaging.pluginHandle, "F4SE", SAF::F4SEMessageHandler);
	
	if (SAF::safMessaging.papyrus)
		SAF::safMessaging.papyrus->Register(SAF::RegisterPapyrus);

	if (SAF::safMessaging.serialization) {
		SAF::safMessaging.serialization->SetUniqueID(SAF::safMessaging.pluginHandle, 'SAF');
		SAF::safMessaging.serialization->SetSaveCallback(SAF::safMessaging.pluginHandle, SAF::SaveCallback);
		SAF::safMessaging.serialization->SetLoadCallback(SAF::safMessaging.pluginHandle, SAF::LoadCallback);
		SAF::safMessaging.serialization->SetRevertCallback(SAF::safMessaging.pluginHandle, SAF::RevertCallback);
	}

	_DMESSAGE("SAF Loaded");

	return true;
}

};