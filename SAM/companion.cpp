#include "companion.h"

#include "io.h"
#include "constants.h"
#include "sam.h"

#include "SAF/io.h"
#include "SAF/util.h"

#include "common/IFileStream.h"

#include "f4se/GameReferences.h"

#include <filesystem>
#include <ostream>
#include <chrono>

//TODO figure out how to calculate date
UInt32 espDate = 0x2CFF;

//TODO figure out what this is
UInt32 espGroupId = 0x1B4;

//TODO will require additional masters for custom races so mod id will change
UInt32 espModId = 1;

struct AAData
{
	UInt32 flags;
	const char* maleWorldModel;
	const char* femaleWorldModel;
	const char* male1stPerson;
	const char* female1stPerson;
};

AAData aaBodyData {
	0xC00008,
	"Actors\\Character\\CharacterAssets\\MaleBody.nif",
	"Actors\\Character\\CharacterAssets\\FemaleBody.nif",
	"Actors\\Character\\CharacterAssets\\1stPersonMaleBody.nif",
	"Actors\\Character\\CharacterAssets\\1stPersonFemaleBody.nif"
};

AAData aaHandsData {
	0x30,
	"Actors\\Character\\CharacterAssets\\MaleHands.nif",
	"Actors\\Character\\CharacterAssets\\FemaleHands.nif",
	"Actors\\Character\\CharacterAssets\\1stPersonMaleHands.nif",
	"Actors\\Character\\CharacterAssets\\1stpersonFemaleHands.nif"
};

struct QuestData {
	std::string edid;
	std::string fullName;
	std::string questScript;
	std::string fragmentScript;
	const char* companionName;
};

struct SceneData {
	std::string edid;
	std::string phase;

	UInt32 index;
	bool action1;

	UInt32 formId;
	UInt32 pPositive;
	UInt32 pNegative;
	UInt32 pNeutral;
	UInt32 pQuestion;
	UInt32 nPositive;
	UInt32 nNegative;
	UInt32 nNeutral;
	UInt32 nQuestion;
};

struct ConditionData {
	UInt32 signature;
	UInt32 type;
	float compValue;
	UInt32 func;
	UInt32 alias;
	UInt32 unk;
	UInt32 runOn;
	UInt32 unk2;
	SInt32 unk3;
};

struct InfoData {
	UInt32 questId;
	UInt32 topicId;
	UInt32 infoId;

	bool openInventory;
	UInt32 responseFlags;
	UInt16 setParentQuestStage;
	UInt32 previousInfo;
	UInt32 sharedInfo;
	UInt32 startScene;

	const char* response;
	const char* prompt;
	const char* phase;

	ConditionData condition1;
	ConditionData condition2;
};

struct GreetingData {
	std::string edid;

	const char* hirePhase;
	const char* rehirePhase;
	const char* talkPhase;

	const char* hireResponse;
	const char* rehireResponse;
	const char* talkResponse;
};

struct TextureSet {
	std::string edid;
	std::string material;

	UInt32 formId;
	UInt32 length;

	std::vector<std::pair<UInt32, std::string>> textures;

	TextureSet(std::string& edid, UInt32 id) : edid(edid), formId(id) {}
};

UInt32 GetTextureSetLength(TextureSet& set) {
	UInt32 length = 0x28;

	if (!set.material.empty())
		length += set.material.length() + 7;

	if (set.textures.size()) {
		for (auto& kvp : set.textures) {
			length += kvp.second.length() + 7;
		}
	}

	return length;
}

struct HeadPart {
	UInt32 formId;
	std::string edid;
	std::string fullName;
	std::string modelFilename;
	UInt32 flags;
	UInt32 type;
	UInt32 textureSet;

	std::vector<UInt32> extraParts;
	std::vector<std::pair<UInt32, std::string>> partTypes;
};

struct FaceTintingLayer {
	UInt16 type;
	UInt16 index;
	UInt8 value;
	UInt8 red;
	UInt8 green;
	UInt8 blue;
	UInt8 alpha;
	UInt16 colorIndex;
};

struct FaceMorph {
	UInt32 index;
	float posX;
	float posY;
	float posZ;
	float rotX;
	float rotY;
	float rotZ;
	float scale;
	UInt32 unk0;
	UInt32 unk1;
};

class Companion {
public:
	std::string name;
	std::string author;

	UInt32 raceId;
	float scale;
	UInt32 voice;
	UInt32 defaultTemplate;
	UInt32 classId;
	UInt32 hairColorId;
	UInt32 combatStyleId;

	float heightMin;
	float heightMax;
	float weightThin;
	float weightMuscular;
	float weightFat;

	float textureLightingRed;
	float textureLightingGreen;
	float textureLightingBlue;
	float textureLightingAlpha;

	float bodyMorphRegionHead;
	float bodyMorphRegionUpperTorso;
	float bodyMorphRegionArms;
	float bodyMorphRegionLowerTorso;
	float bodyMorphRegionLegs;

	float faceMorphIntensity;

	std::vector<HeadPart> headParts;
	std::vector<TextureSet> textureSets;
	std::vector<UInt32> morphKeys;
	std::vector<float> morphValues;
	std::vector<FaceTintingLayer> faceTintingLayers;
	std::vector<FaceMorph> faceMorphs;

	//response Ids
	enum {
		greetingUnhired,
		greetingHired,
		greetingRehire,
		comeWithMe,
		nevermind,
		trade,
		thoughts,
		dismissed,

		responseMax
	};

	std::string responses[responseMax];
	
	//form ids
	enum {
		npc = 0x800,
		refr,
		perk,
		sandbox,
		quest,
		armor,
		outfit,
		keyword,
		textureBody,
		textureHands,
		textureHead,
		textureHeadRear,
		textureEyes,
		aaTorso,
		aaHands,
		hpHead,
		hpHeadRear,
		hpMouth,
		hpMouthShadow,
		hpEyes,
		hpEyesLashes,
		hpEyesAO,
		hpEyesWet,
		hpHair,
		hpBeard,
		
		scenePickup,
		dialPickupPlayerPositive,
		dialPickupPlayerNegative,
		dialPickupPlayerTalk,
		dialPickupPlayerQuestion,
		dialPickupNpcPositive,
		dialPickupNpcNegative,
		dialPickupNpcTalk,
		dialPickupNpcQuestion,
		infoPickupPlayerPositive,
		infoPickupPlayerNegative,
		infoPickupPlayerTalk,
		infoPickupPlayerQuestion,
		infoPickupNpcPositive,
		infoPickupNpcNegative,
		infoPickupNpcTalk,
		infoPickupNpcQuestion,

		sceneDismiss,
		dialDismissPlayerPositive,
		dialDismissPlayerNegative,
		dialDismissPlayerTalk,
		dialDismissPlayerQuestion,
		dialDismissNpcPositive,
		dialDismissNpcNegative,
		dialDismissNpcTalk,
		dialDismissNpcQuestion,
		infoDismissPlayerPositive,
		infoDismissPlayerNegative,
		infoDismissPlayerTalk,
		infoDismissPlayerQuestion,
		infoDismissNpcPositive,
		infoDismissNpcNegative,
		infoDismissNpcTalk,
		infoDismissNpcQuestion,

		sceneTalk,
		dialTalkPlayerPositive,
		dialTalkPlayerNegative,
		dialTalkPlayerTalk,
		dialTalkPlayerQuestion,
		dialTalkNpcPositive,
		dialTalkNpcNegative,
		dialTalkNpcTalk,
		dialTalkNpcQuestion,
		infoTalkPlayerPositive,
		infoTalkPlayerNegative,
		infoTalkPlayerTalk,
		infoTalkPlayerQuestion,
		infoTalkNpcPositive,
		infoTalkNpcNegative,
		infoTalkNpcTalk,
		infoTalkNpcQuestion,

		sceneRehire,
		dialRehirePlayerPositive,
		dialRehirePlayerNegative,
		dialRehirePlayerTalk,
		dialRehirePlayerQuestion,
		dialRehireNpcPositive,
		dialRehireNpcNegative,
		dialRehireNpcTalk,
		dialRehireNpcQuestion,
		infoRehirePlayerPositive,
		infoRehirePlayerNegative,
		infoRehirePlayerTalk,
		infoRehirePlayerQuestion,
		infoRehireNpcPositive,
		infoRehireNpcNegative,
		infoRehireNpcTalk,
		infoRehireNpcQuestion,

		dialGreeting,
		infoGreetingPickup,
		infoGreetingRehire,
		infoGreetingTalk,

		max
	};
};

Companion companionData;

void Write(std::ostream& stream, std::string& str)
{
	UInt16 len = str.length() + 1;
	stream.write(reinterpret_cast<char*>(&len), sizeof(UInt16));
	stream.write(str.data(), len);
}

void Write(std::ostream& stream, const char* str)
{
	UInt16 len = strlen(str) + 1;
	stream.write(reinterpret_cast<char*>(&len), sizeof(UInt16));
	stream.write(str, len);
}

void WriteBytes(std::ostream& stream, std::string& str)
{
	UInt16 len = str.length();
	stream.write(reinterpret_cast<char*>(&len), sizeof(UInt16));
	stream.write(str.data(), len);
}

