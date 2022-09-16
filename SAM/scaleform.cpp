#include "scaleform.h"

#include "f4se/ScaleformValue.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/GameMenus.h"
#include "f4se/CustomMenu.h"
#include "f4se/GameForms.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameReferences.h"
#include "f4se/GameForms.h"
#include "f4se/GameObjects.h"
#include "f4se/NiNodes.h"

#include "sam.h"
#include "hacks.h"
#include "papyrus.h"
#include "pose.h"
#include "mfg.h"
#include "idle.h"
#include "positioning.h"
#include "compatibility.h"
#include "options.h"
#include "scripts.h"
#include "camera.h"
#include "lights.h"

#include "SAF/hacks.h"
#include "SAF/eyes.h"
#include "SAF/util.h"
#include "SAF/conversions.h"

#define GFxFunction(T, Func) class T ## Scaleform : public GFxFunctionHandler \
{ \
public: \
void T ## Scaleform::Invoke(Args* args) \
Func \
};

GFxFunction(SaveState, {
	saveData.Save(&args->args[0]);
	args->args[0];
});

GFxFunction(ClearState, {
	saveData.Clear();
	int i = 0;
});

enum {
	kSamTargetError = 1,
	kSamSkeletonError,
	kSamMorphsError,
	kSamEyesError,
	kSamCameraError,
};

GFxFunction(CheckError, {
	switch (args->args[0].GetInt()) {
		case kSamTargetError: args->result->SetBool(selected.refr); break;
		case kSamSkeletonError: args->result->SetBool(CheckSelectedSkeleton()); break;
		case kSamMorphsError: args->result->SetBool(GetMorphPointer()); break;
		case kSamEyesError: args->result->SetBool(selected.eyeNode); break;
		case kSamCameraError: args->result->SetBool(GetFreeCameraState()); break;
	}
});

GFxFunction(GetOptions, {
	GetMenuOptionsGFx(args->movie->movieRoot, args->result);
});

GFxFunction(SetOption, {
	SetMenuOption(args->args[0].GetInt(), args->args[1].GetBool());
});

