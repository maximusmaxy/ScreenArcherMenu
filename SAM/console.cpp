#include "console.h"

#include "f4se/GameAPI.h"
#include "f4se/GameReferences.h"
#include "f4se/GameMenus.h"
#include "f4se/ObScript.h"

#include "f4se_common/SafeWrite.h"
#include "f4se_common/f4se_version.h"

#include "SAF/adjustments.h"
#include "SAF/util.h"
#include "SAF/types.h"

#include "pose.h"
#include "sam.h"
#include "lights.h"
#include "mfg.h"
#include "constants.h"
#include "camera.h"
#include "io.h"

#include <filesystem>

typedef bool (*_ParseObString)(void* paramInfo, void* scriptData, UInt32* opcodeOffsetPtr, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, char* str1);
RelocAddr<_ParseObString> ParseObString(0x4E37D0);

typedef bool (*_ParseObString2)(void* paramInfo, void* scriptData, UInt32* opcodeOffsetPtr, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, char* str1, char* str2);
RelocAddr<_ParseObString2> ParseObString2(0x4E37D0);

enum {
	kConsoleSingleReset = 1,
	kConsoleSingleReloadMenus,
	kConsoleSingleForceQuit
};

SAF::InsensitiveUInt32Map obscriptSingleMap = {
	{"reset", kConsoleSingleReset},
	{"reloadmenus", kConsoleSingleReloadMenus},
	{"forcequit", kConsoleSingleForceQuit}
};

enum {
	kConsolePairAdjustment = 1,
	kConsolePairPose,
	kConsolePairFaceMorphs,
	kConsolePairLights,
	kConsolePairCamera
};

SAF::InsensitiveUInt32Map obscriptPairMap = {
	{"a", kConsolePairAdjustment},
	{"adjustment", kConsolePairAdjustment},
	{"adjustments", kConsolePairAdjustment},
	{"p", kConsolePairPose},
	{"pose", kConsolePairPose},
	{"poses", kConsolePairPose},
	{"f", kConsolePairFaceMorphs},
	{"facemorph", kConsolePairFaceMorphs},
	{"facemorphs", kConsolePairFaceMorphs},
	{"mfg", kConsolePairFaceMorphs},
	{"l", kConsolePairLights},
	{"light", kConsolePairLights},
	{"lights", kConsolePairLights},
	{"c", kConsolePairCamera},
	{"camera", kConsolePairCamera}
};

// Show Bones
static ObScriptCommand* samObScriptCommand = nullptr;

void samObScriptInit()
{
	for (ObScriptCommand* iter = g_firstConsoleCommand; iter->opcode < (kObScript_NumConsoleCommands + kObScript_ConsoleOpBase); ++iter)
	{
		if (!strcmp(iter->longName, "ShowBones"))
		{
			samObScriptCommand = iter;
			break;
		}
	}

	if (!samObScriptCommand)
	{
		_DMESSAGE("Attempted to replace \"ShowBones\" console command and failed");
	}
}

void ConsolePrintConcat(const char* st1, const char* st2) {
	ConsoleManager* mgr = *g_console;
	if (!mgr)
		return;

	std::stringstream ss;
	ss << st1;
	ss << st2;
	ss << '\n';

	CALL_MEMBER_FN(mgr, Print)(ss.str().c_str());
}

std::string FindPathInSubfolder(const char* folder, const char* stem, const char* ext)
{
	std::wstringstream ss;
	ss << stem;
	ss << ext;
	const std::wstring filename = ss.str();

	for (auto& it : std::filesystem::recursive_directory_iterator(folder)) {
		if (!it.is_directory()) {
			const auto& native = it.path().native();
			const size_t pos = native.find_last_of(L"\\/");
			if (pos != std::wstring::npos && !_wcsicmp(native.c_str() + pos + 1, filename.c_str()))
				return std::filesystem::path(native).string();
		}
	}

	return {};
}

bool LoadSamFile(const char* filename)
{
	if (LoadPoseFile(filename))
		return true;

	if (LoadMfgFile(filename))
		return true;

	if (LoadAdjustmentFile(filename))
		return true;

	if (LoadLightsFile(filename))
		return true;

	ConsolePrintConcat("Could not find: ", filename);

	return false;
}