void WriteBytes(std::ostream& stream, const char* str)
{
	UInt16 len = strlen(str);
	stream.write(reinterpret_cast<char*>(&len), sizeof(UInt16));
	stream.write(str, len);
}

UInt32 GetFormId(UInt32 modId, UInt32 formId) {
	return (modId << 24) | (formId & 0xFFF);
}

void WriteFormId(std::ostream& stream, UInt32 modId, UInt32 formId) {
	UInt32 id = GetFormId(modId, formId);
	stream.write(reinterpret_cast<char*>(&id), sizeof(UInt32));
}

void WriteFormValue(std::ostream& stream, UInt32 type, UInt32 modId, UInt32 formId) {
	Write(stream, type);
	Write<UInt16>(stream, 4);
	WriteFormId(stream, modId, formId);
}

template <class Type>
void WriteValue(std::ostream& stream, UInt32 type, Type val)
{
	Write(stream, type);
	Write<UInt16>(stream, sizeof(Type));
	Write(stream, val);
}

void WriteValue(std::ostream& stream, UInt32 type, const char* str)
{
	Write(stream, type);
	Write(stream, str);
}

void WriteValue(std::ostream& stream, UInt32 type, std::string& str)
{
	Write(stream, type);
	Write(stream, str);
}

template <class T1, class T2>
void WritePair(std::ostream& stream, UInt32 type, T1 val1, T2 val2)
{
	Write(stream, type);
	Write<UInt16>(stream, sizeof(T1) + sizeof(T2));
	Write(stream, val1);
	Write(stream, val2);
}

void WriteNull(std::ostream& stream, UInt32 type)
{
	Write(stream, type);
	Write<UInt16>(stream, 0);
}

bool WriteResource(std::ostream& stream, const char* path)
{
	std::ifstream resourceStream;
	resourceStream.open(path, std::ios::in | std::ios::binary);
	if (resourceStream.fail()) {
		_Log("Failed to open resource: ", path);
		return false;
	}

	//TODO test this
	stream << resourceStream.rdbuf();

	return true;
}

void WriteGroup(std::ostream& stream, UInt32 len, UInt32 type, UInt32 flags, UInt32 unk) {
	Write(stream, 'PURG');
	Write(stream, len); 
	Write(stream, type);
	Write(stream, flags);
	Write(stream, espDate);
	Write(stream, unk);
}

void WriteHeader(std::ostream& stream, UInt32 type, UInt32 len, UInt32 flags, UInt32 formId) {
	Write(stream, 'TFTO');
	Write(stream, len); 
	Write(stream, flags);
	WriteFormId(stream, 1, formId);
	Write(stream, espDate);
	Write(stream, 131); //version
}

void WriteScript(std::ostream& stream, const char* name, UInt8 flags, UInt16 len) {
	WriteBytes(stream, name);
	Write(stream, flags);
	Write(stream, len);
}

void WriteScriptProperty(std::ostream& stream, const char* key, UInt8 type, UInt8 flags, UInt32 value)
{
	WriteBytes(stream, key);
	Write<UInt8>(stream, type);
	Write<UInt8>(stream, flags);
	Write(stream, value);
}

void WriteScriptProperty(std::ostream& stream, const char* key, UInt8 type, UInt8 flags, bool value)
{
	WriteBytes(stream, key);
	Write<UInt8>(stream, type);
	Write<UInt8>(stream, flags);
	Write<UInt8>(stream, value ? 1 : 0);
}

void WriteScriptObject(std::ostream& stream, const char* key, UInt8 type, UInt8 flags, UInt32 value, UInt32 object)
{
	WriteBytes(stream, key);
	Write<UInt8>(stream, type);
	Write<UInt8>(stream, flags);
	Write(stream, value);
	Write(stream, object);
}

void WriteScriptFragment(std::ostream& stream, UInt16 stage, UInt16 unk, UInt32 stageIndex, UInt8 unk2, const char* scriptName, const char* fragmentName) {
	Write(stream, stage);
	Write(stream, unk);
	Write(stream, stageIndex);
	Write(stream, unk2);
	WriteBytes(stream, scriptName);
	WriteBytes(stream, fragmentName);
}

void WriteCondition(std::ostream& stream, ConditionData& data) {
	Write(stream, data.signature);
	Write<UInt16>(stream, 0x20);
	Write(stream, data.type);
	Write(stream, data.compValue);
	Write(stream, data.func);
	Write(stream, data.alias);
	Write(stream, data.unk);
	Write(stream, data.runOn);
	Write(stream, data.unk2);
	Write(stream, data.unk3);
}

void WriteBounds(std::ostream& stream, UInt16 x1, UInt16 y1, UInt16 z1, UInt16 x2, UInt16 y2, UInt16 z2)
{
	Write(stream, 'DNBO');
	Write<UInt16>(stream, 0xC);
	Write<SInt16>(stream, x1);
	Write<SInt16>(stream, y1);
	Write<SInt16>(stream, z1);
	Write<SInt16>(stream, x2);
	Write<SInt16>(stream, y2);
	Write<SInt16>(stream, z2);
}

void WriteKeyword(std::ostream& stream, std::string& keyword) {
	WriteHeader(stream, 'DWYK', 0x1B + keyword.length(), 0, Companion::keyword);
	Write(stream, 'DIDE');
	Write(stream, keyword);
	WriteValue(stream, 'MANC', 0xFFFFFF);
	WriteValue(stream, 'MANT', 0);
}

void WriteTextureSet(std::ostream& stream, TextureSet& set) {
	WriteHeader(stream, 'TSXT', set.length, 0, set.formId);
	WriteValue(stream, 'DIDE', set.edid);
	WriteBounds(stream, -8, -30, -20, 7, 30, 20);

	if (set.textures.size()) {
		for (auto& kvp : set.textures) {
			Write(stream, kvp.first);
			Write(stream, kvp.second);
		}
	}

	WriteValue<UInt16>(stream, 'MAND', 2); //facegen textures

	if (!set.material.empty()) {
		Write(stream, 'MANM');
		Write(stream, set.material);
	}
}

void WriteModel(std::ostream& stream, UInt32 mType, UInt32 tType, const char* path)
{
	Write(stream, mType);
	Write(stream, path);

	Write(stream, tType);
	Write<UInt16>(stream, 0x14);
	Write(stream, 4);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);
}

void WriteArmorAddon(std::ostream& stream, std::string& edid, AAData& data, UInt32 formId, UInt32 textureId, UInt32 raceId)
{
	WriteHeader(stream, 'AMRA', 0, 0, formId); //TODO len

	Write(stream, 'DIDE');
	Write(stream, edid);

	WriteValue(stream, '2DOB', data.flags);
	WriteValue(stream, 'MANR', 0x13746);

	Write(stream, 'MAND');
	Write<UInt16>(stream, 0xC);
	Write<UInt8>(stream, 0); //male prio
	Write<UInt8>(stream, 0); //female prio
	Write<UInt8>(stream, 0); //male weight
	Write<UInt8>(stream, 0); //female weight
	Write<UInt16>(stream, 0x2); //unk
	Write<UInt8>(stream, 0); //detection sound
	Write<UInt8>(stream, 0); //unk2
	Write<float>(stream, 0.0f); //weapon adjust

	WriteModel(stream, '2DOM', 'T2OM', data.maleWorldModel);
	WriteModel(stream, '3DOM', 'T3OM', data.femaleWorldModel);
	WriteModel(stream, '4DOM', 'T4OM', data.male1stPerson);
	WriteModel(stream, '5DOM', 'T5OM', data.female1stPerson);

	WriteFormValue(stream, '0MAN', espModId, textureId);
	WriteFormValue(stream, '1MAN', espModId, textureId);

	//additional races
	WriteValue(stream, 'LDOM', 0xE8D09);
	WriteValue(stream, 'LDOM', 0xEAFB6);
	WriteValue(stream, 'LDOM', 0x10BD65);
	WriteValue(stream, 'LDOM', 0x11D83F);

	//TODO compare against other additional races to prevent duplicates
	if (raceId != 0x13746)
		WriteValue(stream, 'LDOM', raceId);
}

void WriteArmor(std::ostream& stream, std::string& edid, UInt32 formId, UInt32 raceId, UInt32 bodyId, UInt32 handsId) {
	WriteHeader(stream, 'OMRA', 0, 0, formId);

	Write(stream, 'DIDE');
	Write(stream, edid);

	WriteBounds(stream, 0, 0, 0, 0, 0, 0);

	WriteValue(stream, '2DOB', 0x38);
	WriteValue(stream, 'MANR', raceId);
	WriteValue<UInt8>(stream, 'CSED', 0);

	WriteValue<UInt16>(stream, 'XDNI', 0);
	WriteFormValue(stream, 'LDOM', espModId, bodyId);
	WriteValue<UInt16>(stream, 'XDNI', 0);
	WriteFormValue(stream, 'LDOM', espModId, handsId);

	Write(stream, 'ATAD');
	Write<UInt16>(stream, 0xC);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);

	Write(stream, 'MANF');
	Write<UInt16>(stream, 8);
	Write(stream, 0);
	Write(stream, 0);
}

