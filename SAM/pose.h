#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"

extern SAF::AdjustmentManager* safAdjustmentManager;

class SafMessageDispatcher {
public:
	void (*createAdjustment)(UInt32 formId, const char* name);
	void (*loadAdjustment)(UInt32 formId, const char* filename);
	void (*removeAdjustment)(UInt32 formId, UInt32 handle);
	void (*resetAdjustment)(UInt32 formId, UInt32 handle);
	void (*transformAdjustment)(UInt32 formId, UInt32 handle, const char* key, UInt32 type, float a, float b, float c);
};

extern SafMessageDispatcher safMessageDispatcher;

void SetAdjustmentPos(const char* key, UInt32 adjustmentHandle, float x, float y, float z);
void SetAdjustmentRot(const char* key, UInt32 adjustmentHandle, float heading, float attitude, float bank);
void SetAdjustmentSca(const char* key, UInt32 adjustmentHandle, float scale);
void ResetAdjustmentTransform(const char* key, int adjustmentHandle);

void SetPersistence(UInt32 adjustmentHandle, bool isPersistent);
void SetScale(UInt32 adjustmentHandle, int scale);

void SaveAdjustmentFile(std::string filename, int adjustmentHandle);
void LoadAdjustmentFile(const char* filename);
void PushNewAdjustment(const char* name);
void EraseAdjustment(int adjustmentHandle);
void ClearAdjustment(UInt32 adjustmentHandle);

void GetAdjustmentGFx(GFxMovieRoot* root, GFxValue* result, int adjustmentHandle);
void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result);
void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex);
void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex, int nodeIndex, int adjustmentHandle);