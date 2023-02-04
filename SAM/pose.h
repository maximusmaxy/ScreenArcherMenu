#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"
#include "SAF/io.h"

#include "gfx.h"

#include <mutex>

enum {
	kPoseReset = 1,
	kPoseAPose = 2
};

extern std::unordered_map<UInt32, std::string> lastSelectedPose;

bool GetActorAdjustments(std::shared_ptr<SAF::ActorAdjustments>* adjustments);

void ResetAdjustmentTransform(const char* key, UInt32 adjustmentHandle);
void NegateTransform(const char* key, UInt32 adjustmentHandle);

void SaveAdjustmentFile(const char* filename, int adjustmentHandle);
bool LoadAdjustmentFile(const char* filename);
bool LoadAdjustmentPath(const char* path);
void LoadAdjustmentPathGFx(GFxResult& result, const char* path);
void PushNewAdjustment(const char* name);
void EraseAdjustment(UInt32 adjustmentHandle);
void ClearAdjustment(UInt32 adjustmentHandle);
void GetAdjustmentNegate(GFxResult& result);
void SetAdjustmentNegate(GFxResult& result, const char* adjustmentGroup, UInt32 adjustmentHandle);
bool ShiftAdjustment(UInt32 adjustmentHandle, bool increment);
void SetAdjustmentName(GFxResult& result, UInt32 adjustmentHandle, const char* name);

bool CheckMenuHasNode(std::shared_ptr<SAF::ActorAdjustments> adjustments, MenuList& list);

MenuCategoryList* GetAdjustmentMenu();
void SetAdjustmentScale(GFxResult& result, UInt32 adjustmentHandle, int scale);
void GetAdjustmentScale(GFxResult& result, UInt32 adjustmentHandle);
void GetAdjustments(GFxResult& result);
void GetBoneCategories(GFxResult& result);
void GetBoneNames(GFxResult& result, SInt32 categoryIndex);
void SetBoneTransform(GFxResult& result, SInt32 index, float value, UInt32 adjustmentHandle, const char* key, float yaw, float pitch, float roll);
void GetBoneTransform(GFxResult& result, const char* nodeName, int adjustmentHandle);

void GetPoseAdjustments(GFxResult& result);
void SaveJsonPose(const char* filename, GFxValue& selectedHandles, int exportType);
bool LoadPoseFile(const char* filename);
bool LoadPosePath(const char* path);
void LoadPoseGFx(GFxResult& result, const char* path);
void ResetJsonPose();
const char* GetCurrentPoseName();

void GetSkeletonAdjustments(GFxResult& result, const char* path, const char* ext, bool race);
void LoadSkeletonAdjustment(GFxResult& result, const char* filename, bool checked, bool checkbox, bool race);

void RotateAdjustmentXYZ(const char* key, int adjustmentHandle, int type, float scalar);

void GetPoseExportTypes(GFxResult& result);

void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex);
bool GetNodeIsOffset(const char* nodeName);
bool GetNodeIsOffsetOnly(const char* nodeName);
void ToggleNodeName(GFxValue* result, const char* nodeName);
void FindNodeIndexes(SAF::NodeKey& key, SInt32* categoryIndex, SInt32* nodeIndex);

void MergeAdjustment(GFxResult& result, UInt32 handle);
void MirrorAdjustment(GFxResult& result, UInt32 handle);

void AppendPoseFavorite(GFxResult& result);