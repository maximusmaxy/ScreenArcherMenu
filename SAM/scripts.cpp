#include "scripts.h"

#include "f4se/GameTypes.h"
#include "f4se/GameForms.h"
#include "f4se/GameReferences.h"
#include "f4se/GameMenus.h"

#include "f4se/ObScript.h"

typedef void(*_ToggleMenusInternal)();
RelocAddr<_ToggleMenusInternal> ToggleMenusInternal(0x517FF0);

typedef void(*_SetMenusDisabledInternal)(bool hidden);
RelocAddr<_SetMenusDisabledInternal> SetMenusDisabledInternal(0xAE5BB0);

#define uiVisible (reinterpret_cast<bool*>(g_ui.GetUIntPtr()) + 0x248)

bool GetMenusHidden() {
	return !(*uiVisible);
}

void SetMenusHidden(bool hidden) {
	if (hidden != GetMenusHidden())
		ToggleMenusInternal();
}

bool ToggleMenusHidden() {
	ToggleMenusInternal();
	return GetMenusHidden();
}

class Script
	{
public:
	//0 vftable script vftable
	//8 data null
	//10 flags 0x400A
	//14 formid 0x0FF082C73 I'm guessing this is a dynamically generated runtime id
	//18 unk  0
	//1A formType 22 script
	//1B unk1b 0x02, 0x4C
	//1C pad1c 0x01F6
	//20 is 0
	//28 0x18 compiled script size
	//30 is 0
	//31 is 0
	//32 is 1 (byte)
	//38 is a pointer to the cstr "mfg morphs x x"
	//40 pointer to compiled script data
	//54 float -1
	UInt64 vfTable;		//0
	UInt64 data;		//8
	UInt32 flags;		//10
	UInt32 formId;		//14
	UInt16 unk1;		//18
	UInt8 formType;		//1A
	UInt8 unk2;			//1B
	UInt32 unk3;		//1C
	UInt64 unk4;		//20
	UInt64 size;		//28
	UInt8 unk5;			//30
	UInt8 unk6;			//31
	UInt8 unk7;			//32
	UInt8 unk8;			//33
	UInt32 unk9;		//34
	char* command;		//38
	byte* scriptData;	//40
	UInt64 unk10;		//48
	UInt32 unk11;		//50
	float unk12;		//54
	UInt64 unk13;		//58
	UInt64 unk14;		//60
	UInt64 unk15;		//68
	UInt64 unk16;		//70
	UInt64 unk17;		//78
};

struct ParamInfo {
	const char* name;
	UInt32 type;
	UInt32 optional;
};

struct ActionObject {
	TESForm* form;
	UInt32 flags;
	UInt32 unk;
};

struct ScriptLocal {
	UInt32 id;
	float value;
	bool isInteger;
	UInt8 unk1;
	UInt16 unk2;
};

struct ScriptEffectData {
	bool scriptEffectStart;
	bool scriptEffectFinish;
	UInt16 unk1;
	float secondsElapsed;
};

struct ScriptLocals
{
	Script* script;								//00
	UInt64 flags;								//08
	tList<ActionObject>* actionObjects;			//10
	tList<ScriptLocal>* scriptLocals;			//18
	ScriptEffectData* scriptEffectData;			//20 
};

RelocAddr<ObScript_Execute> mfgExecute(0x5266C0);
RelocAddr<UInt64> scriptVfTable(0x2C97C48);

//RelocPtr<char*> paramStringPtr(0x2C4C148);
//RelocPtr<char*> paramIntPtr(0x2CA3920);

const char* mfgComand = "mfg morphs %d %d";

//Not using this anymore but is a good reference on how to manually execute console commands if needed in the future
void ExecuteMfgObScript(TESObjectREFR* refr, UInt32 id, UInt32 scale) {

	id = max(0, min(49, id));
	scale = max(0, min(100, scale));

	//E801140003000600
	//6D6F727068736EXX
	//0000006EXX000000
	byte scriptData[24] = {
		0xE8, 01, 0x14, 00, 03, 00, 06, 00,
		0x6D, 0x6F, 0x72, 0x70, 0x68, 0x73, 0x6E, (byte)id,
		00, 00, 00, 0x6E, (byte)scale, 00, 00, 00 
	};

	char commandBuf[32];
	sprintf_s(commandBuf, 32, mfgComand, id, scale);

	Script script;
	memset(&script, 0, sizeof(script));
	script.vfTable = scriptVfTable;
	script.flags = 0x400A;
	script.formId = 0xDEADBEEF;
	script.formType = kFormType_SCPT;
	script.unk2 = 2; //0x4C
	script.size = 0x18;
	script.unk7 = 1;
	script.command = commandBuf;
	script.scriptData = scriptData;
	script.unk12 = -1.0;

	ScriptLocals scriptLocals;
	scriptLocals.script = &script;
	scriptLocals.flags = 0;
	scriptLocals.actionObjects = nullptr;
	tList<ScriptLocal> scriptLocalList;
	scriptLocals.scriptLocals = &scriptLocalList;
	scriptLocals.scriptEffectData = nullptr;

	ParamInfo params[3];
	params[0].name = "String";
	params[0].type = 0;
	params[0].optional = 0;
	params[1].name = "Integer";
	params[1].type = 1;
	params[1].optional = 1;
	params[2].name = "Integer";
	params[2].type = 1;
	params[2].optional = 1;

	double ret;
	UInt64 offset = 4;

	bool result = (*mfgExecute)(params, &scriptData, refr, nullptr, &script, &scriptLocals, &ret, &offset);

	if (!result) {
		_DMESSAGE("Result was false?");
	}
}

RelocAddr<ObScript_Execute> lookExecute(0x4FB400);

const char* lookCommand = "look %x";

//This is unfinished, an alternative method was found but might need this again later
void ExecuteLookObScript(TESObjectREFR* refr, UInt32 target) {

	//E801140003000600
	//6D6F727068736EXX
	//0000006EXX000000
	byte scriptData[24] = {
		0xE8, 01, 0x14, 00, 03, 00, 06, 00,
		0x6D, 0x6F, 0x72, 0x70, 0x68, 0x73, 0x6E, 00,
		00, 00, 00, 0x6E, 00, 00, 00, 00 
	};

	//char commandBuf[32];
	//sprintf_s(commandBuf, 32, lookCommand, id, scale);

	Script script;
	memset(&script, 0, sizeof(script));
	script.vfTable = scriptVfTable;
	script.flags = 0x400A;
	script.formId = 0xDEADBEEF;
	script.formType = kFormType_SCPT;
	script.unk2 = 2; //0x4C
	script.size = 0x18;
	script.unk7 = 1;
	//script.command = commandBuf;
	//script.scriptData = scriptData;
	script.unk12 = -1.0;

	ScriptLocals scriptLocals;
	scriptLocals.script = &script;
	scriptLocals.flags = 0;
	scriptLocals.actionObjects = nullptr;
	tList<ScriptLocal> scriptLocalList;
	scriptLocals.scriptLocals = &scriptLocalList;
	scriptLocals.scriptEffectData = nullptr;

	ParamInfo params[2];
	params[0].name = "ObjectReferenceID";
	params[0].type = 4;
	params[0].optional = 0;
	params[1].name = "Integer (Optional)";
	params[1].type = 1;
	params[1].optional = 1;

	double ret;
	UInt64 offset = 4;

	bool result = (*lookExecute)(params, &scriptData, refr, nullptr, &script, &scriptLocals, &ret, &offset);

	if (!result) {
		_DMESSAGE("Result was false?");
	}
}