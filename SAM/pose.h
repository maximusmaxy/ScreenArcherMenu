#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"
#include "SAF/io.h"

#include <mutex>

enum {
	kPoseReset = 1,
	kPoseAPose = 2
};

bool CheckSelectedSkeleton();

void SetAdjustmentPos(const char* key, UInt32 adjustmentHandle, float x, float y, float z);
void SetAdjustmentRot(const char* key, UInt32 adjustmentHandle, float yaw, float pitch, float roll);
void SetAdjustmentSca(const char* key, UInt32 adjustmentHandle, float scale);
void ResetAdjustmentTransform(const char* key, int adjustmentHandle);
void NegateTransform(const char* key, UInt32 adjustmentHandle);
void SetScale(UInt32 adjustmentHandle, int scale);

void SaveAdjustmentFile(const char* filename, int adjustmentHandle);
bool LoadAdjustmentFile(const char* filename);
bool LoadAdjustmentPath(const char* path);
void PushNewAdjustment(const char* name);
void EraseAdjustment(UInt32 adjustmentHandle);
void ClearAdjustment(UInt32 adjustmentHandle);
void NegateAdjustments(UInt32 adjustmentHandle, const char* adjustmentGroup);
bool ShiftAdjustment(UInt32 adjustmentHandle, bool increment);
void SetAdjustmentName(UInt32 adjustmentHandle, const char* name);

bool CheckMenuHasNode(std::shared_ptr<SAF::ActorAdjustments> adjustments, MenuList& list);

MenuCategoryList* GetAdjustmentMenu();
void GetAdjustmentGFx(GFxMovieRoot* root, GFxValue* result, int adjustmentHandle);
void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result);
void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex);
void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, const char* nodeName, int adjustmentHandle);

void GetPoseListGFx(GFxMovieRoot* rot, GFxValue* result);
void SaveJsonPose(const char* filename, GFxValue selectedHandles, int exportType);
bool LoadPoseFile(const char* filename);
bool LoadPosePath(const char* path);
void ResetJsonPose();

void GetSkeletonAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result, const char* path, bool race);
void LoadSkeletonAdjustment(const char* filename, bool npc, bool clear, bool enable);

void RotateAdjustmentXYZ(GFxMovieRoot* root, GFxValue* result, const char* key, int adjustmentHandle, int type, int scalar);

void GetSamPosesGFx(GFxMovieRoot* root, GFxValue* result, const char* path);
void GetPoseExportTypesGFx(GFxMovieRoot* root, GFxValue* result);

void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex);
bool GetNodeIsOffset(const char* nodeName);
void ToggleNodeName(GFxValue* result, const char* nodeName);
void FindNodeIndexes(SAF::NodeKey& key, SInt32* categoryIndex, SInt32* nodeIndex);