void WriteOutfit(std::ostream& stream, std::string& edid, UInt32 formId, UInt32 outfitId) {
	WriteHeader(stream, 'TFTO', 0x11 + edid.length(), 0, formId);
	Write(stream, 'DIDE');
	Write(stream, edid);
	WriteValue(stream, 'MANI', outfitId);
}

void WriteHeadPart(std::ostream& stream, HeadPart& data) {
	WriteHeader(stream, 'TPDH', 0, 0, data.formId); //TODO len
	WriteValue(stream, 'DIDE', data.edid);
	WriteValue(stream, 'LLUF', data.fullName);
	WriteValue(stream, 'LDOM', data.modelFilename);
	Write(stream, 'TDOM');
	Write<UInt16>(stream, 0x14);
	Write(stream, 4);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);
	WriteValue<UInt8>(stream, 'ATAD', data.flags);
	WriteValue(stream, 'MANP', data.type);
	
	for (auto& part : data.extraParts) {
		WriteValue(stream, 'MANH', part);
	}

	for (auto& type : data.partTypes) {
		WriteValue(stream, '0MAN', type.first);
		WriteValue(stream, '1MAN', type.second);
	}

	WriteValue(stream, 'MANT', data.textureSet);
	WriteValue(stream, 'MANR', 0xA8026); //TODO might not be able to assume this
}

void WritePerk(std::ostream& stream, std::string& edid) {
	WriteHeader(stream, 'KREP', 0x31 + edid.length(), 0, Companion::perk);
	WriteValue(stream, 'DIDE', edid);
	WriteValue(stream, 'CSED', "");
	Write(stream, 'ATAD');
	Write<UInt16>(stream, 5);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 1);
	Write<UInt8>(stream, 1);
	Write<UInt8>(stream, 1);
}

void WritePackageValue(std::ostream& stream, bool value) 
{
	WriteValue(stream, 'MANA', "Bool");
	WriteValue<UInt8>(stream, 'MANC', value ? 1 : 0);
}

void WritePackageValue(std::ostream& stream, float value)
{
	WriteValue(stream, 'MANA', "Float");
	WriteValue<UInt8>(stream, 'MANC', value);
}

void WritePackageMarker(std::ostream& stream, UInt32 signature)
{
	WriteNull(stream, signature);
	WriteValue(stream, 'MANI', 0);
	WriteValue(stream, 'OTDP', 0);
	Write(stream, 0);
}

void WritePackage(std::ostream& stream, std::string& edid) {
	WriteHeader(stream, 'KCAP', 0, 0, Companion::sandbox); //TODO len
	WriteValue(stream, 'DIDE', edid);

	Write(stream, 'TDKP');
	Write<UInt16>(stream, 0xC);
	Write(stream, 0);
	Write<UInt16>(stream, 0x12);
	Write<UInt16>(stream, 0x2);
	Write<UInt16>(stream, 0xFF); //flags
	Write<UInt16>(stream, 0);

	Write(stream, 'TDSP');
	Write<UInt16>(stream, 0xC);
	Write(stream, 0xFF00FF00);
	Write(stream, 0xFF);
	Write(stream, 0);

	Write(stream, 'UCKP');
	Write<UInt16>(stream, 0xC);
	Write(stream, 0xF);
	Write(stream, 0x2CB1);
	Write(stream, 7);

	WriteValue(stream, 'MANA', "Location");
	Write(stream, 'TDLP');
	Write<UInt16>(stream, 0x10);
	Write(stream, 3);
	Write(stream, 0);
	Write(stream, 0x400);
	Write(stream, 0);

	WritePackageValue(stream, true);
	WritePackageValue(stream, true);
	WritePackageValue(stream, true);
	WritePackageValue(stream, true);
	WritePackageValue(stream, true);
	WritePackageValue(stream, true);
	WritePackageValue(stream, true);
	WritePackageValue(stream, false);
	WritePackageValue(stream, false);
	WritePackageValue(stream, 50.0f);
	WritePackageValue(stream, false);
	WritePackageValue(stream, false);

	WriteValue(stream, 'MANA', "TargetSelector");
	Write(stream, 'ADTP');
	Write<UInt16>(stream, 0xC);
	Write(stream, 2);
	Write(stream, 0);
	Write(stream, 0);

	WritePackageValue(stream, 300.0f);

	WriteValue<UInt8>(stream, 'MANU', 2);
	WriteValue<UInt8>(stream, 'MANU', 12);
	WriteValue<UInt8>(stream, 'MANU', 5);
	WriteValue<UInt8>(stream, 'MANU', 6);
	WriteValue<UInt8>(stream, 'MANU', 7);
	WriteValue<UInt8>(stream, 'MANU', 8);
	WriteValue<UInt8>(stream, 'MANU', 9);
	WriteValue<UInt8>(stream, 'MANU', 14);
	WriteValue<UInt8>(stream, 'MANU', 10);
	WriteValue<UInt8>(stream, 'MANU', 22);
	WriteValue<UInt8>(stream, 'MANU', 18);
	WriteValue<UInt8>(stream, 'MANU', 20);
	WriteValue<UInt8>(stream, 'MANU', 16);
	WriteValue<UInt8>(stream, 'MANU', 24);
	WriteValue<UInt8>(stream, 'MANU', 26);

	WriteValue<UInt8>(stream, 'MANX', 0x1B);

	WritePackageMarker(stream, 'ABOP');
	WritePackageMarker(stream, 'AEOP');
	WritePackageMarker(stream, 'ACOP');
}

