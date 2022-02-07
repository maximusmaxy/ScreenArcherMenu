#pragma once

#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include "SAF/adjustments.h"

extern SAF::AdjustmentManager* safAdjustmentManager;

typedef std::vector<std::pair<std::string, std::string>> MenuList;
typedef std::vector<std::pair<std::string, MenuList>> MenuCategoryList;
typedef std::unordered_map<UInt64, MenuCategoryList> MenuCache;

extern MenuCache menuCache;

void SetAdjustmentPos(std::string name, UInt32 adjustmentIndex, float x, float y, float z);
void SetAdjustmentRot(std::string name, UInt32 adjustmentIndex, float heading, float attitude, float bank);
void SetAdjustmentSca(std::string name, UInt32 adjustmentIndex, float scale);
void ResetAdjustmentTransform(std::string name, int adjustmentIndex);

void SaveAdjustmentFile(std::string filename, int adjustmentIndex);
void LoadAdjustmentFile(std::string filename);
void PushNewAdjustment(std::string name);
void EraseAdjustment(int adjustmentIndex);

void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result);
void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex);
void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex, int nodeIndex, int adjustmentIndex);
void LoadMenuFiles();