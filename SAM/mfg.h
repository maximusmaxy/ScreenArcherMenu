#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

class BSFaceGenAnimationData {
public:
	UInt64 vfTable;
	UInt32 size;
	UInt32 unk1;
	UInt64 unk2;
	float faceAnimMorphs[54];
	float mfgMorphs[54];
};

bool GetMorphArray(SInt32* morphs);
void SetFaceMorph(UInt32 id, UInt32 scale);
void SaveMfg(std::string filename);
bool LoadMfg(std::string filename, SInt32* morphs);
void ResetMfg();
bool GetGFxMorphArray(GFxMovieRoot* root, GFxValue* morphArray);