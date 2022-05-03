#include "positioning.h"

#include "SAF/conversions.h"

NonActorRefr selectedNonActor;

union TranslationValue
{
	double value;
};

typedef UInt32(*_UpdateTranslationInternal)(UInt64 unk, UInt32 flags, TESObjectREFR* refr, TranslationValue value);
RelocAddr<_UpdateTranslationInternal> UpdateTranslationInternal(0xD583F0);

typedef float (*_GetScaleInternal)(TESObjectREFR* refr, float mod);
RelocAddr<_GetScaleInternal> GetScaleInternal(0x3F8540);

typedef UInt64(*_SetScaleInternal)(TESObjectREFR* refr, float scale);
RelocAddr<_SetScaleInternal> SetScaleInternal(0x3F85B0);

typedef UInt64(*_TESObjectREFRUnk29)(TESObjectREFR* refr, UInt8 result);
RelocAddr<_TESObjectREFRUnk29> TesObjectREFRUnk29(0x40B4B0);

RelocPtr<UInt64> unkTranslation(0x5AC64F0);

typedef void(*_ToggleWorldCollisionInternal)();
RelocAddr<_ToggleWorldCollisionInternal> ToggleWorldCollisionInternal(0xFBF80);

typedef void(*_ToggleGamePauseInternal)();
RelocAddr<_ToggleGamePauseInternal> ToggleGamePauseInternal(0x520530);

RelocPtr<UInt8> gamePaused(0x5AA4278);
RelocPtr<UInt8> collisionsEnabled(0x58E0AE0);

TESObjectREFR* GetNonActorRefr() {
	UInt32 handle = (*g_consoleHandle);
	NiPointer<TESObjectREFR> refr;
	if (handle == 0 || handle == *g_invalidRefHandle) {
		refr = *g_player;
	}
	else {
		LookupREFRByHandle(handle, refr);
		if (refr->flags & TESObjectREFR::kFlag_IsDeleted)
			return nullptr;
	}
	return refr;
}

void SaveObjectTranslation() {
	if (!selectedNonActor.refr) return;

	selectedNonActor.translation.position = selectedNonActor.refr->pos;
	selectedNonActor.translation.rotation = selectedNonActor.refr->rot;
	selectedNonActor.translation.scale = GetScaleInternal(selectedNonActor.refr, 0.0);
}

void UpdateNonActorRefr() {
	TESObjectREFR* refr = GetNonActorRefr();

	if (selectedNonActor.refr != refr) {
		selectedNonActor.refr = refr;
		SaveObjectTranslation();
	}
}

bool GetGamePaused() {
	UInt8* paused = gamePaused;
	return paused[0x2A];
}

void ToggleGamePaused() {
	ToggleGamePauseInternal();
}

bool GetWorldCollision() {
	return *collisionsEnabled;
}

void ToggleWorldCollision() {
	ToggleWorldCollisionInternal();
}

void ToggleFootIK() {
	static BSFixedString footIKString("bGraphWantsFootIK");
}

bool GetRefrCollision(TESObjectREFR* refr) {
	if (!refr) return false;

	TESObjectREFR::LoadedData* data = refr->unkF0;
	if (!data) return false;

	return !((data->flags >> 11) & 1);
}

void SetRefrCollision(TESObjectREFR* refr, bool enabled) {
	if (!refr) return;

	TESObjectREFR::LoadedData* data = refr->unkF0;
	if (!data) return;

	if (enabled) {
		data->flags &= 0xFFFFFFFFFFFFF7FF;
	}
	else {
		data->flags |= 0x800;
	}
}

//Rotate around axis as opposed to yaw pitch roll
NiPoint3 RotateObjectAxis(NiPoint3& rot, UInt32 axis, float step) {
	NiMatrix43 matrix;
	SAF::MatrixFromEulerYPR(matrix, rot.x, rot.y, rot.z);
	SAF::RotateMatrixXYZ(matrix, axis - kAdjustRotationX + 1, step);

	float x, y, z;
	SAF::MatrixToEulerYPR(matrix, x, y, z);
	return NiPoint3(x, y, z);
}

