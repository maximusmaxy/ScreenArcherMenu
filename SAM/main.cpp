#include "f4se/PluginAPI.h"

#include "constants.h"
#include "sam.h"
#include "console.h"
#include "scaleform.h"
#include "papyrus.h"
#include "messaging.h"

#include "f4se_common/f4se_version.h"

#include <shlobj.h>

extern "C"
{

bool F4SEPlugin_Query(const F4SEInterface* f4se, PluginInfo* info)
{
	gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Fallout4\\F4SE\\sam.log");
	_DMESSAGE("Screen Archer Menu");

	info->infoVersion =	PluginInfo::kInfoVersion;
	info->name =		SAM_MENU_NAME;
	info->version =		3;

	samMessaging.pluginHandle = f4se->GetPluginHandle();

	if(f4se->isEditor)
	{
		_FATALERROR("loaded in editor, marking as incompatible");
		return false;
	}
	else if (f4se->runtimeVersion != RUNTIME_VERSION_1_10_163)
	{
		_FATALERROR("unsupported runtime version %08X", f4se->runtimeVersion);
		return false;
	}

	samMessaging.f4se = (F4SEInterface*)f4se;

	samMessaging.scaleform = (F4SEScaleformInterface *)f4se->QueryInterface(kInterface_Scaleform);
	if (!samMessaging.scaleform)
	{
		_FATALERROR("couldn't get scaleform interface");
		return false;
	}

	samMessaging.messaging = (F4SEMessagingInterface*)f4se->QueryInterface(kInterface_Messaging);
	if(!samMessaging.messaging)
	{
		_FATALERROR("couldn't get messaging interface");
		return false;
	}

	samMessaging.papyrus = (F4SEPapyrusInterface *)f4se->QueryInterface(kInterface_Papyrus);
	if(!samMessaging.papyrus)
	{
		_WARNING("couldn't get papyrus interface");
	}

	samMessaging.serialization = (F4SESerializationInterface*)f4se->QueryInterface(kInterface_Serialization);
	if (!samMessaging.serialization) {
		_FATALERROR("couldn't get serialization interface");
		return false;
	}

	samObScriptInit();

	return true;
}

bool F4SEPlugin_Load(const F4SEInterface* f4se)
{
	if (samMessaging.scaleform)
		samMessaging.scaleform->Register(SAM_MENU_NAME, RegisterScaleform);

	if (samMessaging.messaging)
		samMessaging.messaging->RegisterListener(samMessaging.pluginHandle, "F4SE", F4SEMessageHandler);

	if (samMessaging.papyrus)
		samMessaging.papyrus->Register(RegisterPapyrus);

	if (samMessaging.serialization) {
		samMessaging.serialization->SetUniqueID(samMessaging.pluginHandle, 'SAM');
		samMessaging.serialization->SetSaveCallback(samMessaging.pluginHandle, SamSerializeSave);
		samMessaging.serialization->SetLoadCallback(samMessaging.pluginHandle, SamSerializeLoad);
		samMessaging.serialization->SetRevertCallback(samMessaging.pluginHandle, SamSerializeRevert);
	}
	samObScriptCommit();

	_DMESSAGE("Screen Archer Menu Loaded");
	return true;
}

};