void WriteNpc(std::ostream& stream, Companion& data)
{
	
	WriteValue(stream, 'DIDE', data.name);

	//vm
	Write(stream, 'DAMV');
	Write<UInt16>(stream, 0x347); //TODO len
	Write<UInt16>(stream, 6); //version
	Write<UInt16>(stream, 2); //object format

	//scripts
	Write<UInt16>(stream, 4); //script len

	//companion actor
	WriteScript(stream, "companionactorscript", 0, 0x11);
	WriteScriptObject(stream, "MQComplete", 1, 1, 0xFFFF0000, 0x17D5B2);
	WriteScriptObject(stream, "InfatuationThreshold", 1, 1, 0xFFFF0000, 0x15503F);
	WriteScriptObject(stream, "LovesEvent", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::keyword));
	WriteScriptObject(stream, "InfatuationPerkMessage", 1, 1, 0xFFFF0000, 0x48328);
	WriteScriptObject(stream, "HasItemForPlayer", 1, 1, 0xFFFF0000, 0x16FBC6);
	WriteScriptObject(stream, "DislikesEvent", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::keyword));
	WriteScriptObject(stream, "TemporaryAngerLevel", 1, 1, 0xFFFF0000, 0x2DA12);
	WriteScriptObject(stream, "Experience", 1, 1, 0xFFFF0000, 0x2C9);
	WriteScriptObject(stream, "HomeLocation", 1, 1, 0xFFFF0000, 0x1F228);
	WriteScriptObject(stream, "HatesEvent", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::keyword));
	WriteScriptObject(stream, "StartingThreshold", 1, 1, 0xFFFF0000, 0x4B1C6);
	WriteScriptObject(stream, "Tutorial", 1, 1, 0xFFFF0000, 0x160147);
	WriteScriptObject(stream, "InfatuationPerk", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::perk));
	WriteScriptObject(stream, "CA_Event_Murder", 1, 1, 0xFFFF0000, 0x4D8AB);

	//array of object
	WriteBytes(stream, "KeywordsToAddWhileCurrentCompanion");
	Write<UInt8>(stream, 0xB);
	Write<UInt8>(stream, 1);
	Write<UInt8>(stream, 1);
	Write(stream, 0xFFFF0000);
	Write(stream, 0xAD52A);

	WriteScriptObject(stream, "DismissScene", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::sceneDismiss));
	WriteScriptObject(stream, "LikesEvent", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::keyword));

	//companion power armor
	WriteScript(stream, "CompanionPowerArmorKeywordScript", 0, 3);
	WriteScriptObject(stream, "isPowerArmorFrame", 1, 1, 0xFFFF0000, 0x15503F);
	WriteScriptObject(stream, "pAttachSlot2", 1, 1, 0xFFFF0000, 0xFF18C);
	WriteScriptObject(stream, "pAttachPassenger", 1, 1, 0xFFFF0000, 0x1F9859);
	
	//teleport actor
	WriteScript(stream, "teleportactorscript", 0, 3);
	WriteScriptObject(stream, "TeleportOutSpell", 1, 1, 0xFFFF0000, 0x62BDB);
	WriteScriptProperty(stream, "teleportInOnLoad", 5, 1, false);
	WriteScriptObject(stream, "TeleportInSpell", 1, 1, 0xFFFF0000, 0x62BDC);

	//workshop npc
	WriteScript(stream, "workshopnpcscript", 0, 5);
	WriteScriptProperty(stream, "bAllowCaravan", 5, 1, true);
	WriteScriptProperty(stream, "bAllowMove", 5, 1, true);
	WriteScriptObject(stream, "WorkshopParent", 1, 1, 0xFFFF0000, 0x2058E);
	WriteScriptProperty(stream, "bCommandable", 5, 1, true);
	WriteScriptProperty(stream, "bApplyWorkshopOwnerFaction", 5, 1, false);

	WriteBounds(stream, -22, -14, 0, 22, 14, 128);

	//config
	Write(stream, 'SBCA');
	Write<UInt16>(stream, 0x14);
	Write(stream, 0xB2); //flags
	Write<UInt16>(stream, 0x4B);
	Write<UInt16>(stream, 0x1000);
	Write<UInt16>(stream, 10);
	Write<UInt16>(stream, 0);
	Write<UInt16>(stream, 35);
	Write<UInt16>(stream, 0);
	Write<UInt16>(stream, 0);
	Write<UInt16>(stream, 0x6572);

	//factions
	WritePair<UInt32, UInt8>(stream, 'MANS', 0x23C01, -1);
	WritePair<UInt32, UInt8>(stream, 'MANS', 0x337F3, 0);
	WritePair<UInt32, UInt8>(stream, 'MANS', 0xA1B85, -1);
	WritePair<UInt32, UInt8>(stream, 'MANS', 0x1EC1B9, 0);

	//misc
	WriteValue(stream, 'KCTV', data.voice);
	WriteValue(stream, 'TLPT', data.defaultTemplate);
	WriteValue(stream, 'MANR', data.raceId);
	WriteValue(stream, 'TCPS', 1);
	WriteValue(stream, 'OLPS', 0x4C5053);
	WriteValue(stream, 'MANW', GetFormId(espModId, Companion::armor));
	WriteValue(stream, 'KRTA', data.raceId); //TODO might need to differentiate this

	//perks
	WriteValue(stream, 'ZKRP', 6);
	WritePair<UInt32, UInt8>(stream, 'RKRP', 0x4C935, 1);
	WritePair<UInt32, UInt8>(stream, 'RKRP', 0xA2775, 1);
	WritePair<UInt32, UInt8>(stream, 'RKRP', 0xB9882, 1);
	WritePair<UInt32, UInt8>(stream, 'RKRP', 0xB9883, 1);
	WritePair<UInt32, UInt8>(stream, 'RKRP', 0xB9884, 1);
	WritePair<UInt32, UInt8>(stream, 'RKRP', 0x1D33D7, 1);

	//properties
	Write(stream, 'SPRP');
	Write<UInt16>(stream, 0x58);
	Write(stream, 0x2C2);
	Write(stream, 10.0f);
	Write(stream, 0x2C3);
	Write(stream, 10.0f);
	Write(stream, 0x2C4);
	Write(stream, 10.0f);
	Write(stream, 0x2C5);
	Write(stream, 10.0f);
	Write(stream, 0x2C6);
	Write(stream, 10.0f);
	Write(stream, 0x2C7);
	Write(stream, 10.0f);
	Write(stream, 0x2C8);
	Write(stream, 10.0f);
	Write(stream, 0x2D4);
	Write(stream, 100.0f);
	Write(stream, 0x2D5);
	Write(stream, 50.0f);
	Write(stream, 0x2DA);
	Write(stream, 100.0f);
	Write(stream, 0x2DC);
	Write(stream, 9999.0f);

	//TODO items
	WriteValue(stream, 'TCOC', 0);

	//AI data
	Write(stream, 'TDIA');
	Write<UInt16>(stream, 0x18);
	Write<UInt8>(stream, 1);
	Write<UInt8>(stream, 3);
	Write<UInt8>(stream, 0x32);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 1);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 0);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 0);

	//Packages
	WriteValue(stream, 'DIKP', GetFormId(espModId, Companion::sandbox));
	
	//Keywords
	WriteValue(stream, 'ZISK', 4);
	Write(stream, 'ADWK');
	Write<UInt16>(stream, 0x10);
	Write(stream, 0x1F9859);
	Write(stream, 0x2049E5);
	Write(stream, 0x22E49);
	Write(stream, 0x23F1CA);

	//misc
	WriteValue(stream, 'MANC', data.classId);
	WriteValue(stream, 'LLUF', data.name);
	WriteValue(stream, 'TRHS', data.name);
	WriteNull(stream, 'ATAD');
	Write(stream, 'MAND');
	Write<UInt16>(stream, 0x8);
	Write<UInt16>(stream, 185);
	Write<UInt16>(stream, 50);
	Write<UInt16>(stream, 0);
	Write<UInt16>(stream, 1);

	//TODO head parts
	for (auto& headPart : data.headParts) {
		WriteValue(stream, 'MANP', headPart.formId);
	}

	//Misc
	WriteValue(stream, 'FLCH', data.hairColorId);
	WriteValue(stream, 'MANZ', data.combatStyleId);
	WriteValue<UInt16>(stream, '5MAN', 0xFF00);
	WriteValue(stream, '6MAN', data.heightMin);
	WriteValue(stream, '4MAN', data.heightMax);
	WriteValue(stream, '6MAN', data.heightMin);
	Write(stream, 'TGWM');
	Write<UInt16>(stream, 0xC);
	Write(stream, data.weightThin);
	Write(stream, data.weightMuscular);
	Write(stream, data.weightFat);
	WriteValue(stream, '8MAN', 1);
	WriteValue(stream, 'TFOD', GetFormId(espModId, Companion::outfit));
	WriteValue(stream, 'TSTF', GetFormId(espModId, Companion::textureHead));
	Write(stream, 'MANQ');
	Write<UInt16>(stream, 0x10);
	Write(stream, data.textureLightingRed);
	Write(stream, data.textureLightingGreen);
	Write(stream, data.textureLightingBlue);
	Write(stream, data.textureLightingAlpha);

	//Morphs
	Write(stream, 'KDSM');
	Write<UInt16>(stream, data.morphKeys.size() << 2);
	for (auto& key : data.morphKeys) {
		Write(stream, key);
	}

	Write(stream, 'VDSM');
	Write<UInt16>(stream, data.morphValues.size() << 2);
	for (auto& value : data.morphValues) {
		Write(stream, value);
	}

	//Face tinting layers
	for (auto& layer : data.faceTintingLayers) {
		Write(stream, 'ITET');
		Write<UInt16>(stream, 4);
		Write<UInt16>(stream, layer.type);
		Write<UInt16>(stream, layer.index);
		Write(stream, 'DNET');
		if (layer.type == 1) {
			Write<UInt16>(stream, 7);
			Write<UInt8>(stream, layer.value);
			Write<UInt8>(stream, layer.red);
			Write<UInt8>(stream, layer.green);
			Write<UInt8>(stream, layer.blue);
			Write<UInt8>(stream, layer.alpha);
			Write<UInt16>(stream, layer.colorIndex);
		}
		else {
			Write<UInt16>(stream, 1);
			Write<UInt8>(stream, layer.value);
		}
	}

	//Body morph regions
	Write(stream, 'VSRM');
	Write<UInt16>(stream, 0x14);
	Write(stream, data.bodyMorphRegionHead);
	Write(stream, data.bodyMorphRegionUpperTorso);
	Write(stream, data.bodyMorphRegionArms);
	Write(stream, data.bodyMorphRegionLowerTorso);
	Write(stream, data.bodyMorphRegionLegs);

	//morphs
	for (auto& morph : data.faceMorphs) {
		WriteValue(stream, 'IRMF', morph.index);
		Write(stream, 'SRMF');
		Write<UInt16>(stream, 0x24);
		Write(stream, morph.posX);
		Write(stream, morph.posY);
		Write(stream, morph.posZ);
		Write(stream, morph.rotX);
		Write(stream, morph.rotY);
		Write(stream, morph.rotZ);
		Write(stream, morph.scale);
		Write(stream, morph.unk0);
		Write(stream, morph.unk1);
	}

	WriteValue(stream, 'NIMF', data.faceMorphIntensity);
}

void WriteActor(std::ostream& stream, float scale) {
	WriteHeader(stream, 'RHCA', 0x32, 0x400, Companion::refr);
	WriteFormValue(stream, 'EMAN', espModId, Companion::npc);
	WriteValue(stream, 'LCSX', scale);

	Write(stream, 'ATAD');
	Write<UInt16>(stream, 0x18);
	Write<float>(stream, -81660.070312);
	Write<float>(stream, 88974.070312);
	Write<float>(stream, 7718.489746);
	Write<float>(stream, 0.0f);
	Write<float>(stream, 0.0f);
	Write<float>(stream, 0.0f);
}

UInt32 GetGreetingLength(GreetingData& data) {
	//TODO
	return 0;
}

UInt32 GetInfoLength(std::vector<InfoData>& infoData) {
	//TODO 
	return 0;
}

UInt32 GetSceneLength(std::vector<SceneData>& sceneData, std::vector<InfoData>& infoData, GreetingData& greetingData) {
	//TODO
	return 0;
}

UInt32 GetQuestLength(QuestData& questData) {
	//TODO
	return 0;
}

void WriteQuestStage(std::ostream& stream, UInt16 index) {
	WriteValue(stream, 'XDNI', index);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 116);
	WriteValue<UInt8>(stream, 'TDSQ', 0);
	WriteValue(stream, '2MAN', "");
}

