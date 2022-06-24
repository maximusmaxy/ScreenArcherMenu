#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"

#include <mutex>

extern SAF::AdjustmentManager* safAdjustmentManager;

class SafMessageDispatcher {
private:
	std::mutex mutex;
public:
	std::shared_ptr<SAF::ActorAdjustments> actorAdjustments = nullptr;
	bool result = false;

	std::shared_ptr<SAF::ActorAdjustments> GetActorAdjustments(UInt32 formId);
	bool GetResult();

	void (*createAdjustment)(UInt32 formId, const char* name);
	void (*saveAdjustment)(UInt32 formId, const char* filename, UInt32 handle);
	void (*loadAdjustment)(UInt32 formId, const char* filename);
	void (*removeAdjustment)(UInt32 formId, UInt32 handle);
	void (*resetAdjustment)(UInt32 formId, UInt32 handle);
	void (*transformAdjustment)(UInt32 formId, UInt32 handle, const char* key, UInt32 type, float a, float b, float c);
	void (*createActorAdjustments)(UInt32 formId);
	void (*negateAdjustments)(UInt32 formId, UInt32 handle, const char* group);
	void (*loadPose)(UInt32 formId, const char* filename);
	void (*resetPose)(UInt32 formId);
	void (*loadDefaultAdjustment)(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
	void (*moveAdjustment)(UInt32 formId, UInt32 from, UInt32 to);
	void (*renameAdjustment)(UInt32 formId, UInt32 handle, const char* name);
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

void GetAdjustmentGFx(GFxMovieRoot* root, GFxValue* result, int adjustmentHandle);
void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result);
void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex);
void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex, int nodeIndex, int adjustmentHandle);

void GetPoseListGFx(GFxMovieRoot* rot, GFxValue* result);
void SaveJsonPose(const char* filename, GFxValue selectedHandles);
bool LoadJsonPose(const char* filename);
void ResetJsonPose();

void GetDefaultAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result);
void LoadDefaultAdjustment(const char* filename, bool npc, bool clear, bool enable);

void RotateAdjustmentXYZ(GFxMovieRoot* root, GFxValue* result, const char* key, int adjustmentHandle, int type, int scalar);

void GetSamPosesGFx(GFxMovieRoot* root, GFxValue* result, const char* path);