void AdjustObjectPosition(int type, int scalar, int step) {
	if (!selectedNonActor.refr) return;

	TranslationValue result;
	UInt32 flags;
	bool isActor = selectedNonActor.refr->formType == kFormType_ACHR;
	float mod = step * 0.01;

	switch (type) {
	case kAdjustPositionX:
		selectedNonActor.refr->pos.x += scalar * 0.1 * mod;
		result.value = selectedNonActor.refr->pos.x;
		flags = 0x1007;
		break;
	case kAdjustPositionY:
		selectedNonActor.refr->pos.y += scalar * 0.1 * mod;
		result.value = selectedNonActor.refr->pos.y;
		flags = 0x1007;
		break;
	case kAdjustPositionZ:
		selectedNonActor.refr->pos.z += scalar * 0.1 * mod;
		result.value = selectedNonActor.refr->pos.z;
		flags = 0x1007;
		break;
	case kAdjustRotationX:
		selectedNonActor.refr->rot = RotateObjectAxis(selectedNonActor.refr->rot, type, scalar * 0.01 * mod);
		result.value = selectedNonActor.refr->rot.x;
		flags = 0x1009;
		break;
	case kAdjustRotationY:
		selectedNonActor.refr->rot = RotateObjectAxis(selectedNonActor.refr->rot, type, scalar * 0.01 * mod);
		result.value = selectedNonActor.refr->rot.y;
		flags = 0x1009;
		break;
	case kAdjustRotationZ:
		selectedNonActor.refr->rot = RotateObjectAxis(selectedNonActor.refr->rot, type, scalar * 0.01 * mod);
		result.value = selectedNonActor.refr->rot.z;
		flags = 0x1009;
		break;
	}

	if (type != kAdjustScale) {
		//If actor collision is enabled, disable temporarily before moving
		bool collision = false;
		if (isActor) {
			collision = GetRefrCollision(selectedNonActor.refr);
			if (collision) {
				SetRefrCollision(selectedNonActor.refr, false);
			}
		}
		UpdateTranslationInternal(*unkTranslation, flags, selectedNonActor.refr, result);
		if (isActor && collision) {
			SetRefrCollision(selectedNonActor.refr, true);
		}
	}
	else {
		float scale = GetScaleInternal(selectedNonActor.refr, 0.0) + scalar * 0.01 * mod;
		if (scale < 0.01)
			scale = 0.01;
		else if (scale > 10.0)
			scale = 10.0;
		result.value = scale;

		//if npc call mod scale instead
		if (isActor) {
			SetScaleInternal(selectedNonActor.refr, result.value);
			TesObjectREFRUnk29(selectedNonActor.refr, 1);
		}
		else {
			result.value = scale;
			UpdateTranslationInternal(*unkTranslation, 0x113C, selectedNonActor.refr, result);
			UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, result);
		}
	}

	//call a random setpos to prevent the vibrating bug
	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, result);
}

void ResetObjectPosition() {
	if (!selectedNonActor.refr) return;

	selectedNonActor.refr->pos = selectedNonActor.translation.position;

	TranslationValue zero { 0.0 };

	//call twice to prevent vibrating bug
	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
}

void ResetObjectRotation() {
	if (!selectedNonActor.refr) return;

	selectedNonActor.refr->rot = selectedNonActor.translation.rotation;

	TranslationValue zero { 0.0 };

	//call twice to prevent vibrating bug
	UpdateTranslationInternal(*unkTranslation, 0x1009, selectedNonActor.refr, zero);
	UpdateTranslationInternal(*unkTranslation, 0x1009, selectedNonActor.refr, zero);
}

void ResetObjectScale() {
	if (!selectedNonActor.refr) return;

	SetScaleInternal(selectedNonActor.refr, selectedNonActor.translation.scale);

	TranslationValue scale { selectedNonActor.translation.scale };
	UpdateTranslationInternal(*unkTranslation, 0x113C, selectedNonActor.refr, scale);

	//random set pos to force update and prevent vibrating bug
	TranslationValue zero { 0.0 };
	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
}

void SetDefaultObjectTranslation() {
	if (!selectedNonActor.refr) return;

	selectedNonActor.refr->rot = NiPoint3(0.0f, 0.0f, 0.0f);
	SetScaleInternal(selectedNonActor.refr, 1.0f);

	TranslationValue zero { 0.0f };

	//call twice to prevent vibrating bug
	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
}

void SelectPositioningMenuOption(UInt32 option) {
	switch (option)
	{
	case kResetPosition: ResetObjectPosition(); break;
	case kResetRotation: ResetObjectRotation(); break;
	case kResetScale: ResetObjectScale(); break;
	case kTogglePause: ToggleGamePaused(); break;
	case kToggleCollision: ToggleWorldCollision(); break;
	case kToggleFootIK:	ToggleFootIK(); break;
	}
}