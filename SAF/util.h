#pragma once

#include "f4se/NiTypes.h"
#include "adjustments.h"

void _Log(std::string msg, UInt64 num);
void _Logf(std::string msg, float num);
UInt32 GetFormId(std::string modName, UInt32 formId);
UInt32 GetFormId(std::string modName, std::string idString);
bool TransormIsDefault(NiTransform& transform);
float Modulo(float a, float b);
std::string toLower(std::string& str);