bool LoadSamFolder(const char* typeStr, const char* filename)
{
	auto it = obscriptPairMap.find(typeStr);
	if (it == obscriptPairMap.end()) {
		ConsolePrintConcat("Could not recognize type: ", typeStr);
		return false;
	}

	UInt32 type = it->second;

	const char* folder;
	const char* ext;

	switch (type) {
	case kConsolePairAdjustment: folder = ADJUSTMENTS_PATH; ext = ".json";  break;
	case kConsolePairPose: folder = POSES_PATH; ext = ".json";  break;
	case kConsolePairLights: folder = LIGHTS_PATH; ext = ".json";  break;
	case kConsolePairFaceMorphs: folder = FACEMORPHS_PATH; ext = ".txt";  break;
	case kConsolePairCamera: folder = CAMERA_PATH; ext = ".json"; break;
	default: return false;
	}

	std::string path;

	//if string is a fully extended path, use that instead of recursive subfolder search
	if (strpbrk(filename, "/\\")) {
		path = GetPathWithExtension(folder, filename, ext);
	}
	else {
		path = FindPathInSubfolder(folder, filename, ext);
	}

	bool success = false;

	if (!path.empty()) {
		switch (type) {
		case kConsolePairAdjustment: success = LoadAdjustmentPath(path.c_str()); break;
		case kConsolePairPose: success = LoadPosePath(path.c_str()); break;
		case kConsolePairLights: success = LoadLightsPath(path.c_str()); break;
		case kConsolePairFaceMorphs: success = LoadMfgPath(path.c_str()); break;
		case kConsolePairCamera: success = LoadCameraPath(path.c_str()); break;
		default: return false;
		}
	}

	if (success)
		return true;

	const char* error;
	switch (type) {
	case kConsolePairAdjustment: error = "Could not find adjustment: "; break;
	case kConsolePairPose: error = "Could not find pose: "; break;
	case kConsolePairLights: error = "Could not find lights: "; break;
	case kConsolePairFaceMorphs: error = "Could not find morphs: "; break;
	case kConsolePairCamera: error = "Could not find camera state: "; break;
	default: return false;
	}

	ConsolePrintConcat(error, filename);

	return false;
}

bool samObScriptExecute(void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffsetPtr)
{
	char buf[0x400];
	memset(buf, 0, sizeof(buf));
	char* str1 = &buf[0];
	char* str2 = &buf[0x200];

	if ((*ParseObString2)(paramInfo, scriptData, (UInt32*)opcodeOffsetPtr, thisObj, containingObj, scriptObj, locals, str1, str2)) 
	{
		//yes param 1
		if (str1[0]) 
		{
			//if there is no selected refr, temporarily select it
			bool notSelected = !selected.refr;
			if (notSelected)
			{
				TESObjectREFR* refr = GetRefr();
				if (refr) 
					selected.Update(refr);
			}

			//If param 2 exists
			if (str2[0]) 
			{
				if (selected.refr) {
					LoadSamFolder(str1, str2);
				}
			}
			//only param 1
			else {

				//check for single commands
				auto it = obscriptSingleMap.find(str1);
				if (it != obscriptSingleMap.end()) {
					switch (it->second) {
					case kConsoleSingleReset: ResetJsonPose(); break;
					case kConsoleSingleReloadMenus: ReloadJsonMenus(); break;
					case kConsoleSingleForceQuit: samManager.ForceQuit(); break;
					}
				}

				//open file name
				else
				{
					if (selected.refr) {
						LoadSamFile(str1);
					}
				}
			}

			//clear temp ref
			if (notSelected) 
				selected.Clear();

		}

		//no param1, open/close menu
		else 
		{
			samManager.ToggleMenu();
		}

		return true;
	}

	return false;;
}

void samObScriptCommit()
{
	if (!samObScriptCommand) 
		return; 

	ObScriptCommand cmd = *samObScriptCommand;

	cmd.longName = "screenarchermenu";
	cmd.shortName = "sm";
	cmd.helpText = "Opens/Closes the screen archer menu";
	cmd.needsParent = 0;
	cmd.numParams = 2;
	cmd.execute = samObScriptExecute;
	cmd.flags = 0;
	cmd.eval = nullptr;

	static ObScriptParam samConsoleParams[2] = {
		{ "String", 0, 1 },
		{ "String", 0, 1 }
	};

	cmd.params = samConsoleParams;

	SafeWriteBuf((uintptr_t)samObScriptCommand, &cmd, sizeof(cmd));
}