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
#include "gfx.h"
#include "inventory.h"
#include "eyes.h"
#include "io.h"

#include "SAF/hacks.h"
#include "SAF/util.h"
#include "SAF/conversions.h"

#define GFxFunction(T, Func) class T ## Scaleform : public GFxFunctionHandler \
{ \
public: \
void T ## Scaleform::Invoke(Args* args) \
Func \
}

GFxFunction(GetMenu, {
	GetMenuGFx(GFxResult(args), args->args[0].GetString());
});

//LoadMenu is is a cpp function
class LoadMenuScaleform : public GFxFunctionHandler
{
public:
	void LoadMenuScaleform::Invoke(Args* args)
	{
		samManager.PushMenu(args->args[0].GetString());
	}
};

GFxFunction(LoadMenuValue, {
	samManager.PushMenu(args->args[1].GetString());
});

GFxFunction(GetExtensions, {
	GetExtensionMenusGFx(GFxResult(args));
});

GFxFunction(CallPapyrusForm, {
	CallPapyrusForm(GFxResult(args), args->args[0].GetString(), args->args[1].GetString(), args->args[2]);
});

GFxFunction(GetPathStem, {
	GetPathStem(GFxResult(args), args->args[0].GetString());
});

GFxFunction(GetPathRelative, {
	GetPathRelative(GFxResult(args), args->args[0].GetString(), args->args[1].GetString(), args->args[2].GetString());
});

GFxFunction(SaveState, {
	samManager.SaveData(&args->args[0]);
});

GFxFunction(ClearState, {
	samManager.ClearData();
});

GFxFunction(GetOptions, {
	GetMenuOptionsGFx(GFxResult(args));
});

GFxFunction(SetOption, {
	GFxResult result(args);
	SetMenuOption(args->args[0].GetInt(), args->args[1].GetBool());
});

