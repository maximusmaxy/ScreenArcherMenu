#include "console.h"

#include "f4se/GameAPI.h"
#include "f4se/GameReferences.h"
#include "f4se/GameMenus.h"
#include "f4se/ObScript.h"

#include "f4se_common/SafeWrite.h"
#include "f4se_common/f4se_version.h"

#include "SAF/adjustments.h"
#include "SAF/util.h"

#include "pose.h"
#include "sam.h"
#include "lights.h"
#include "mfg.h"

#include <filesystem>

typedef bool (*_ParseObString)(void* paramInfo, void* scriptData, UInt32* opcodeOffsetPtr, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, char* str1);
RelocAddr<_ParseObString> ParseObString(0x4E37D0);

typedef bool (*_ParseObString2)(void* paramInfo, void* scriptData, UInt32* opcodeOffsetPtr, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, char* str1, char* str2);
RelocAddr<_ParseObString2> ParseObString2(0x4E37D0);

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

enum {
	kObscriptAdjustment = 1,
	kObscriptPose,
	kObscriptFaceMorphs,
	kObscriptLights
};

SAF::InsensitiveUInt32Map obscriptTypes = {
	{"a", kObscriptAdjustment},
	{"adjustment", kObscriptAdjustment},
	{"adjustments", kObscriptAdjustment},
	{"p", kObscriptPose},
	{"pose", kObscriptPose},
	{"poses", kObscriptPose},
	{"f", kObscriptFaceMorphs},
	{"facemorph", kObscriptFaceMorphs},
	{"facemorphs", kObscriptFaceMorphs},
	{"mfg", kObscriptFaceMorphs},
	{"l", kObscriptLights},
	{"light", kObscriptLights},
	{"lights", kObscriptLights}
};

std::string FindPathInSubfolder(const char* folder, const char* stem, const char* ext)
{
	std::stringstream ss;
	ss << stem;
	ss << ext;

	std::string filename = ss.str();

	for (auto& it : std::filesystem::recursive_directory_iterator(folder)) {
		if (!it.is_directory() && !_stricmp(it.path().filename().string().c_str(), filename.c_str())) {
			return it.path().string();
		}
	}

	return std::string();
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
	auto it = obscriptTypes.find(typeStr);
	if (it == obscriptTypes.end()) {
		ConsolePrintConcat("Could not recognize type: ", typeStr);
		return false;
	}

	UInt32 type = it->second;

	const char* folder;
	const char* ext;

	switch (type) {
	case kObscriptAdjustment: folder = ADJUSTMENTS_PATH; ext = ".json";  break;
	case kObscriptPose: folder = POSES_PATH; ext = ".json";  break;
	case kObscriptLights: folder = LIGHTS_PATH; ext = ".json";  break;
	case kObscriptFaceMorphs: folder = FACEMORPHS_PATH; ext = ".txt";  break;
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
		case kObscriptAdjustment: success = LoadAdjustmentPath(path.c_str()); break;
		case kObscriptPose: success = LoadPosePath(path.c_str()); break;
		case kObscriptLights: success = LoadLightsPath(path.c_str()); break;
		case kObscriptFaceMorphs: success = LoadMfgPath(path.c_str()); break;
		}
	}

	if (success)
		return true;

	const char* error;
	switch (type) {
	case kObscriptAdjustment: error = "Could not find adjustment: "; break;
	case kObscriptPose: error = "Could not find pose: "; break;
	case kObscriptLights: error = "Could not find lights: "; break;
	case kObscriptFaceMorphs: error = "Could not find morphs: "; break;
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

			//If string 2 exists
			if (str2[0]) 
			{
				if (selected.refr) {
					LoadSamFolder(str1, str2);
				}
			}
			//only string 1
			else {
				//check for reset
				if (!_stricmp(str1, "reset"))
				{
					ResetJsonPose();
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
			ToggleMenu();
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