void WriteQuestAlias(std::ostream& stream, UInt32 id, const char* name, UInt32 flags, UInt32 actorId, UInt32 questId, UInt32 alias)
{
	WriteValue(stream, 'TSLA', id);
	WriteValue(stream, 'DILA', name);
	WriteValue(stream, 'MANF', flags);
	
	if (actorId)
		WriteValue(stream, 'AULA', actorId);

	if (questId) {
		WriteValue(stream, 'QELA', questId);
		WriteValue(stream, 'AELA', alias);
	}

	WriteValue(stream, 'KCTV', 0);
	WriteNull(stream, 'DELA');
}

void WriteQuest(std::ostream& stream, QuestData& data) {
	WriteHeader(stream, 'TSUQ', 0, 0, Companion::quest);
	WriteValue(stream, 'DIDE', data.edid);

	//vm
	Write(stream, 'DAMV');
	Write<UInt16>(stream, 0x2D7); //todo len
	Write<UInt16>(stream, 6); //version
	Write<UInt16>(stream, 2); //object format

	//scripts
	Write<UInt16>(stream, 3); //script len

	//affinity scene handler
	WriteScript(stream, "affinityscenehandlerscript", 0, 8);
	WriteScriptObject(stream, "CA_TCustom2_Friend", 1, 1, 0xFFFF0000, 0xF75E1);
	WriteScriptObject(stream, "CA_T3_Neutral", 1, 1, 0xFFFF0000, 0x4B1C6);
	WriteScriptObject(stream, "CA_T5_Hatred", 1, 1, 0xFFFF0000, 0x4B1C8);
	WriteScriptObject(stream, "CompanionAlias", 1, 1, 0, GetFormId(espModId, Companion::quest));
	WriteScriptObject(stream, "CA_T2_Admiration", 1, 1, 0xFFFF0000, 0x4B1C5);
	WriteScriptObject(stream, "CA_T1_Infatuation", 1, 1, 0xFFFF0000, 0x4B1C4);
	WriteScriptObject(stream, "CA_T4_Disdain", 1, 1, 0xFFFF0000, 0x4B1C7);
	WriteScriptObject(stream, "CA_TCustom1_Confidant", 1, 1, 0xFFFF0000, 0xF75E2);

	//comtalkquestscript
	WriteScript(stream, "COMTalkQuestScript", 0, 6);
	WriteScriptObject(stream, "CA_T3_Neutral", 1, 1, 0xFFFF0000, 0x4B1C6);
	WriteScriptObject(stream, "CA_T5_Hatred", 1, 1, 0xFFFF0000, 0x4B1C8);
	WriteScriptObject(stream, "CompanionActor", 1, 1, 0xFFFF0000, GetFormId(espModId, Companion::refr));
	WriteScriptObject(stream, "CA_T2_Admiration", 1, 1, 0xFFFF0000, 0x4B1C5);
	WriteScriptObject(stream, "CA_T1_Infatuation", 1, 1, 0xFFFF0000, 0x4B1C4);
	WriteScriptObject(stream, "CA_T4_Disdain", 1, 1, 0xFFFF0000, 0x4B1C7);
		
	//<companion>questscript
	WriteScript(stream, data.questScript.c_str(), 0, 3);
	WriteScriptProperty(stream, "Status", 3, 1, (UInt32)0);
	WriteScriptObject(stream, "CurrentcompanionFaction", 1, 1, 0xFFFF0000, 0x23C01);
	WriteScriptObject(stream, data.companionName, 1, 1, 0, GetFormId(espModId, Companion::quest));

	//fragments
	Write<UInt8>(stream, 3); //unk
	Write<UInt16>(stream, 2); //fragment len
	WriteBytes(stream, data.fragmentScript);
	Write<UInt8>(stream, 0); //flags
	Write<UInt16>(stream, 1); //prop len
	WriteScriptObject(stream, data.companionName, 1, 1, 0, GetFormId(espModId, Companion::quest));
	WriteScriptFragment(stream, 80, 0, 0, 1, data.fragmentScript.c_str(), "Fragment_Stage_0080_Item_00");
	WriteScriptFragment(stream, 90, 0, 0, 1, data.fragmentScript.c_str(), "Fragment_Stage_0090_Item_00");

	Write<UInt16>(stream, 0); //aliases
	
	WriteValue(stream, 'LLUF', data.fullName);
	
	Write(stream, 'MAND');
	Write(stream, 0x11D000C);
	Write<UInt8>(stream, 0x70);
	Write<UInt8>(stream, 0);
	Write(stream, 0);
	Write(stream, 0);

	WriteValue(stream, 'RTLF', "Followers\\Companions\\COM\\");
	ConditionData conditionData = { 'ADTC', 0, 1.0f, 0x236, 0, 0, 0, 0, -1 };
	WriteCondition(stream, conditionData);
	WriteNull(stream, 'TXEN');
	WriteQuestStage(stream, 80);
	WriteQuestStage(stream, 90);
	WriteValue(stream, 'MANA', 2); //next alias id
	WriteQuestAlias(stream, 0, data.companionName, 0, GetFormId(espModId, Companion::npc), 0, 0);
	WriteQuestAlias(stream, 1, "Companion", 0, 0, GetFormId(espModId, Companion::quest), 0);
}

void WriteScene(std::ostream& stream, SceneData& data) {
	WriteHeader(stream, 'NECS', 0, 0, data.formId);

	Write(stream, 'DIDE');
	Write(stream, data.edid);

	WriteValue(stream, 'MANF', 0x24);

	//phase
	WriteNull(stream, 'MANH');
	Write(stream, '0MAN');
	Write(stream, data.phase);
	WriteNull(stream, 'TXEN');
	WriteNull(stream, 'TXEN');
	WriteValue(stream, 'MANW', 350);
	WriteNull(stream, 'MANH');

	//actors
	WriteValue(stream, 'DILA', 0);
	WriteValue(stream, 'MANL', 4); //run only scene packages
	WriteValue(stream, 'MAND', 0xA); //death/combat end

	//action 0
	WriteValue<UInt16>(stream, 'MANA', 3);
	WriteValue<UInt8>(stream, '0MAN', 0);
	WriteValue(stream, 'DILA', 0);
	WriteValue(stream, 'MANI', 1);
	WriteValue(stream, 'MANF', 0x228000); //face target, headtrack player, camera speaker target
	WriteValue(stream, 'MANS', 0);
	WriteValue(stream, 'MANE', 0);
	WriteFormValue(stream, 'POTP', espModId, data.pPositive);
	WriteFormValue(stream, 'POTN', espModId, data.pNegative);
	WriteFormValue(stream, 'OTEN', espModId, data.pNeutral);
	WriteFormValue(stream, 'POTQ', espModId, data.pQuestion);
	WriteFormValue(stream, 'TOPN', espModId, data.pPositive);
	WriteFormValue(stream, 'TGNN', espModId, data.pNegative);
	WriteFormValue(stream, 'TUNN', espModId, data.pNeutral);
	WriteFormValue(stream, 'TUQN', espModId, data.pQuestion);
	WriteValue(stream, 'TGTD', 0);
	WriteNull(stream, 'MANA');

	//action 1
	if (data.action1) {
		WriteValue<UInt16>(stream, 'MANA', 1);
		WriteValue<UInt8>(stream, '0MAN', 0);
		WriteValue(stream, 'DILA', 0);
		WriteValue(stream, 'MANI', 2);
		WriteValue(stream, 'MANS', 0);
		WriteValue(stream, 'MANE', 0);
		WriteValue(stream, 'MANP', 0x19AA2); //defaultStayAtSelfScene
		WriteNull(stream, 'MANA');
	}

	WriteFormValue(stream, 'MANP', espModId, Companion::quest);
	WriteValue(stream, 'MANI', data.action1 ? 2 : 1);

	Write(stream, 'MANV');
	Write<UInt16>(stream, 0x10);
	Write(stream, 3);
	Write(stream, 3);
	Write(stream, 3);
	Write(stream, 3);

	WriteValue(stream, 'MANX', data.index);
}

void InitInfoData(InfoData& data, UInt32 topicId, UInt32 infoId) {
	memset(&data, 0, sizeof(InfoData));
	data.topicId = topicId;
	data.infoId = infoId;
}

void WriteBaseTopic(std::ostream& stream, UInt32 formId) {
	WriteHeader(stream, 'LAID', 0, 0, formId);

	WriteValue(stream, 'MANP', 50.0f);
	WriteFormValue(stream, 'MANQ', espModId, Companion::quest);

	Write(stream, 'ATAD');
	Write<UInt16>(stream, 4);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 2);
	Write<UInt16>(stream, 0x11);
	WriteValue(stream, 'MANS', 'NECS');
	WriteValue(stream, 'CFIT', 1);
}

