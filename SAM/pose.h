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

void SetAdjustmentPos(const char* key, UInt32 adjustmentHandle, float x, float y, float z);
void SetAdjustmentRot(const char* key, UInt32 adjustmentHandle, float yaw, float pitch, float roll);
void SetAdjustmentSca(const char* key, UInt32 adjustmentHandle, float scale);
void ResetAdjustmentTransform(const char* key, int adjustmentHandle);
void NegateTransform(const char* key, UInt32 adjustmentHandle);

void SaveAdjustmentFile(const char* filename, int adjustmentHandle);
bool LoadAdjustmentFile(const char* filename);
bool LoadAdjustmentPath(const char* path);
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
void GetAdjustmentsGFx(GFxResult& result);
void GetCategoriesGFx(GFxResult& result);
void GetNodesGFx(GFxResult& result, int categoryIndex);
void GetTransformGFx(GFxResult& result, const char* nodeName, int adjustmentHandle);

void GetPoseListGFx(GFxResult& result);
void SaveJsonPose(const char* filename, GFxValue& selectedHandles, int exportType);
bool LoadPoseFile(const char* filename);
bool LoadPosePath(const char* path);
void ResetJsonPose();
const char* GetCurrentPoseName();

void GetSkeletonAdjustmentsGFx(GFxResult& result, const char* path, bool race);
void LoadSkeletonAdjustment(const char* filename, bool npc, bool clear, bool enable);

void RotateAdjustmentXYZ(const char* key, int adjustmentHandle, int type, int scalar);

void GetSamPosesGFx(GFxResult& result, const char* path);
void GetPoseExportTypesGFx(GFxResult& result);

void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex);
bool GetNodeIsOffset(const char* nodeName);
void ToggleNodeName(GFxValue* result, const char* nodeName);
void FindNodeIndexes(SAF::NodeKey& key, SInt32* categoryIndex, SInt32* nodeIndex);

void MergeAdjustment(GFxResult& result, UInt32 handle);
void MirrorAdjustment(GFxResult& result, UInt32 handle);

void GetPoseFavorites(GFxResult& result);
void AppendPoseFavorite(GFxResult& result);
void PlayPoseFavorite(GFxResult& result, const char* idleName);
bool LoadPoseFavorites();