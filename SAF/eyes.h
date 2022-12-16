#pragma once

#include "f4se/NiObjects.h"
#include "f4se/GameReferences.h"
#include "f4se/NiMaterials.h"

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
	//BSFaceGenAnimationData* faceAnimData[30];
	UInt64 faceAnimData;
	UInt64 unk12[30];
};

namespace SAF {
	NiAVObject* GetEyeNode(TESObjectREFR* refr);
	bool GetEyecoords(TESObjectREFR* refr, float* coords);
	bool GetEyecoords(NiAVObject* eyeNode, float* ret);
	void SetEyecoords(TESObjectREFR* refr, float x, float y);
	void SetEyecoords(NiAVObject* eyeNode, float x, float y);
}