void WriteInfo(std::ostream& stream, InfoData& data) {
	WriteHeader(stream, 'OFNI', 0, 0, data.infoId); //TODO len

	if (data.openInventory) {
		Write(stream, 'DAMV');
		Write<UInt16>(stream, 0x22);
		Write<UInt16>(stream, 0x6);
		Write<UInt16>(stream, 0x2);
		Write<UInt16>(stream, 0x1);
		Write(stream, "OpenInventoryInfoScript");
		Write<UInt8>(stream, 0);
		Write<UInt8>(stream, 0);
	}

	WriteValue(stream, 'MANE', data.responseFlags);
	WriteValue(stream, 'MANP', data.previousInfo);

	if (data.sharedInfo)
		WriteValue(stream, 'MAND', data.sharedInfo);

	//responses
	if (data.response) {
		Write(stream, 'ADRT');
		Write<UInt16>(stream, 0x14);
		Write(stream, -1); //emotion
		Write(stream, 1);
		Write(stream, 0);
		Write(stream, -1);
		Write(stream, -1);

		WriteValue(stream, '1MAN', data.response);
		WriteValue(stream, '2MAN', "");
		WriteValue(stream, '3MAN', "");
		WriteValue(stream, '4MAN', "");
		//WriteValue<UInt64>(stream, '9MAN', 0x0000000000000000); //TODO find out if this is necessary
	}

	if (data.condition1.signature)
		WriteCondition(stream, data.condition1);

	if (data.condition2.signature)
		WriteCondition(stream, data.condition2);

	if (data.startScene)
		WriteValue(stream, 'ECST', data.startScene);

	if (data.setParentQuestStage) {
		Write(stream, 'SQIT');
		Write<UInt16>(stream, -1);
		Write<UInt16>(stream, data.setParentQuestStage);
	}

	WriteValue(stream, '0MAN', data.phase ? data.phase : "");

	WriteValue(stream, 'MANI', 1);
}

void WriteTopicInfo(std::ostream& stream, InfoData& data) {
	WriteBaseTopic(stream, data.topicId);
	WriteGroup(stream, 0, GetFormId(espModId, data.topicId), 7, espGroupId); //TODO len
	WriteInfo(stream, data);
}

void WriteGreeting(std::ostream& stream, GreetingData& data) {
	WriteHeader(stream, 'LAID', 0, 0, Companion::dialGreeting);

	WriteValue(stream, 'DIDE', data.edid);
	WriteValue(stream, 'MANP', 50.0f);
	WriteFormValue(stream, 'MANQ', espModId, Companion::quest);

	Write(stream, 'ATAD');
	Write<UInt16>(stream, 4);
	Write<UInt8>(stream, 0);
	Write<UInt8>(stream, 7);
	Write<UInt16>(stream, 0x76);
	WriteValue(stream, 'MANS', 'EERG');
	WriteValue(stream, 'CFIT', 3);

	WriteGroup(stream, 0, GetFormId(espModId, Companion::dialGreeting), 7, espGroupId); //TODO len

	//Hire
	InfoData infoData;
	InitInfoData(infoData, 0, Companion::infoGreetingPickup);
	infoData.responseFlags = 8;
	infoData.response = data.hireResponse;
	infoData.condition1 = { 'ADTC', 0x2DA87800, 0.0f, 0x2DA8003A, GetFormId(espModId, Companion::quest), 0, 0, 0, -1 };
	infoData.condition2 = { 'ADTC', 0x2DA87800, 1.0f, 0x2DA80048, GetFormId(espModId, Companion::npc), 0, 0, 0, -1 };
	infoData.startScene = GetFormId(espModId, Companion::scenePickup);
	infoData.phase = data.hirePhase;
	WriteInfo(stream, infoData);

	//Rehire
	InitInfoData(infoData, 0, Companion::infoGreetingRehire);
	infoData.responseFlags = 8;
	infoData.previousInfo = GetFormId(espModId, Companion::infoGreetingTalk);
	infoData.response = data.rehireResponse;
	infoData.condition1 = { 'ADTC', 0x2DA87800, 1.0f, 0x2DA80048, GetFormId(espModId, Companion::npc), 0, 0, 0, -1 };
	infoData.condition2 = { 'ADTC', 0x2DA8B400, 0.0f, 0x2DA80047, 0x23C01, 0, 0, 0, -1 };
	infoData.startScene = GetFormId(espModId, Companion::sceneRehire);
	infoData.phase = data.rehirePhase;
	WriteInfo(stream, infoData);

	//Talk
	InitInfoData(infoData, 0, Companion::infoGreetingTalk);
	infoData.responseFlags = 8;
	infoData.previousInfo = GetFormId(espModId, Companion::infoGreetingPickup);
	infoData.response = data.talkResponse;
	infoData.condition1 = { 'ADTC', 0x2DA87800, 1.0f, 0x2DA80047, 0x23C01, 0, 0, 0, -1 };
	infoData.condition2 = { 'ADTC', 0x2DA8B100, 1.0f, 0x2DA80048, GetFormId(espModId, Companion::npc), 0, 0, 0, -1 };
	infoData.startScene = GetFormId(espModId, Companion::sceneTalk);
	infoData.phase = data.talkPhase;
	WriteInfo(stream, infoData);
}

