#include "positioning.h"

#include "SAF/conversions.h"
#include "SAF/util.h"

#include <math.h>

NonActorRefr selectedNonActor;

//typedef UInt32 (*_UpdateTranslationInternal)(UInt64 unk, UInt32 flags, TESObjectREFR* refr, TranslationValue value);
//RelocAddr<_UpdateTranslationInternal> UpdateTranslationInternal(0xD583F0);

RelocAddr<_UpdateTranslationInternal> UpdateTranslationInternal(0xD583F0);
RelocPtr<UInt64> unkTranslation(0x5AC64F0);

typedef float (*_GetScaleInternal)(TESObjectREFR* refr, float mod);
RelocAddr<_GetScaleInternal> GetScaleInternal(0x3F8540);

typedef UInt64 (*_SetScaleInternal)(TESObjectREFR* refr, float scale);
RelocAddr<_SetScaleInternal> SetScaleInternal(0x3F85B0);

typedef UInt64 (*_TESObjectREFRUnk29)(TESObjectREFR* refr, UInt8 result);
RelocAddr<_TESObjectREFRUnk29> TesObjectREFRUnk29(0x40B4B0);

typedef void (*_ToggleWorldCollisionInternal)();
RelocAddr<_ToggleWorldCollisionInternal> ToggleWorldCollisionInternal(0xFBF80);

typedef void (*_ToggleGamePauseInternal)();
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
		if (!refr || (refr->flags & TESObjectREFR::kFlag_IsDeleted))
			return nullptr;
	}
	return refr;
}

