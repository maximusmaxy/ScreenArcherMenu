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
	//incomplete
};

void SetFaceMorph(UInt32 categoryIndex, UInt32 morphIndex, UInt32 scale);
void SaveMfg(std::string filename);
bool LoadMfg(std::string filename);
void ResetMfg();
void GetMorphCategoriesGFx(GFxMovieRoot* root, GFxValue* morphArray);
void GetMorphsGFx(GFxMovieRoot* root, GFxValue* morphArray, UInt32 categoryIndex);