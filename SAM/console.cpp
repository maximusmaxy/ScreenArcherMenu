#include "console.h"

#include "f4se/GameAPI.h"
#include "f4se/GameReferences.h"
#include "f4se/GameMenus.h"
#include "f4se/ObScript.h"

#include "f4se_common/SafeWrite.h"
#include "f4se_common/f4se_version.h"

#include "pose.h"
#include "sam.h"

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

typedef bool (*ParseObString)(void* paramInfo, void* scriptData, UInt32* opcodeOffsetPtr, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, char* parsed);
RelocAddr<ParseObString> parseObString(0x4E37D0);

bool samObScriptExecute(void* paramInfo, void* scriptData, TESObjectREFR* thisObj, void* containingObj, void* scriptObj, void* locals, double* result, void* opcodeOffsetPtr)
{
	char parsedString[0x200];
	memset(parsedString, 0, sizeof(parsedString));

	if ((*parseObString)(paramInfo, scriptData, (UInt32*)opcodeOffsetPtr, thisObj, containingObj, scriptObj, locals, parsedString)) {

		//yes param1, open file
		if (parsedString[0]) {

			//if there is no selected refr, temporarily select it
			bool notSelected = !selected.refr;
			if (notSelected) {
				TESObjectREFR* refr = GetRefr();
				if (refr) selected.Update(refr);
			}

			//check for reset
			if (!_stricmp(parsedString, "reset")) {
				ResetJsonPose();
			}

			//open file name
			else {
				OpenSamFile(std::string(parsedString));
			}

			//clear temp ref
			if (notSelected) selected.Clear();
		}

		//no param1, open/close menu
		else {
			ToggleMenu();
		}

		return true;
	}

	return false;;
}

void samObScriptCommit()
{
	if (!samObScriptCommand) return; 

	ObScriptCommand cmd = *samObScriptCommand;

	cmd.longName = "screenarchermenu";
	cmd.shortName = "sm";
	cmd.helpText = "Opens/Closes the screen archer menu";
	cmd.needsParent = 0;
	cmd.numParams = 1;
	cmd.execute = samObScriptExecute;
	cmd.flags = 0;
	cmd.eval = nullptr;

	static ObScriptParam samConsoleParams[1] = {
		{ "String", 0, 1 }
	};

	cmd.params = samConsoleParams;

	SafeWriteBuf((uintptr_t)samObScriptCommand, &cmd, sizeof(cmd));
}
