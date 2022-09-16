#pragma once

#include "f4se/NiTypes.h"
#include "adjustments.h"

void _Log(std::string msg, UInt64 num);
void _Logf(std::string msg, float num);

UInt32 GetFormId(std::string modName, UInt32 formId);
UInt32 GetFormId(std::string modName, std::string idString);
UInt32 GetModId(UInt32 formId);
UInt32 GetBaseId(UInt32 formId);

bool TransormIsDefault(NiTransform& transform);
bool TransformMapIsDefault(SAF::TransformMap& map);

float Modulo(float a, float b);

std::string toLower(std::string& str);
std::string getFilename(std::string& path);
std::string HexToString(UInt32 hex);