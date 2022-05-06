#pragma once

#include "f4se/NiTypes.h"
#include "adjustments.h"

void _Log(std::string msg, UInt64 num);
void _Logf(std::string msg, float num);
void _LogCat(std::string c1, std::string c2);
UInt32 GetFormId(std::string modName, UInt32 formId);
UInt32 GetFormId(std::string modName, std::string idString);
bool TransormIsDefault(NiTransform& transform);
float Modulo(float a, float b);