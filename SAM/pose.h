#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"
#include "SAF/io.h"

#include "gfx.h"

enum {
	kPoseReset = 1,
	kPoseAPose = 2
};

extern std::unordered_map<UInt32, std::string> lastSelectedPose;

bool GetActorAdjustments(SAF::ActorAdjustmentsPtr* adjustments);

void ResetAdjustmentTransform(const char* key, UInt32 adjustmentHandle);
void NegateTransform(const char* key, UInt32 adjustmentHandle);

void SaveAdjustmentFile(GFxResult& result, const char* filename, int adjustmentHandle);
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
void SetLocalAdjustmentName(UInt32 handle);

bool CheckMenuHasNode(SAF::ActorAdjustmentsPtr adjustments, MenuList& list);

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

bool GetNodeIsOffset(const char* nodeName);
const char* GetBoneInit(const char* nodeName);
void ToggleNodeName(GFxValue* result, const char* nodeName);

void MergeAdjustment(GFxResult& result, UInt32 handle);
//void MirrorAdjustment(GFxResult& result, UInt32 handle);

void AppendPoseFavorite(GFxResult& result);

void ClearBoneEdit(GFxResult& result);
void StartBoneEdit(UInt32 handle, const char* key);
void EndBoneEdit(UInt32 handle, const char* key);
void UndoBoneEdit(GFxResult& result);
void RedoBoneEdit(GFxResult& result);

std::vector<bool>* GetCachedBoneFilter(MenuCategoryList* list);
void GetBoneFilter(GFxResult& result);
void SetBoneFilter(GFxResult& result, SInt32 index, bool enabled);

void UpdateTransform(UInt32 adjustmentHandle, const char* key, SInt32 type, float scalar);