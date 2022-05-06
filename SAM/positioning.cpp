#include "positioning.h"

#include "SAF/conversions.h"
#include "SAF/util.h"

#include <math.h>

NonActorRefr selectedNonActor;

union TranslationParam
{
	UInt64 axis;
	double value;
};

//typedef UInt32 (*_UpdateTranslationInternal)(UInt64 unk, UInt32 flags, TESObjectREFR* refr, TranslationValue value);
//RelocAddr<_UpdateTranslationInternal> UpdateTranslationInternal(0xD583F0);

typedef UInt32 (*_UpdateTranslationInternal)(UInt64 unk, UInt32 flags, TESObjectREFR* refr, TranslationParam param1, TranslationParam param2);
RelocAddr<_UpdateTranslationInternal> UpdateTranslationInternal(0xD583F0);

typedef float (*_GetScaleInternal)(TESObjectREFR* refr, float mod);
RelocAddr<_GetScaleInternal> GetScaleInternal(0x3F8540);

typedef UInt64 (*_SetScaleInternal)(TESObjectREFR* refr, float scale);
RelocAddr<_SetScaleInternal> SetScaleInternal(0x3F85B0);

typedef UInt64 (*_TESObjectREFRUnk29)(TESObjectREFR* refr, UInt8 result);
RelocAddr<_TESObjectREFRUnk29> TesObjectREFRUnk29(0x40B4B0);

RelocPtr<UInt64> unkTranslation(0x5AC64F0);

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

class BSAnimationGraphManager : public NiRefObject {};

typedef bool (*_GetBSAnimationGraphInternal)(UInt64* holder, NiPointer<BSAnimationGraphManager>& result);

typedef void (*_SetAnimGraphVarInternal)(BSAnimationGraphManager* graphManager, GraphParamsPtr* params);
RelocAddr<_SetAnimGraphVarInternal> SetAnimGraphVarInternal(0x514320);

typedef bool(*_LookupREFRByHandle)(const UInt32& handleIn, NiPointer<TESObjectREFR>& ref);
extern RelocAddr <_LookupREFRByHandle> LookupREFRByHandle;

void SetFootIK(const char* enabled) {
	if (!selectedNonActor.refr || selectedNonActor.refr->formType != kFormType_ACHR) return;

	UInt64* animGraphHolder = (UInt64*)selectedNonActor.refr + 9;
	NiPointer<BSAnimationGraphManager> graphManager;
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

void UpdateRefrScale(bool isActor, double value)
{
	if (isActor) {
		SetScaleInternal(selectedNonActor.refr, value);
		TesObjectREFRUnk29(selectedNonActor.refr, 1);
	}
	else {
		TranslationParam p1;
		p1.value = value;

		UpdateTranslationInternal(*unkTranslation, 0x113C, selectedNonActor.refr, p1, p1);
		UpdateTranslationInternal(*unkTranslation, 0x113C, selectedNonActor.refr, p1, p1);
	}
}

void AdjustObjectPosition(int type, int scalar, int step) {
	if (!selectedNonActor.refr) return;
	
	float mod = step * 0.01;
	bool isActor = selectedNonActor.refr->formType == kFormType_ACHR;

	//handle scale seperately
	if (type == kAdjustScale) {
		float scale = GetScaleInternal(selectedNonActor.refr, 0.0) + scalar * 0.01 * mod;
		if (scale < 0.01)
			scale = 0.01;
		else if (scale > 10.0)
			scale = 10.0;

		UpdateRefrScale(isActor, scale);
		return;
	}

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
		if (isActor) return; //Use pose adjustments instead
		UpdateRefrTranslation(0x1009, 'X', selectedNonActor.refr->rot.x, scalar * 0.01 * mod);
		break;
	case kAdjustRotationY:
		if (isActor) return; //Use pose adjustments instead
		UpdateRefrTranslation(0x1009, 'Y', selectedNonActor.refr->rot.y, scalar * 0.01 * mod);
		break;
	case kAdjustRotationZ:
		UpdateRefrTranslation(0x1009, 'Z', selectedNonActor.refr->rot.z, scalar * 0.01 * mod);
		break;
	}

	//UpdateRefrTranslation(0x1007, 0x58, selectedNonActor.refr->pos.x);
}

void ResetObjectPosition() {
	if (!selectedNonActor.refr) return;

	SetRefrTranslation(0x1007, 0x58, selectedNonActor.translation.position.x);
	SetRefrTranslation(0x1007, 0x59, selectedNonActor.translation.position.y);
	SetRefrTranslation(0x1007, 0x6A, selectedNonActor.translation.position.z);
	
	selectedNonActor.refr->pos = selectedNonActor.translation.position;

	//UpdateRefrTranslation(0x1007, 0x58, selectedNonActor.refr->pos.x);
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

	//UpdateRefrTranslation(0x1007, 0x58, selectedNonActor.refr->pos.x);
}

void ResetObjectScale() {
	if (!selectedNonActor.refr) return;

	UpdateRefrScale(selectedNonActor.refr->formType == kFormType_ACHR, selectedNonActor.translation.scale);

	//UpdateRefrTranslation(0x1007, 0x58, selectedNonActor.refr->pos.x);
}

void SetDefaultObjectTranslation() {
	if (!selectedNonActor.refr) return;

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
	
	//sca
	UpdateRefrScale(selectedNonActor.refr->formType == kFormType_ACHR, selectedNonActor.translation.scale);
	
	selectedNonActor.refr->rot = selectedNonActor.translation.rotation;

	//UpdateRefrTranslation(0x1007, 0x58, selectedNonActor.refr->pos.x);
}

//void SetDefaultObjectTranslation() {
//	if (!selectedNonActor.refr) return;
//
//	selectedNonActor.refr->rot = NiPoint3(0.0f, 0.0f, 0.0f);
//	SetScaleInternal(selectedNonActor.refr, 1.0f);
//
//	TranslationValue zero { 0.0f };
//
//	//call twice to prevent vibrating bug
//	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
//	UpdateTranslationInternal(*unkTranslation, 0x1007, selectedNonActor.refr, zero);
//}

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

	float scale = GetScaleInternal(selectedNonActor.refr, 0.0);
	result->PushBack(&GFxValue(scale));
}