bool WriteEsp(const char* path, Companion& data) {
	SAF::OutStreamWrapper wrapper(path);
	if (wrapper.fail)
		return false;

	auto& stream = wrapper.stream;

	//tes4
	Write(stream, '4SET');
	Write<UInt32>(stream, 108 + data.author.length());
	Write(stream, 0x200); //esl TODO make dynamic adjustment from esl->esp when necessary
	Write(stream, 0);
	Write(stream, 0);
	Write(stream, 131); //version

	//header
	Write(stream, 'RDEH');
	Write<UInt16>(stream, 0xC);
	Write(stream, 1.0f);

	//TODO might need to make dynamic
	Write(stream, Companion::max - 0x800); //numrecords
	Write(stream, Companion::max); //nextobjectid

	Write(stream, 'MANC');
	Write(stream, data.author);

	Write(stream, 'TSAM');
	Write(stream, "Fallout4.esm");

	Write(stream, 'ATAD');
	Write<UInt16>(stream, 8);
	Write(stream, 0);
	Write(stream, 0);

	//transient type 0
	Write(stream, 'MANT');
	Write<UInt16>(stream, 8);
	Write(stream, 78); //type
	Write(stream, Companion::dialGreeting);

	//transient type 1
	Write(stream, 'MANT');
	Write<UInt16>(stream, 0x14);
	Write(stream, 126); //type
	Write(stream, Companion::sceneRehire);
	Write(stream, Companion::sceneTalk); 
	Write(stream, Companion::sceneDismiss);
	Write(stream, Companion::sceneRehire); 

	Write(stream, 'VTNI');
	Write<UInt16>(stream, 4);
	Write(stream, 1);

	//Keyword
	std::string keyword = "Keyword" + data.name;
	WriteGroup(stream, 0x4B + keyword.length(), 'KYWD', 0, 0);
	WriteKeyword(stream, keyword);

	//Texture Set
	UInt32 textureSetLength = 0x18;
	for (auto& textureSet : data.textureSets) {
		textureSetLength += 0x18; //header
		textureSetLength += textureSet.length;
	}

	WriteGroup(stream, textureSetLength, 'TSXT', 0, 0);
	for (auto& textureSet : data.textureSets) {
		WriteTextureSet(stream, textureSet);
	}

	//Armor Addon
	std::string aaBody = "AANakedBody" + data.name;
	std::string aaHands = "AANakedHands" + data.name;

	WriteGroup(stream, 0, 'AMRA', 0, 0); //TODO len
	WriteArmorAddon(stream, aaBody, aaBodyData, Companion::aaTorso, Companion::textureBody, data.raceId);
	WriteArmorAddon(stream, aaHands, aaHandsData, Companion::aaHands, Companion::textureHands, data.raceId);

	//Armor
	std::string nakedArmor = "Naked" + data.name;
	WriteGroup(stream, 0, 'OMRA', 0, 0); //TODO len
	WriteArmor(stream, nakedArmor, data.raceId, Companion::armor, Companion::aaTorso, Companion::aaHands);

	//Outfit
	std::string nakedOutfit = "NakedOutfit" + data.name;
	WriteGroup(stream, 0, 'TFTO', 0, 0); //TODO len
	WriteOutfit(stream, nakedOutfit, Companion::outfit, Companion::armor);

	//Head Part
	WriteGroup(stream, 0, 'TPDH', 0, 0); //TODO len
	for (auto& headPart : data.headParts) {
		WriteHeadPart(stream, headPart);
	}

	//Might need to reserve multiple parts for each head part eg.hair
	
	//Head
	//HeadRear
	//Mouth
	//MouthShadow
	//Eyes
	//EyesLashes
	//EyesAO
	//EyesWet
	//Hair
	//Beards

	//Perk
	std::string perkName = "Perk" + data.name;
	WriteGroup(stream, 0, 'KREP', 0, 0); //TODO len
	WritePerk(stream, perkName);

	//Package
	std::string packageName = "Sandbox" + data.name;
	WriteGroup(stream, 0, 'KCAP', 0, 0); //TODO len
	WritePackage(stream, packageName);

	//NPC
	WriteGroup(stream, 0, '_CPN', 0, 0); //TODO len
	WriteHeader(stream, '_CPN', 0, 0, Companion::npc); //TODO len
	WriteNpc(stream, data); //TODO deflate maybe

	//Worldspace
	WriteGroup(stream, 0, 'DLRW', 0, 0); //TODO len
	std::string worldspacePath = GetPathWithExtension(RESOURCES_PATH, "CommonwealthWorldspace", ".bin");
	if (!WriteResource(stream, worldspacePath.c_str()))
		return false;

	//Cell
	const char* cellData = "\x47\x52\x55\x50\xE1\0\0\0\x3C\0\0\0\x1\0\0\0\xFF\x2C\0\0\0\0\0\0\x43\x45\x4C\x4C\x37\0\0\0\0\x4\x4\0\xA2\x8A\x1\0\xF9\x2C\0\0\x83\0\0\0\x38\0\0\0\x78\xDA\x73\x71\xC\x71\x64\x62\x60\x62\x88\x70\xF6\x71\xE6\x61\x80\x03\x8B\x13\x75\x3E\x21\xBE\x1\x2C\x60\xE\x50\x2E\x9C\x85\xE1\xFF\xFF\xFA\x7A\x20\x2B\x88\x85\xE1\xC1\x6C\x16\x6\0\x15\xED\xC\xE";
	stream.write(cellData, constStrLen(cellData));

	//Persistent
	WriteGroup(stream, 0, 0x18AA2, 6, espGroupId); //TODO len 1b4

	//Actor
	WriteGroup(stream, 0x62, 0x18AA2, 0, espGroupId); //TODO 1b4
	WriteActor(stream, data.scale);

	//Gather all quest and sub group data
	QuestData questData{
		"Quest" + data.name,
		"Companion Dialogue " + data.name,
		"Fragments:Quests:QFQuest" + data.name,
		data.name.c_str(),
	};

	std::vector<SceneData> sceneData;
	std::string pickupPhase = "PhasePickup" + data.name;
	std::string dismissPhase = "PhaseDismiss" + data.name;
	std::string talkPhase = "PhaseTalk" + data.name;
	std::string rehirePhase = "PhaseRehire" + data.name;

	//Pickup
	sceneData.push_back({
		"ScenePickup" + data.name,
		pickupPhase,

		10,
		true,

		Companion::scenePickup,
		Companion::dialPickupPlayerPositive,
		Companion::dialPickupPlayerNegative,
		Companion::dialPickupPlayerTalk,
		Companion::dialPickupPlayerQuestion,
		Companion::dialPickupNpcPositive,
		Companion::dialPickupNpcNegative,
		Companion::dialPickupNpcTalk,
		Companion::dialPickupNpcQuestion
	});

	//Dismiss
	sceneData.push_back({
		"SceneDismiss" + data.name,
		dismissPhase,

		20,
		false,

		Companion::sceneDismiss,
		Companion::dialDismissPlayerPositive,
		Companion::dialDismissPlayerNegative,
		Companion::dialDismissPlayerTalk,
		Companion::dialDismissPlayerQuestion,
		Companion::dialDismissNpcPositive,
		Companion::dialDismissNpcNegative,
		Companion::dialDismissNpcTalk,
		Companion::dialDismissNpcQuestion
	});

	//Talk
	sceneData.push_back({
		"SceneTalk" + data.name,
		talkPhase,

		30,
		true,

		Companion::sceneTalk,
		Companion::dialTalkPlayerPositive,
		Companion::dialTalkPlayerNegative,
		Companion::dialTalkPlayerTalk,
		Companion::dialTalkPlayerQuestion,
		Companion::dialTalkNpcPositive,
		Companion::dialTalkNpcNegative,
		Companion::dialTalkNpcTalk,
		Companion::dialTalkNpcQuestion
	});

	//Rehire
	sceneData.push_back({
		"SceneRehire" + data.name,
		rehirePhase,

		40,
		true,

		Companion::sceneRehire,
		Companion::dialRehirePlayerPositive,
		Companion::dialRehirePlayerNegative,
		Companion::dialRehirePlayerTalk,
		Companion::dialRehirePlayerQuestion,
		Companion::dialRehireNpcPositive,
		Companion::dialRehireNpcNegative,
		Companion::dialRehireNpcTalk,
		Companion::dialRehireNpcQuestion
	});

	//Topic and info
	std::vector<InfoData> infoDatas;
	InfoData infoData;

	//Pickup player
	InitInfoData(infoData, Companion::dialPickupPlayerPositive, Companion::infoPickupPlayerPositive);
	infoData.response = "Come with me";
	infoData.prompt = "Come with me";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialPickupPlayerNegative, Companion::infoPickupPlayerNegative);
	infoData.sharedInfo = 0x644E6; //shared nevermind
	infoData.prompt = "Nevermind";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialPickupPlayerTalk, Companion::infoPickupPlayerTalk);
	infoData.sharedInfo = 0x10F0F5;
	infoData.prompt = "Trade";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialPickupPlayerQuestion, Companion::infoPickupPlayerQuestion);
	infoData.sharedInfo = 0x1AADEA; //follower thoughts 01
	infoData.prompt = "Thoughts?";
	infoDatas.push_back(infoData);

	//Pickup npc
	InitInfoData(infoData, Companion::dialPickupNpcPositive, Companion::infoPickupNpcPositive);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::comeWithMe].c_str();
	infoData.setParentQuestStage = 80;
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialPickupNpcNegative, Companion::infoPickupNpcNegative);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::nevermind].c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialPickupNpcTalk, Companion::infoPickupNpcTalk);
	infoData.openInventory = true;
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::trade].c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialPickupNpcQuestion, Companion::infoPickupNpcQuestion);
	infoData.response = data.responses[Companion::thoughts].c_str();
	infoDatas.push_back(infoData);

	//Dismiss player
	InitInfoData(infoData, Companion::dialDismissPlayerPositive, Companion::infoDismissPlayerPositive);
	infoData.response = "Goodbye";
	infoData.prompt = "Bye";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialDismissPlayerNegative, Companion::infoDismissPlayerNegative);
	infoData.responseFlags = 0x40;
	infoData.sharedInfo = 0x644E6; //nevermind
	infoData.prompt = "Forget it";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialDismissPlayerTalk, Companion::infoDismissPlayerTalk);
	infoData.response = "???";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialDismissPlayerQuestion, Companion::infoDismissPlayerQuestion);
	infoData.response = ";)";
	infoDatas.push_back(infoData);

	//Dismiss npc
	InitInfoData(infoData, Companion::dialDismissNpcPositive, Companion::infoDismissNpcPositive);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::dismissed].c_str();
	infoData.setParentQuestStage = 90;
	infoDatas.push_back(infoData);

	InitInfoData(infoData,Companion::dialDismissNpcNegative, Companion::infoDismissNpcNegative);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::nevermind].c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialDismissNpcTalk, Companion::infoDismissNpcTalk);
	infoData.response = "???";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialDismissNpcQuestion, Companion::infoDismissNpcQuestion);
	infoData.response = ";)";
	infoDatas.push_back(infoData);

	//Talk player
	InitInfoData(infoData, Companion::dialTalkPlayerPositive, Companion::infoTalkPlayerPositive);
	infoData.sharedInfo = 0x19D344;
	infoData.prompt = "Dismiss";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialTalkPlayerNegative, Companion::infoTalkPlayerNegative);
	infoData.sharedInfo = 0x19D33C;
	infoData.prompt = "Nevermind";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialTalkPlayerTalk, Companion::infoTalkPlayerTalk);
	infoData.sharedInfo = 0x10F0F5;
	infoData.prompt = "Trade";
	infoData.phase = dismissPhase.c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialTalkPlayerQuestion, Companion::infoTalkPlayerQuestion);
	infoData.sharedInfo = 0x1AADEA;
	infoData.prompt = "Thoughts?";
	infoDatas.push_back(infoData);

	//Talk npc
	InitInfoData(infoData, Companion::dialTalkNpcPositive, Companion::infoTalkNpcPositive);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::dismissed].c_str();
	infoData.setParentQuestStage = 90;
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialTalkNpcNegative, Companion::infoTalkNpcNegative);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::nevermind].c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialTalkNpcTalk, Companion::infoTalkNpcTalk);
	infoData.openInventory = true;
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::trade].c_str();
	WriteTopicInfo(stream, infoData);

	InitInfoData(infoData, Companion::dialTalkNpcQuestion, Companion::infoTalkNpcQuestion);
	infoData.response = data.responses[Companion::thoughts].c_str();
	infoDatas.push_back(infoData);

	//Rehire player
	InitInfoData(infoData, Companion::dialRehirePlayerPositive, Companion::infoRehirePlayerPositive);
	infoData.response = "Come with me";
	infoData.prompt = "Travel Together";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialRehirePlayerNegative, Companion::infoRehirePlayerNegative);
	infoData.sharedInfo = 0x19D33C;
	infoData.prompt = "Nevermind";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialRehirePlayerTalk, Companion::infoRehirePlayerTalk);
	infoData.sharedInfo = 0x10F0F5;
	infoData.prompt = "Trade";
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialRehirePlayerQuestion, Companion::infoRehirePlayerQuestion);
	infoData.sharedInfo = 0x1AADEA;
	infoData.prompt = "Thoughts?";
	infoDatas.push_back(infoData);

	//Rehire npc
	InitInfoData(infoData, Companion::dialRehireNpcPositive, Companion::infoRehireNpcPositive);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::comeWithMe].c_str();
	infoData.setParentQuestStage = 80;
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialRehireNpcNegative, Companion::infoRehireNpcNegative);
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::nevermind].c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialRehireNpcTalk, Companion::infoRehireNpcTalk);
	infoData.openInventory = true;
	infoData.responseFlags = 0x40;
	infoData.response = data.responses[Companion::trade].c_str();
	infoDatas.push_back(infoData);

	InitInfoData(infoData, Companion::dialRehireNpcQuestion, Companion::infoRehireNpcQuestion);
	infoData.response = data.responses[Companion::thoughts].c_str();
	infoDatas.push_back(infoData);

	GreetingData greetingData{
		"Greeting" + data.name,

		pickupPhase.c_str(),
		rehirePhase.c_str(),
		talkPhase.c_str(),

		data.responses[Companion::greetingUnhired].c_str(),
		data.responses[Companion::greetingRehire].c_str(),
		data.responses[Companion::greetingHired].c_str(),
	};

	//Quest
	UInt32 sceneLength = GetSceneLength(sceneData, infoDatas, greetingData);

	WriteGroup(stream, GetQuestLength(questData) + sceneLength, 'TSUQ', 0, 0); //TODO len
	WriteQuest(stream, questData);

	//Scene
	WriteGroup(stream, sceneLength, GetFormId(espModId, Companion::quest), 0xA, espGroupId); //TODO len
	for (auto& scene : sceneData) {
		WriteScene(stream, scene);
	}

	//Info
	for (auto& info : infoDatas) {
		WriteTopicInfo(stream, info);
	}

	//Greeting
	WriteGreeting(stream, greetingData);

	return true;
}

