#pragma once

#include "f4se/NiTypes.h"
#include "adjustments.h"

void _Log(std::string msg, UInt64 num);
void _Logf(std::string msg, float num);
void _LogCat(std::string c1, std::string c2);
UInt32 GetFormID(std::string modName, std::string formId);
bool TransormIsDefault(NiTransform& transform);
bool ContainsBSFixed(SAF::NodeSet* set, BSFixedString* str, std::string* res);