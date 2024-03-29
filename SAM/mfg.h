#pragma once

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "gfx.h"

class BSFaceGenAnimationData {
public:
	UInt64 vfTable;				//0
	UInt32 size;				//8
	UInt32 unk1;				//C
	UInt64 unk2;				//10
	float faceAnimMorphs[54];	//18
	float mfgMorphs[54];		//F0
	//ect...
};

float* GetMorphPointer();
void SetFaceMorph(SInt32 categoryIndex, SInt32 morphIndex, SInt32 scale);
void SaveMfg(const char* filename);
bool LoadMfgFile(const char* filename);
bool LoadMfgPath(const char* filename);
void ResetMfg();
void GetMorphCategories(GFxResult& result);
void GetMorphs(GFxResult& result, SInt32 categoryIndex);
void GetMorphsTongueNodes(GFxResult& result);
//void GetMorphsTongueGFx(GFxMovieRoot* root, GFxValue* result, UInt32 categoryIndex, UInt32 tongueIndex);
void SetFaceMorphCategory(GFxResult& result, SInt32 index, UInt32 value);