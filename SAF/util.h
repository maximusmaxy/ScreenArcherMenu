#pragma once

#include "f4se/NiTypes.h"

#include "conversions.h"
#include "adjustments.h"

void _Logs(std::string msg, std::string msg2);
void _Logi(std::string msg, UInt64 num);
void _Logf(std::string msg, float num);

UInt32 GetFormId(const char* modName, UInt32 formId);
UInt32 GetFormId(const char* modName, const char* idString);
UInt32 GetModId(UInt32 formId);
UInt32 GetBaseId(UInt32 formId);

float Modulo(float a, float b);

std::string toLower(std::string& str);
std::string getFilename(std::string& path);
std::string HexToString(UInt32 hex);
UInt32 ComparePostfix(const char* comparer, UInt32 cLength, const char* postfix, UInt32 pLength);