GFxFunction(ModifyFacegenMorph, {
	if (GetForceMorphUpdate() != kHackEnabled)
		SetForceMorphUpdate(true);

	SetFaceMorph(args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
});

GFxFunction(GetMorphCategories, {
	GetMorphCategoriesGFx(GFxResult(args));
});

GFxFunction(GetMorphs, {
	GetMorphsGFx(GFxResult(args), args->args[0].GetInt());
});

GFxFunction(SaveMorphsPreset, {
	SaveMfg(args->args[0].GetString());
});

GFxFunction(LoadMorphsPreset, {
	LoadMfgPath(args->args[0].GetString());
});

GFxFunction(ResetFaceMorphs, {
	ResetMfg();
});

GFxFunction(GetMorphsTongue, {
	GetMorphsTongueGFx(args->movie->movieRoot, args->result, args->args[0].GetInt(), args->args[1].GetInt());
});

GFxFunction(GetMorphsTongueNodes, {
	GetMorphsTongueNodesGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
});

GFxFunction(SamPlaySound, {
	PlayUISound(args->args[0].GetString());
});

GFxFunction(SamOpenMenu, {
	samManager.OpenMenu(args->args[0].GetString());
});

GFxFunction(SamCloseMenu, {
	samManager.CloseMenu(args->args[0].GetString());
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

GFxFunction(GetEyes, {
	GetEyes(GFxResult(args));
});

GFxFunction(SetEyes, {
	SetEyes(GFxResult(args), args->args[2].GetNumber(), args->args[3].GetNumber());
});

GFxFunction(GetAdjustmentScale, {
	GetAdjustmentScale(GFxResult(args), args->args[0].GetUInt());
});

GFxFunction(SetAdjustmentScale, {
	SetAdjustmentScale(GFxResult(args), args->args[2].GetUInt(), args->args[1].GetUInt());
});

GFxFunction(GetAdjustmentList, {
	GetAdjustmentsGFx(GFxResult(args));
});

GFxFunction(GetCategoryList, {
	GetCategoriesGFx(GFxResult(args));
});

GFxFunction(GetNodeList, {
	GetNodesGFx(GFxResult(args), args->args[0].GetInt());
});

GFxFunction(GetNodeTransform, {
	GetTransformGFx(GFxResult(args), args->args[0].GetString(), args->args[1].GetInt());
});

GFxFunction(GetNodeIsOffset, {
	args->result->SetBool(GetNodeIsOffset(args->args[0].GetString()));
});

GFxFunction(ToggleNodeName, {
	ToggleNodeName(args->result, args->args[0].GetString());
});

GFxFunction(GetNodeNameFromIndexes, {
	GetNodeNameFromIndexes(args->result, args->args[0].GetInt(), args->args[1].GetInt());
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
	{7, kAxisX},
	{8, kAxisY},
	{9, kAxisZ}
};

GFxFunction(AdjustNodeRotation, {
	UInt32 type = rotationTypeMap[args->args[2].GetInt()];
	RotateAdjustmentXYZ(args->args[0].GetString(), args->args[1].GetInt(), type, args->args[3].GetNumber());
});

GFxFunction(ResetTransform, {
	ResetAdjustmentTransform(args->args[0].GetString(), args->args[1].GetInt());
)};

GFxFunction(SaveAdjustment, {
	GFxResult result(args);
	SaveAdjustmentFile(args->args[0].GetString(), args->args[1].GetInt());
});

GFxFunction(LoadAdjustment, {
	LoadAdjustmentPath(args->args[0].GetString());
});

GFxFunction(NewAdjustment, {
	PushNewAdjustment("New Adjustment");
	GetAdjustmentsGFx(GFxResult(args));
});

GFxFunction(RemoveAdjustment, {
	EraseAdjustment(args->args[0].GetInt());
});

GFxFunction(ResetAdjustment, {
	GFxResult result(args);
	ClearAdjustment(args->args[2].GetInt());
});

GFxFunction(NegateAdjustment, {
	NegateTransform(args->args[0].GetString(), args->args[1].GetInt());
});

GFxFunction(GetAdjustmentNegate, {
	GetAdjustmentNegate(GFxResult(args));
});

GFxFunction(SetAdjustmentNegate, {
	SetAdjustmentNegate(GFxResult(args), args->args[1].GetString(), args->args[2].GetUInt());
});

GFxFunction(MoveAdjustment, {
	args->result->SetBool(ShiftAdjustment(args->args[0].GetInt(), args->args[1].GetBool()));
});

GFxFunction(RenameAdjustment, {
	SetAdjustmentName(GFxResult(args), args->args[1].GetInt(), args->args[0].GetString());
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

GFxFunction(GetPoseName, {
	args->result->SetString(GetCurrentPoseName());
});

GFxFunction(GetPoseList, {
	GetPoseListGFx(GFxResult(args));
});

GFxFunction(SavePose, {
	SaveJsonPose(args->args[0].GetString(), args->args[1], args->args[2].GetUInt());
});

GFxFunction(LoadPose, {
	LoadPosePath(args->args[0].GetString());
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
	GetSkeletonAdjustmentsGFx(GFxResult(args), args->args[0].GetString(), args->args[1].GetBool());
});

GFxFunction(LoadSkeletonAdjustment, {
	LoadSkeletonAdjustment(args->args[0].GetString(), args->args[1].GetBool(), args->args[2].GetBool(), args->args[3].GetBool());
});

GFxFunction(ResetSkeletonAdjustment, {
	LoadSkeletonAdjustment(nullptr, args->args[0].GetBool(), true, false);
});

GFxFunction(GetPositioning, {
	SaveObjectTranslation();
	GetPositioningGFx(args->movie->movieRoot, args->result);
});

GFxFunction(RotateIdle, {
	AdjustObjectPosition(kAdjustRotationZ, args->args[1].GetNumber(), 100);
});

GFxFunction(AdjustPositioning, {
	AdjustObjectPosition(args->args[0].GetInt(), args->args[1].GetNumber(), args->args[2].GetInt());
});

GFxFunction(SelectPositioning, {
	SelectPositioningMenuOption(args->args[0].GetInt());
});

GFxFunction(ResetPositioning, {
	SetDefaultObjectTranslation();
});

GFxFunction(GetFolder, {
	GetFolderGFx(GFxResult(args), args->args[0].GetString(), args->args[1].GetString());
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
	GetCameraGFx(GFxResult(args));
});

GFxFunction(SetCamera, {
	SetCamera(GFxResult(args), args->args[0].GetInt(), args->args[1].GetNumber());
});

GFxFunction(SaveCameraState, {
	SaveCamera(args->args[1].GetInt());
});

GFxFunction(LoadCameraState, {
	LoadCamera(args->args[1].GetInt());
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
	LoadLightsPath(args->args[0].GetString());
});

GFxFunction(GetPoseExportTypes, {
	GetPoseExportTypesGFx(GFxResult(args));
});

GFxFunction(MergeAdjustmentDown, {
	MergeAdjustment(GFxResult(args), args->args[2].GetUInt());
});

GFxFunction(MirrorAdjustment, {
	MirrorAdjustment(GFxResult(args), args->args[2].GetUInt());
});

GFxFunction(OpenActorContainer, {
	OpenActorContainer(GFxResult(args));
});

void testFunc() {

}

void testFunc2() {

}

GFxFunction(Test, {
	testFunc();
});

GFxFunction(Test2, {
	testFunc2();
});

#define GFxRegister(T) RegisterFunction<T ## Scaleform>(value, view->movieRoot, #T)

bool RegisterScaleform(GFxMovieView* view, GFxValue* value)
{ 
	GFxRegister(GetMenu);
	RegisterFunction<LoadMenuScaleform>(value, view->movieRoot, "LoadMenu");
	GFxRegister(LoadMenuValue);
	GFxRegister(GetExtensions);
	GFxRegister(CallPapyrusForm);
	GFxRegister(GetPathStem);
	GFxRegister(GetPathRelative);
	GFxRegister(SaveState);
	GFxRegister(ClearState);
	GFxRegister(GetOptions);
	GFxRegister(SetOption);
	GFxRegister(ModifyFacegenMorph);
	GFxRegister(GetMorphCategories);
	GFxRegister(GetMorphs);
	GFxRegister(SaveMorphsPreset);
	GFxRegister(LoadMorphsPreset);
	GFxRegister(ResetFaceMorphs);
	GFxRegister(GetMorphsTongue);
	GFxRegister(GetMorphsTongueNodes);
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
	GFxRegister(GetEyes);
	GFxRegister(SetEyes);
	GFxRegister(GetAdjustmentScale);
	GFxRegister(SetAdjustmentScale);
	GFxRegister(GetAdjustmentList);
	GFxRegister(SaveAdjustment);
	GFxRegister(LoadAdjustment);
	GFxRegister(NewAdjustment);
	GFxRegister(RemoveAdjustment);
	GFxRegister(ResetAdjustment);
	GFxRegister(NegateAdjustment);
	GFxRegister(GetAdjustmentNegate);
	GFxRegister(SetAdjustmentNegate);
	GFxRegister(MoveAdjustment);
	GFxRegister(RenameAdjustment);
	GFxRegister(GetCategoryList);
	GFxRegister(GetNodeList);
	GFxRegister(GetNodeIsOffset);
	GFxRegister(ToggleNodeName);
	GFxRegister(GetNodeNameFromIndexes);
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
	GFxRegister(GetPoseList);
	GFxRegister(GetIdleName);
	GFxRegister(GetPoseName);
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
	GFxRegister(GetFolder);
	GFxRegister(SetCursorVisible);
	GFxRegister(GetCursorPosition);
	GFxRegister(SetCursorPosition);
	GFxRegister(GetLock);
	GFxRegister(ToggleMenus);
	GFxRegister(GetCamera);
	GFxRegister(SetCamera);
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
	GFxRegister(GetPoseExportTypes);
	GFxRegister(Test);
	GFxRegister(Test2);

	return true;
}