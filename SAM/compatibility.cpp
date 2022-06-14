#include "compatibility.h"

#include "SAF/util.h"

bool** ffcKeyboardInput = nullptr;
bool** ffcPadInput = nullptr;

//LoadedPlugin* GetLoadedPlugin(const char* name)
//{

	//m_plugins
	//auto plugins = (std::vector<LoadedPlugin>*)((UInt64)&g_pluginManager + 0x48);

	//for (auto& plugin : *plugins) {
	//	if (plugin.info.name && !_stricmp(name, plugin.info.name))
	//		return &plugin;
	//}

	//return nullptr;
	//return  nullptr;
//}

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