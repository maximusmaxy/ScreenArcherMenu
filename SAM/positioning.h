#pragma once

#include "f4se/GameReferences.h"

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

#include "gfx.h"

union TranslationParam
{
	UInt64 axis;
	double value;
};

typedef UInt32(*_UpdateTranslationInternal)(UInt64 unk, UInt32 flags, TESObjectREFR* refr, TranslationParam param1, TranslationParam param2);
extern RelocAddr<_UpdateTranslationInternal> UpdateTranslationInternal;

extern RelocPtr<UInt64> unkTranslation;

struct ObjectTranslation {
	NiPoint3 position;
	NiPoint3 rotation;
	UInt16 scale;
};

struct NonActorRefr {
	TESObjectREFR* refr;
	ObjectTranslation translation;
};

extern NonActorRefr selectedNonActor;

enum {
	kAdjustPositionStep = 0,
	kAdjustPositionX,
	kAdjustPositionY,
	kAdjustPositionZ,
	kAdjustRotationX,
	kAdjustRotationY,
	kAdjustRotationZ,
	kAdjustScale,
	kResetPosition,
	kResetRotation,
	kResetScale,
	kTogglePause,
	kToggleCollision,
	kEnableFootIK,
	kDisableFootIK
};

void UpdateNonActorRefr();
void SaveObjectTranslation();

void AdjustObjectPosition(GFxResult& result, int type, GFxValue& value, bool hasStep);
void AdjustRefrPosition(TESObjectREFR* refr, int type, float dif);
void ResetObjectPosition();
void ResetObjectRotation();
void ResetObjectScale();
void SetDefaultObjectTranslation();

bool GetGamePaused();
void ToggleGamePaused();

bool GetWorldCollision();
void ToggleWorldCollision();

void SelectPositioningMenuOption(UInt32 option);

void GetPositioning(GFxResult& result);