void SaveObjectTranslation() {
	if (!selectedNonActor.refr) return;

	selectedNonActor.translation.position = selectedNonActor.refr->pos;
	selectedNonActor.translation.rotation = selectedNonActor.refr->rot;
	//selectedNonActor.translation.scale = GetScaleInternal(selectedNonActor.refr, 0.0);
	selectedNonActor.translation.scale = (UInt16)selectedNonActor.refr->unk104;
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

struct GraphParams
{
	BSFixedString str;
	const char* param1;
	const char* param2;
	const char* param3;
	UInt32 unk;
};

struct GraphParamsPtr
{
	GraphParams* params;
	UInt64 unk;
};

class BSAnimationGraphManagerRef : public NiRefObject {};

typedef bool (*_GetBSAnimationGraphInternal)(UInt64* holder, NiPointer<BSAnimationGraphManagerRef>& result);

typedef void (*_SetAnimGraphVarInternal)(BSAnimationGraphManagerRef* graphManager, GraphParamsPtr* params);
RelocAddr<_SetAnimGraphVarInternal> SetAnimGraphVarInternal(0x514320);

typedef bool(*_LookupREFRByHandle)(const UInt32& handleIn, NiPointer<TESObjectREFR>& ref);
extern RelocAddr <_LookupREFRByHandle> LookupREFRByHandle;

void SetFootIK(const char* enabled) {
	if (!selectedNonActor.refr || selectedNonActor.refr->formType != kFormType_ACHR) return;

	UInt64* animGraphHolder = (UInt64*)selectedNonActor.refr + 9;
	NiPointer<BSAnimationGraphManagerRef> graphManager;
	auto func = (_GetBSAnimationGraphInternal)*(UInt64*)(*animGraphHolder + 0x20);
	UInt8 result = func(animGraphHolder, graphManager);

	if (result && graphManager) {
		BSFixedString footIKString("bGraphWantsFootIK");

		const char* zero = "0";

		GraphParams params;
		params.str = footIKString;
		params.param1 = enabled;
		params.param2 = zero;
		params.param3 = zero;
		params.unk = 0;

		GraphParamsPtr paramsPtr;
		paramsPtr.params = &params;
		paramsPtr.unk = 0;

		SetAnimGraphVarInternal(graphManager, &paramsPtr);
	}
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

NiPoint3 RadianToPositiveDegree(NiPoint3& rot) {
	NiPoint3 result;

	result.x = Modulo(rot.x * SAF::RADIAN_TO_DEGREE, 360);
	result.y = Modulo(rot.y * SAF::RADIAN_TO_DEGREE, 360);
	result.z = Modulo(rot.z * SAF::RADIAN_TO_DEGREE, 360);

	return result;
}

//void SetRefrPosRot(TESObjectREFR* refr, NiPoint3& pos, NiPoint3& rot)
//{
//	UInt32 nullHandle = *g_invalidRefHandle;
//	TESObjectCELL* parentCell = refr->parentCell;
//	TESWorldSpace* worldspace = CALL_MEMBER_FN(refr, GetWorldspace)();
//
//	MoveRefrToPosition(refr, &nullHandle, parentCell, worldspace, &pos, &rot);
//}

void SetRefrTranslation(UInt32 flags, char axis, double value)
{
	TranslationParam p1;
	p1.axis = axis;
	TranslationParam p2;
	p2.value = value;

	UpdateTranslationInternal(*unkTranslation, flags, selectedNonActor.refr, p1, p2);
}

void UpdateRefrTranslation(UInt32 flags, char axis, float& prop, float mod)
{
	float value = prop + mod;
	
	//if rot, convert radian to degree
	if (flags == 0x1009)
		SetRefrTranslation(flags, axis, value * SAF::RADIAN_TO_DEGREE);
	else
		SetRefrTranslation(flags, axis, value);

	prop = value;
}

void SetRefrScale(double value)
{
	TranslationParam p1;
	p1.value = value;
	TranslationParam p2;
	p2.axis = 0;

	UpdateTranslationInternal(*unkTranslation, 0x113C, selectedNonActor.refr, p1, p2);
	//selectedNonActor.refr->unk104 = (selectedNonActor.refr->unk104 & 0xFFFF0000) + static_cast<int>(value);

	//force scale update
	SetRefrTranslation(0x1007, 'X', selectedNonActor.refr->pos.x);
}

void AdjustObjectPosition(int type, int scalar, int step) {
	if (!selectedNonActor.refr) return;

	float mod = step * 0.01;

	switch (type) {
	case kAdjustPositionX:
		UpdateRefrTranslation(0x1007, 'X', selectedNonActor.refr->pos.x, scalar * 0.1 * mod);
		break;
	case kAdjustPositionY:
		UpdateRefrTranslation(0x1007, 'Y', selectedNonActor.refr->pos.y, scalar * 0.1 * mod);
		break;
	case kAdjustPositionZ:
		UpdateRefrTranslation(0x1007, 'Z', selectedNonActor.refr->pos.z, scalar * 0.1 * mod);
		break;
	case kAdjustRotationX:
		if (selectedNonActor.refr->formType == kFormType_ACHR) return; //Use pose adjustments instead
		UpdateRefrTranslation(0x1009, 'X', selectedNonActor.refr->rot.x, scalar * 0.01 * mod);
		break;
	case kAdjustRotationY:
		if (selectedNonActor.refr->formType == kFormType_ACHR) return; //Use pose adjustments instead
		UpdateRefrTranslation(0x1009, 'Y', selectedNonActor.refr->rot.y, scalar * 0.01 * mod);
		break;
	case kAdjustRotationZ:
		UpdateRefrTranslation(0x1009, 'Z', selectedNonActor.refr->rot.z, scalar * 0.01 * mod);
		break;
	case kAdjustScale:
		float scale = (UInt16)selectedNonActor.refr->unk104 * 0.01 + scalar * 0.01 * mod;
		if (scale < 0.01) {
			scale = 0.01;
		}
		else if (scale > 10.0) {
			scale = 10.0;
		}
		SetRefrScale(scale);
		return;
	}
}

void ResetObjectPosition() {
	if (!selectedNonActor.refr) return;

	SetRefrTranslation(0x1007, 0x58, selectedNonActor.translation.position.x);
	SetRefrTranslation(0x1007, 0x59, selectedNonActor.translation.position.y);
	SetRefrTranslation(0x1007, 0x6A, selectedNonActor.translation.position.z);
	
	selectedNonActor.refr->pos = selectedNonActor.translation.position;
}

void ResetObjectRotation() {
	if (!selectedNonActor.refr) return;

	NiPoint3 rot = RadianToPositiveDegree(selectedNonActor.translation.rotation);

	SetRefrTranslation(0x1009, 0x6A, rot.z);

	//don't update xy rot of actors
	if (selectedNonActor.refr->formType != kFormType_ACHR) {
		SetRefrTranslation(0x1009, 0x58, rot.x);
		SetRefrTranslation(0x1009, 0x59, rot.y);

		selectedNonActor.refr->rot = selectedNonActor.translation.rotation;
	}
	else {
		selectedNonActor.refr->rot.z = selectedNonActor.translation.rotation.z;
	}

	//force update
	SetRefrTranslation(0x1007, 0x58, selectedNonActor.refr->pos.x);
}

void ResetObjectScale() {
	if (!selectedNonActor.refr) return;

	SetRefrScale(selectedNonActor.translation.scale * 0.01);
	selectedNonActor.refr->unk104 = (selectedNonActor.refr->unk104 & 0xFFFF0000) + static_cast<int>(selectedNonActor.translation.scale);
}

void SetDefaultObjectTranslation() {
	if (!selectedNonActor.refr) return;

	//sca
	SetRefrScale(selectedNonActor.translation.scale * 0.01);
	selectedNonActor.refr->unk104 = (selectedNonActor.refr->unk104 & 0xFFFF0000) + static_cast<int>(selectedNonActor.translation.scale);

	//pos
	SetRefrTranslation(0x1007, 0x58, selectedNonActor.translation.position.x);
	SetRefrTranslation(0x1007, 0x59, selectedNonActor.translation.position.y);
	SetRefrTranslation(0x1007, 0x6A, selectedNonActor.translation.position.z);

	selectedNonActor.refr->pos = selectedNonActor.translation.position;

	//rot
	NiPoint3 rot = RadianToPositiveDegree(selectedNonActor.translation.rotation);

	SetRefrTranslation(0x1009, 0x6A, rot.z);

	//don't update xy rot of actors
	if (selectedNonActor.refr->formType != kFormType_ACHR) {
		SetRefrTranslation(0x1009, 0x58, rot.x);
		SetRefrTranslation(0x1009, 0x59, rot.y);

		selectedNonActor.refr->rot = selectedNonActor.translation.rotation;
	}
	else {
		selectedNonActor.refr->rot.z = selectedNonActor.translation.rotation.z;
	}
}

void SelectPositioningMenuOption(UInt32 option) {
	switch (option)
	{
	case kResetPosition: ResetObjectPosition(); break;
	case kResetRotation: ResetObjectRotation(); break;
	case kResetScale: ResetObjectScale(); break;
	case kTogglePause: ToggleGamePaused(); break;
	case kToggleCollision: ToggleWorldCollision(); break;
	case kEnableFootIK:	SetFootIK("1"); break;
	case kDisableFootIK: SetFootIK("0"); break;
	}
}

void GetPositioningGFx(GFxMovieRoot* root, GFxValue* result) {
	root->CreateArray(result);

	if (!selectedNonActor.refr) return;

	result->PushBack(&GFxValue(100));
	result->PushBack(&GFxValue(selectedNonActor.refr->pos.x));
	result->PushBack(&GFxValue(selectedNonActor.refr->pos.y));
	result->PushBack(&GFxValue(selectedNonActor.refr->pos.z));

	NiPoint3 rot = RadianToPositiveDegree(selectedNonActor.refr->rot);
	result->PushBack(&GFxValue(rot.x));
	result->PushBack(&GFxValue(rot.y));
	result->PushBack(&GFxValue(rot.z));

	result->PushBack(&GFxValue((UInt16)selectedNonActor.refr->unk104 * 0.01));
}