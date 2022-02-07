#pragma once

#include "f4se/NiObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/NiMaterials.h"

#include "mfg.h"

struct EyeUpdateData
{
	float unk1[30];
	UInt16 flags[30];
	UInt8 pad1[12];
	float unk3[30];
	UInt32 unk4;
	UInt32 unk5;
	NiAVObject* lookTargets[30];
	NiAVObject* eyeObjects[30];
	NiAVObject* headObjects[30];
	NiPoint2 points[30];
	NiPoint3 bounds[30];
	UInt8 pad2[8];
	BSFaceGenAnimationData* faceAnimData[30];
	UInt64 unk12[30];
};

NiAVObject* GetEyeNode(TESObjectREFR* refr);
bool GetEyecoords(float* coords);
void SetEyecoords(float x, float y);
BGSHeadPart* GetHeadPartByID(UInt32 id);
BGSHeadPart* ChangeEyeMesh(TESObjectREFR* refr, BGSHeadPart* eyePart);