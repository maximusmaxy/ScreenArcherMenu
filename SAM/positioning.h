#pragma once

#include "f4se/GameReferences.h"

struct ObjectTranslation {
	NiPoint3 position;
	NiPoint3 rotation;
	double scale;
};

struct NonActorRefr {
	TESObjectREFR* refr;
	ObjectTranslation translation;
};

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
	kToggleFootIK
};

void UpdateNonActorRefr();
void SaveObjectTranslation();

void AdjustObjectPosition(int type, double scalar, int step);
void ResetObjectPosition();
void ResetObjectRotation();
void ResetObjectScale();
void SetDefaultObjectTranslation();

bool GetGamePaused();
void ToggleGamePaused();

bool GetWorldCollision();
void ToggleWorldCollision();

void SelectPositioningMenuOption(UInt32 option);