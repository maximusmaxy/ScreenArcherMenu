#pragma once

#include "f4se/GameForms.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformMovie.h"

#include <vector>
#include <unordered_map>

class TESIdleForm : public TESForm
{
public:
	UInt64 unk20;
	UInt64 unk28;
	UInt64 unk30;
	UInt64 unk38;
	UInt64 unk40;
	BSFixedString behaviorGraph;
	BSFixedString animationEvent;
	BSFixedString animationFile;
	const char* editorID;
	UInt64 unk68;
};

typedef std::vector<std::pair<std::string, std::vector<std::pair<std::string, UInt32>>>> IdleMenu;

struct IdleData
{
	UInt32 raceId;
	UInt32 resetId;
	BSFixedString behavior;
	BSFixedString event;
};

extern std::unordered_map<UInt32, IdleData> raceIdleData;

void PlayIdleAnimation(UInt32 formId);
void ResetIdleAnimation();
void GetIdleMenuCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetIdleMenuGFx(GFxMovieRoot* root, GFxValue* result, UInt32 selectedCategory);

const char* GetCurrentIdleName();