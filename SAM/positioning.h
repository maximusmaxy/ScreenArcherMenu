#pragma once

#include "f4se/GameReferences.h"

#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"

struct ObjectTranslation {
	NiPoint3 position;
	NiPoint3 rotation;
	double scale;
};

struct NonActorRefr {
	TESObjectREFR* refr;
	ObjectTranslation translation;
};

extern NonActorRefr selectedNonActor;

enum {
	kAdjustPositionX = 1,
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

void AdjustObjectPosition(int type, int scalar, int step);
void ResetObjectPosition();
void ResetObjectRotation();
void ResetObjectScale();
void SetDefaultObjectTranslation();

bool GetGamePaused();
void ToggleGamePaused();

bool GetWorldCollision();
void ToggleWorldCollision();

void SelectPositioningMenuOption(UInt32 option);

void GetPositioningGFx(GFxMovieRoot* root, GFxValue* result);