bool WriteQuestFragment(const char* path, std::string& name) 
{
	SAF::OutStreamWrapper wrapper(path);
	if (wrapper.fail)
		return false;

	auto& stream = wrapper.stream;

	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);

	//header
	Write(stream, 0xFA57C0DE);
	Write(stream, 0x20903);
	Write(stream, time);
	Write(stream, 0x63EADE7E);
	Write(stream, 0);
	WriteBytes(stream, "C:\\Screen\\Archer\\Menu\\Fragments\\Quests\\QFQuest.psc");
	WriteBytes(stream, "Sam");
	WriteBytes(stream, "SAM-COMPUTER");

	//string table
	Write<UInt16>(stream, 0x1A);
	WriteBytes(stream, "Fragments:Quests:QF" + name + "Quest");
	WriteBytes(stream, "Fragment_Stage_0080_Item_00");
	WriteBytes(stream, "Fragment_Stage_0090_Item_00");
	WriteBytes(stream, "default");
	WriteBytes(stream, "mandatory");
	WriteBytes(stream, "conditional");
	WriteBytes(stream, "collapsedonbase");
	WriteBytes(stream, "collapsedonref");
	WriteBytes(stream, "hidden");
	WriteBytes(stream, "Quest");
	WriteBytes(stream, "::Alias_" + name + "_var");
	WriteBytes(stream, "referencealias");
	WriteBytes(stream, "Alias_" + name);
	WriteBytes(stream, "None");
	WriteBytes(stream, "::temp0");
	WriteBytes(stream, "followersscript");
	WriteBytes(stream, "::temp1");
	WriteBytes(stream, "actor");
	WriteBytes(stream, "::nonevar");
	WriteBytes(stream, "GetScript");
	WriteBytes(stream, "::GetActorRef");
	WriteBytes(stream, "SetCompanion");
	WriteBytes(stream, "::temp2");
	WriteBytes(stream, "::temp3");
	WriteBytes(stream, "DismissCompanion");
	
	Write<UInt8>(stream, 1);
	Write(stream, time);

	//code
	const char* code = "\x2\0\0\0\x1\0\x2\0\0\x3\0\x7\0\x7\0\x7\0\0\0\x1\0\x3\0\0\x3\0\xf\0\xf\0\xf\0\x1\0\0\0\x1\0\x1\0\0\0\0\0\x1\0\xd\0\0\0\x6\0\x4\0\x2\x5\0\x5\x6\0\x1\x7\0\x4\x8\0\x3\x9\0\0\x1\0\0\0\xd6\0\0\0\xa\0\x1\0\x1\x1\0\0\0\x1\0\0\0\x1\0\xb\0\xc\0\0\0\0\0\0\x1\x1\0\xd\0\xc\0\x1\0\x20\0\0\0\x7\xb\0\x1\0\x1\0\x2\0\x2\0\xe\0\x1\0\0\0\0\0\0\0\0\x3\0\xf\0\x10\0\x11\0\x12\0\x13\0\xe\0\x3\0\x19\x1\x10\0\x1\x14\0\x1\xf\0\x3\0\0\0\0\x17\x1\x15\0\x1\xb\0\x1\x11\0\x3\0\0\0\0\x17\x1\x16\0\x1\xf\0\x1\x13\0\x3\x4\0\0\0\x1\x11\0\x5\x1\x5\x1\x5\0\x3\0\xe\0\x1\0\0\0\0\0\0\0\0\x3\0\x17\0\x10\0\x18\0\x12\0\x13\0\xe\0\x3\0\x19\x1\x10\0\x1\x14\0\x1\x17\0\x3\0\0\0\0\x17\x1\x15\0\x1\xb\0\x1\x18\0\x3\0\0\0\0\x17\x1\x19\0\x1\x17\0\x1\x13\0\x3\x3\0\0\0\x1\x18\0\x5\x1\x5\0";
	stream.write(code, constStrLen(code));

	return true;
}

//bool WriteTopicInfoFragment(const char* path, std::string& name)
//{
//	SAF::OutStreamWrapper wrapper(path);
//	if (wrapper.fail)
//		return false;
//
//	//TODO
//
//	return true;
//}

bool WriteScriptFragments(std::string& folder, std::string& name) {
	std::string questFragment = folder + "\\Data\\Scripts\\Fragments\\Quests\\QF" + name + "Quest.pex";
	
	if (!WriteQuestFragment(questFragment.c_str(), name))
		return false;

	//std::string topicInfoFragment = folder + "\\Data\\Scripts\\Fragments\\Quests\\TIF" + name + "Quest.pex";

	//if (!WriteTopicInfoFragment(topicInfoFragment.c_str(), name))
	//	return false;

	return true;
}

//TODO figure out what menu options we want for manual configuration
void ExportCompanion(GFxResult& result) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	Actor* actor = (Actor*)selected.refr;

	const char* name = selected.refr->baseForm->GetFullName();
	if (!name || !*name) {
		//Need to force a name
		if (selected.isFemale)
			name = "Nora";
		else
			name = "Nate";
	}

	companionData.name = name;
	companionData.author = "Screen Archer Menu";
	
	//TODO esp mod id depends on the number of masters
	companionData.raceId = (espModId << 24) |
		(((actor->race->formID & 0xFE000000) == 0xFE000000) ? (actor->race->formID & 0xFFF) : (actor->race->formID & 0xFFFFFF));

	companionData.scale = ((UInt16)actor->unk104) * 0.01;

	//TODO texture set can either be a fo4 material or skyrim texture set, figure out how to determine which one it is and push to this vector
	companionData.textureSets.push_back(TextureSet("TextureBody" + companionData.name, Companion::textureBody));
	companionData.textureSets.push_back(TextureSet("TextureHands" + companionData.name, Companion::textureHands));
	companionData.textureSets.push_back(TextureSet("TextureHead" + companionData.name, Companion::textureHead));
	companionData.textureSets.push_back(TextureSet("TextureHeadRear" + companionData.name, Companion::textureHeadRear));
	companionData.textureSets.push_back(TextureSet("TextureEyes" + companionData.name, Companion::textureEyes));

	std::filesystem::path folder(COMPANION_PATH);
	folder.append(companionData.name);

	std::stringstream espName;
	espName << folder;
	espName << "\\Companion";
	espName << companionData.name;
	espName << ".esp";

	if (!WriteEsp(espName.str().c_str(), companionData))
		return result.SetError("Failed to write esp file");

	if (!WriteScriptFragments(folder.string(), companionData.name))
		return result.SetError("Failed to write script fragments");
}

const char* responseNames[] = {
	"Unhired Greeting",
	"Hired Greeting",
	"Rehire Greeting",
	"Come with me",
	"Nevermind",
	"Trade",
	"Thoughts",
	"Dimissed"
};

void GetCompanionResponses(GFxResult& result) {
	result.CreateMenuItems();

	for (SInt32 i = 0; i < Companion::responseMax; ++i) {
		result.PushItem(responseNames[i], i);
	}
}

void SetCompanionResponse(GFxResult& result, const char* response, SInt32 index)
{
	if (index < 0 || index >= Companion::responseMax)
		return result.SetError("Response index out of range");

	companionData.responses[index] = response;
}