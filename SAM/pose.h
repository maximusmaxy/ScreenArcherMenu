#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"
#include "SAF/io.h"

#include <mutex>

extern SAF::AdjustmentManager* safAdjustmentManager;

class SafMessageDispatcher {
private:
	std::mutex mutex;
public:
	std::shared_ptr<SAF::ActorAdjustments> actorAdjustments = nullptr;
	UInt32 result = 0;

	std::shared_ptr<SAF::ActorAdjustments> GetActorAdjustments(UInt32 formId);
	UInt32 GetResult();

	void (*createAdjustment)(UInt32 formId, const char* name);
	void (*saveAdjustment)(UInt32 formId, const char* filename, UInt32 handle);
	void (*loadAdjustment)(UInt32 formId, const char* filename);
	void (*removeAdjustment)(UInt32 formId, UInt32 handle);
	void (*resetAdjustment)(UInt32 formId, UInt32 handle);
	void (*transformAdjustment)(UInt32 formId, UInt32 handle, const SAF::NodeKey nodeKey, UInt32 type, float a, float b, float c);
	void (*createActorAdjustments)(UInt32 formId);
	void (*negateAdjustments)(UInt32 formId, UInt32 handle, const char* group);
	void (*loadPose)(UInt32 formId, const char* filename);
	void (*resetPose)(UInt32 formId);
	void (*loadDefaultAdjustment)(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
	void (*moveAdjustment)(UInt32 formId, UInt32 from, UInt32 to);
	void (*renameAdjustment)(UInt32 formId, UInt32 handle, const char* name);
	void (*loadTongueAdjustment)(UInt32 formId, SAF::TransformMap* transforms);
};

extern SafMessageDispatcher safMessageDispatcher;

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
void PushNewAdjustment(const char* name);
void EraseAdjustment(UInt32 adjustmentHandle);
void ClearAdjustment(UInt32 adjustmentHandle);
void NegateAdjustments(UInt32 adjustmentHandle, const char* adjustmentGroup);
bool ShiftAdjustment(UInt32 adjustmentHandle, bool increment);
void SetAdjustmentName(UInt32 adjustmentHandle, const char* name);

MenuCategoryList* GetAdjustmentMenu();
void GetAdjustmentGFx(GFxMovieRoot* root, GFxValue* result, int adjustmentHandle);
void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result);
void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex);
void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, const char* nodeName, int adjustmentHandle);

void GetPoseListGFx(GFxMovieRoot* rot, GFxValue* result);
void SaveJsonPose(const char* filename, GFxValue selectedHandles, int exportType);
bool LoadJsonPose(const char* filename);
void ResetJsonPose();

void GetSkeletonAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result, bool race);
void LoadSkeletonAdjustment(const char* filename, bool npc, bool clear, bool enable);

void RotateAdjustmentXYZ(GFxMovieRoot* root, GFxValue* result, const char* key, int adjustmentHandle, int type, int scalar);

void GetSamPosesGFx(GFxMovieRoot* root, GFxValue* result, const char* path);
void GetPoseExportTypesGFx(GFxMovieRoot* root, GFxValue* result);

void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex);
bool GetNodeIsOffset(const char* nodeName);
void ToggleNodeName(GFxValue* result, const char* nodeName);
void FindNodeIndexes(SAF::NodeKey& key, SInt32* categoryIndex, SInt32* nodeIndex);