GFxFunction(ModifyFacegenMorph, {
	if (GetForceMorphUpdate() != kHackEnabled)
		SetForceMorphUpdate(true);

	SetFaceMorph(args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
});

GFxFunction(GetMorphCategories, {
	GetMorphCategoriesGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetMorphs, {
	GetMorphsGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
})

GFxFunction(SaveMorphsPreset, {
	SaveMfg(args->args[0].GetString());
});

GFxFunction(LoadMorphsPreset, {
	LoadMfg(args->args[0].GetString());
	GetMorphsGFx(args->movie->movieRoot, args->result, args->args[1].GetInt());
});

GFxFunction(ResetMorphs, {
	ResetMfg();
});

GFxFunction(SamPlaySound, {
	PlayUISound(args->args[0].GetString());
});

GFxFunction(SamOpenMenu, {
	OpenMenu(args->args[0].GetString());
});

GFxFunction(SamCloseMenu, {
	CloseMenu(args->args[0].GetString());
});

GFxFunction(SamIsMenuOpen, {
	args->result->SetBool((*g_ui)->IsMenuOpen(args->args[0].GetString()));
});

GFxFunction(GetHacks, {
	GetHacksGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetBlinkHack, {
	args->result->SetBool(GetBlinkState() == kHackEnabled);
});

GFxFunction(SetBlinkHack, {
	SetBlinkState(args->args[0].GetBool());
});

GFxFunction(GetMorphHack, {
	args->result->SetBool(GetForceMorphUpdate() == kHackEnabled);
});

GFxFunction(SetMorphHack, {
	SetForceMorphUpdate(args->args[0].GetBool());
});

GFxFunction(GetEyeTrackingHack, {
	args->result->SetBool(GetDisableEyecoordUpdate() == kHackEnabled);
});

GFxFunction(SetEyeTrackingHack, {
	SetDisableEyecoordUpdate(args->args[0].GetBool());
});

GFxFunction(GetEyeCoords, {
	args->movie->movieRoot->CreateArray(args->result);
	float coords[2];
	if (GetEyecoords(selected.eyeNode, coords)) {
		args->result->PushBack(&GFxValue(coords[0] * -4));
		args->result->PushBack(&GFxValue(coords[1] * 5));
	}
	 else {
	  args->result->PushBack(&GFxValue(0.0));
	  args->result->PushBack(&GFxValue(0.0));
	}
});

GFxFunction(SetEyeCoords, {
	if (GetDisableEyecoordUpdate() != kHackEnabled)
		SetDisableEyecoordUpdate(true);
	SetEyecoords(selected.eyeNode, args->args[0].GetNumber() * -0.25, args->args[1].GetNumber() * 0.2);
});

GFxFunction(GetAdjustment, {
	GetAdjustmentGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
});

GFxFunction(SetAdjustmentScale, {
	SetScale(args->args[0].GetInt(), args->args[1].GetInt());
});

GFxFunction(GetAdjustmentList, {
	GetAdjustmentsGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetCategoryList, {
	GetCategoriesGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetNodeList, {
	GetNodesGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
});

GFxFunction(GetNodeTransform, {
	GetTransformGFx(args->movie->movieRoot, args->result, args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
});

GFxFunction(SetNodePosition, {
	SetAdjustmentPos(args->args[0].GetString(), args->args[1].GetInt(), args->args[2].GetNumber(), args->args[3].GetNumber(), args->args[4].GetNumber());
});

GFxFunction(SetNodeRotation, {
	SetAdjustmentRot(args->args[0].GetString(), args->args[1].GetInt(), args->args[2].GetNumber(), args->args[3].GetNumber(), args->args[4].GetNumber());
});

GFxFunction(SetNodeScale, {
	SetAdjustmentSca(args->args[0].GetString(), args->args[1].GetInt(), args->args[2].GetNumber());
});

std::unordered_map<UInt32, UInt32> rotationTypeMap = {
	{7, kRotationX},
	{8, kRotationY},
	{9, kRotationZ}
};

GFxFunction(AdjustNodeRotation, {
	UInt32 type = rotationTypeMap[args->args[2].GetInt()];
	RotateAdjustmentXYZ(args->movie->movieRoot, args->result, args->args[0].GetString(), args->args[1].GetInt(), type, args->args[3].GetNumber());
});

GFxFunction(ResetTransform, {
	ResetAdjustmentTransform(args->args[0].GetString(), args->args[1].GetInt());
)};

GFxFunction(SaveAdjustment, {
	SaveAdjustmentFile(args->args[1].GetString(), args->args[0].GetInt());
});

GFxFunction(LoadAdjustment, {
	LoadAdjustmentFile(args->args[0].GetString());
});

GFxFunction(NewAdjustment, {
	PushNewAdjustment("New Adjustment");
	GetAdjustmentsGFx(args->movie->movieRoot, args->result);
});

GFxFunction(RemoveAdjustment, {
	EraseAdjustment(args->args[0].GetInt());
});

GFxFunction(ResetAdjustment, {
	ClearAdjustment(args->args[0].GetInt());
});

GFxFunction(NegateAdjustment, {
	NegateTransform(args->args[0].GetString(), args->args[1].GetInt());
});

GFxFunction(NegateAdjustmentGroup, {
	NegateAdjustments(args->args[0].GetInt(), args->args[1].GetString());
});

GFxFunction(MoveAdjustment, {
	args->result->SetBool(ShiftAdjustment(args->args[0].GetInt(), args->args[1].GetBool()));
});

GFxFunction(RenameAdjustment, {
	SetAdjustmentName(args->args[0].GetInt(), args->args[1].GetString());
});

GFxFunction(GetIdleCategories, {
	GetIdleMenuCategoriesGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetIdles, {
	GetIdleMenuGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
});

GFxFunction(PlayIdle, {
	PlayIdleAnimation(args->args[0].GetUInt());
});

GFxFunction(ResetIdle, {
	ResetIdleAnimation();
	ResetJsonPose();
});

GFxFunction(GetIdleName, {
	args->result->SetString(GetCurrentIdleName());
});

GFxFunction(GetPoseList, {
	GetPoseListGFx(args->movie->movieRoot, args->result);
});

GFxFunction(SavePose, {
	SaveJsonPose(args->args[0].GetString(), args->args[1]);
});

GFxFunction(LoadPose, {
	LoadJsonPose(args->args[0].GetString());
});

class ResetPoseScaleform : public GFxFunctionHandler
{
public:
	void ResetPoseScaleform::Invoke(Args* args)
	{
		switch (args->args[0].GetInt()) {
		case kPoseReset:
			ResetJsonPose();
			break;
		case kPoseAPose:
			PlayAPose();
			break;
		}
	}
};

GFxFunction(GetSkeletonAdjustments, {
	GetDefaultAdjustmentsGFx(args->movie->movieRoot, args->result, args->args[0].GetBool());
});

GFxFunction(LoadSkeletonAdjustment, {
	LoadDefaultAdjustment(args->args[0].GetString(), args->args[1].GetBool(), args->args[2].GetBool(), args->args[3].GetBool());
});

GFxFunction(ResetSkeletonAdjustment, {
	LoadDefaultAdjustment(nullptr, args->args[0].GetBool(), true, false);
});

GFxFunction(GetPositioning, {
	SaveObjectTranslation();
	GetPositioningGFx(args->movie->movieRoot, args->result);
});

GFxFunction(AdjustPositioning, {
	AdjustObjectPosition(args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
	GetPositioningGFx(args->movie->movieRoot, args->result);
});

GFxFunction(SelectPositioning, {
	SelectPositioningMenuOption(args->args[0].GetInt());
	GetPositioningGFx(args->movie->movieRoot, args->result);
});

GFxFunction(ResetPositioning, {
	SetDefaultObjectTranslation();
	GetPositioningGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetSamPoses, {
	GetSamPosesGFx(args->movie->movieRoot, args->result, args->args[0].GetString());
});

GFxFunction(SetCursorVisible, {
	static BSFixedString cursorMenu("CursorMenu");
	SetMenuVisible(cursorMenu, "root1.Cursor_mc.visible", args->args[0].GetBool());
});

GFxFunction(GetCursorPosition, {
	GetCursorPositionGFx(args->movie->movieRoot, args->result);
});

GFxFunction(SetCursorPosition, {
	SetCursor(args->args[0].GetInt(), args->args[1].GetInt());
});

GFxFunction(GetLock, {
	args->result->SetBool(GetFfcLock((FfcType)args->args[0].GetInt()));
});

GFxFunction(ToggleMenus, {
	args->result->SetBool(ToggleMenusHidden());
});

GFxFunction(GetCamera, {
	GetCameraGFx(args->movie->movieRoot, args->result);
});

GFxFunction(SetCameraPosition, {
	SetCameraPos(args->args[0].GetNumber(), args->args[1].GetNumber(), args->args[2].GetNumber());
});

GFxFunction(SetCameraRotation, {
	SetCameraRot(args->args[0].GetNumber(), args->args[1].GetNumber(), args->args[2].GetNumber());
});

GFxFunction(SetCameraFOV, {
	SetFOV(args->args[0].GetNumber());
});

GFxFunction(SaveCameraState, {
	SaveCamera(args->args[0].GetInt());
});

GFxFunction(LoadCameraState, {
	LoadCamera(args->args[0].GetInt());
});

GFxFunction(GetLightSelect, {
	GetLightSelectGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetLightEdit, {
	GetLightEditGFx(args->movie->movieRoot, args->result, args->args[0].GetUInt());
});

GFxFunction(GetLightCategories, {
	GetLightCategoriesGFx(args->movie->movieRoot, args->result);
});

GFxFunction(GetLightObjects, {
	GetLightObjectsGFx(args->movie->movieRoot, args->result, args->args[0].GetUInt());
});

GFxFunction(GetLightSettings, {
	GetLightSettingsGFx(args->movie->movieRoot, args->result);
});

GFxFunction(CreateLight, {
	CreateLight(args->args[0].GetUInt(), args->args[1].GetUInt());
});

GFxFunction(AddLight, {
	AddLight();
});

GFxFunction(EditLight, {
	EditLight(args->args[0].GetUInt(), args->args[1].GetUInt(), args->args[2].GetNumber());
});

GFxFunction(SwapLight, {
	SwapLight(args->args[0].GetUInt(), args->args[1].GetUInt(), args->args[2].GetUInt());
});

GFxFunction(DeleteLight, {
	DeleteLight(args->args[0].GetUInt());
});

GFxFunction(ResetLight, {
	ResetLight(args->args[0].GetUInt());
});

GFxFunction(RenameLight, {
	RenameLight(args->args[0].GetUInt(), args->args[1].GetString());
});

GFxFunction(GetLightVisible, {
	args->result->SetBool(GetLightVisible(args->args[0].GetUInt()));
});

GFxFunction(ToggleLightVisible, {
	args->result->SetBool(ToggleLightVisible(args->args[0].GetUInt()));
});

GFxFunction(GetAllLightsVisible, {
	args->result->SetBool(GetAllLightsVisible());
});

GFxFunction(ToggleAllLightsVisible, {
	args->result->SetBool(ToggleAllLightsVisible());
}); 

GFxFunction(EditLightSettings, {
	EditLightSettings(args->args[0].GetUInt(), args->args[1].GetNumber());
});

GFxFunction(ResetLightSettings, {
	ResetLightSettings();
});

GFxFunction(UpdateAllLights, {
	UpdateAllLights();
});

GFxFunction(DeleteAllLights, {
	DeleteAllLights();
});

GFxFunction(SaveLights, {
	SaveLightsJson(args->args[0].GetString());
});

GFxFunction(LoadLights, {
	LoadLightsJson(args->args[0].GetString());
});

GFxFunction(Test, {
	
});

GFxFunction(Test2, {

});

#define GFxRegister(T) RegisterFunction<T ## Scaleform>(value, view->movieRoot, #T)

bool RegisterScaleform(GFxMovieView* view, GFxValue* value)
{ 
	GFxRegister(SaveState);
	GFxRegister(ClearState);
	GFxRegister(CheckError);
	GFxRegister(GetOptions);
	GFxRegister(SetOption);
	GFxRegister(ModifyFacegenMorph);
	GFxRegister(GetMorphCategories);
	GFxRegister(GetMorphs);
	GFxRegister(SaveMorphsPreset);
	GFxRegister(LoadMorphsPreset);
	GFxRegister(ResetMorphs);
	GFxRegister(SamPlaySound);
	GFxRegister(SamOpenMenu);
	GFxRegister(SamCloseMenu);
	GFxRegister(SamIsMenuOpen);
	GFxRegister(GetHacks);
	GFxRegister(GetBlinkHack);
	GFxRegister(SetBlinkHack);
	GFxRegister(GetMorphHack);
	GFxRegister(SetMorphHack);
	GFxRegister(GetEyeTrackingHack);
	GFxRegister(SetEyeTrackingHack);
	GFxRegister(GetEyeCoords);
	GFxRegister(SetEyeCoords);
	GFxRegister(GetAdjustment);
	GFxRegister(SetAdjustmentScale);
	GFxRegister(GetAdjustmentList);
	GFxRegister(SaveAdjustment);
	GFxRegister(LoadAdjustment);
	GFxRegister(NewAdjustment);
	GFxRegister(RemoveAdjustment);
	GFxRegister(ResetAdjustment);
	GFxRegister(NegateAdjustment);
	GFxRegister(NegateAdjustmentGroup);
	GFxRegister(MoveAdjustment);
	GFxRegister(RenameAdjustment);
	GFxRegister(GetCategoryList);
	GFxRegister(GetNodeList);
	GFxRegister(GetNodeTransform);
	GFxRegister(SetNodePosition);
	GFxRegister(SetNodeRotation);
	GFxRegister(SetNodeScale);
	GFxRegister(AdjustNodeRotation);
	GFxRegister(ResetTransform);
	GFxRegister(GetIdleCategories);
	GFxRegister(GetIdles);
	GFxRegister(PlayIdle);
	GFxRegister(ResetIdle);
	GFxRegister(GetIdleName);
	GFxRegister(GetPoseList);
	GFxRegister(SavePose);
	GFxRegister(LoadPose);
	GFxRegister(ResetPose);
	GFxRegister(GetSkeletonAdjustments);
	GFxRegister(LoadSkeletonAdjustment);
	GFxRegister(ResetSkeletonAdjustment);
	GFxRegister(AdjustPositioning);
	GFxRegister(GetPositioning);
	GFxRegister(SelectPositioning);
	GFxRegister(ResetPositioning);
	GFxRegister(GetSamPoses);
	GFxRegister(SetCursorVisible);
	GFxRegister(GetCursorPosition);
	GFxRegister(SetCursorPosition);
	GFxRegister(GetLock);
	GFxRegister(ToggleMenus);
	GFxRegister(GetCamera);
	GFxRegister(SetCameraPosition);
	GFxRegister(SetCameraRotation);
	GFxRegister(SetCameraFOV);
	GFxRegister(SaveCameraState);
	GFxRegister(LoadCameraState);
	GFxRegister(GetLightSelect);
	GFxRegister(GetLightEdit);
	GFxRegister(GetLightCategories);
	GFxRegister(GetLightObjects);
	GFxRegister(GetLightSettings);
	GFxRegister(CreateLight);
	GFxRegister(AddLight);
	GFxRegister(EditLight);
	GFxRegister(SwapLight);
	GFxRegister(DeleteLight);
	GFxRegister(ResetLight);
	GFxRegister(RenameLight);
	GFxRegister(GetLightVisible);
	GFxRegister(ToggleLightVisible);
	GFxRegister(GetAllLightsVisible);
	GFxRegister(ToggleAllLightsVisible);
	GFxRegister(EditLightSettings);
	GFxRegister(UpdateAllLights);
	GFxRegister(ResetLightSettings);
	GFxRegister(DeleteAllLights);
	GFxRegister(SaveLights);
	GFxRegister(LoadLights);
	GFxRegister(Test);
	GFxRegister(Test2);

	return true;
}