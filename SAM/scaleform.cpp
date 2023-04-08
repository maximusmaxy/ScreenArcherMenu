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
#include "constants.h"
#include "bodymorphs.h"

#include "SAF/hacks.h"
#include "SAF/util.h"
#include "SAF/conversions.h"

//Standard function
#define GFxFunction(T, Func) class T ## Scaleform : public GFxFunctionHandler \
{ \
public: \
void T ## Scaleform::Invoke(Args* arguments) \
{ \
GFxValue* result = arguments->result; \
GFxValue* args = arguments->args; \
Func \
} \
}

//Creates a GFxResult which will handle the response state ie success, error, waiting. Will default to success
//TODO a lot of requests still need proper error responses
#define GFxRequest(T, Func) class T ## Scaleform : public GFxFunctionHandler \
{ \
public: \
void T ## Scaleform::Invoke(Args* arguments) \
{ \
GFxResult result(arguments); \
GFxValue* args = arguments->args; \
Func \
} \
}

GFxRequest(GetMenu, {
	GetMenuData(result, args[0].GetString());
});

//LoadMenu is taken by WinUser.h :)
class LoadMenuScaleform : public GFxFunctionHandler
{
public:
	void LoadMenuScaleform::Invoke(Args* args)
	{
		samManager.PushMenu(args->args[0].GetString());
	}
};

GFxFunction(LoadMenuValue, {
	samManager.PushMenu(args[1].GetString());
});

GFxRequest(GetExtensions, {
	GetExtensionMenus(result);
});

GFxRequest(LoadExtension, {
	samManager.PushMenu(args[1].GetString());
});

GFxRequest(CallPapyrusForm, {
	CallPapyrusForm(result, args[0].GetString(), args[1].GetString(), args[2]);
});

GFxRequest(GetPathStem, {
	GetPathStem(result, args[0].GetString());
});

GFxRequest(GetPathRelative, {
	GetPathRelative(result, args[0].GetString(), args[1].GetString(), args[2].GetString());
});

GFxFunction(SaveState, {
	samManager.SaveData(&args[0]);
});

GFxFunction(ClearState, {
	samManager.ClearData();
});

GFxRequest(GetOptions, {
	GetMenuOptions(result);
});

GFxRequest(SetOption, {
	SetMenuOption(result, args[0].GetInt(), args[1].GetBool());
});

GFxRequest(GetFaceMorphs, {
	GetMorphs(result, args[0].GetInt());
});

GFxRequest(SetFaceMorph, {
	SetFaceMorph(args[0].GetInt(), args[1].GetInt(), args[2].GetInt());
});

GFxRequest(GetFaceMorphCategories, {
	GetMorphCategories(result);
});

GFxRequest(SetFaceMorphCategory, {
	SetFaceMorphCategory(result, args[0].GetInt(), args[1].GetUInt());
});

GFxRequest(SaveFaceMorphs, {
	SaveMfg(args[0].GetString());
});

GFxRequest(LoadFaceMorphs, {
	LoadMfgPath(args[0].GetString());
});

GFxRequest(ResetFaceMorphs, {
	ResetMfg();
});

GFxRequest(GetTongueBones, {
	GetMorphsTongueNodes(result);
});

GFxFunction(SamPlaySound, {
	PlayUISound(args[0].GetString());
});

//TODO remove this
GFxFunction(SamOpenMenu, {
	//samManager.OpenMenu();
});

GFxFunction(SamCloseMenu, {
	samManager.CloseMenu();
});

GFxFunction(SamIsMenuOpen, {
	result->SetBool((*g_ui)->IsMenuOpen(args[0].GetString()));
});

GFxRequest(GetHacks, {
	GetHacks(result);
});

GFxRequest(SetHack, {
	SetHack(args[0].GetInt(), args[1].GetBool());
});

GFxRequest(GetEyes, {
	GetEyes(result);
});

GFxRequest(SetEyes, {
	SetEyes(result, args[2].GetNumber(), args[3].GetNumber());
});

GFxRequest(GetAdjustmentScale, {
	GetAdjustmentScale(result, args[0].GetUInt());
});

GFxRequest(SetAdjustmentScale, {
	SetAdjustmentScale(result, args[2].GetUInt(), args[1].GetUInt());
});

GFxRequest(GetAdjustments, {
	GetAdjustments(result);
});

GFxRequest(GetBoneCategories, {
	GetBoneCategories(result);
});

GFxRequest(GetBoneNames, {
	GetBoneNames(result, args[0].GetInt());
});

GFxRequest(GetBoneTransform, {
	GetBoneTransform(result, args[1].GetString(), args[0].GetUInt());
});

GFxRequest(SetBoneTransform, {
	//index, value, handle, key, yaw, pitch, roll
	SetBoneTransform(result, args[0].GetInt(), args[1].GetNumber(), args[2].GetUInt(), args[3].GetString(),
		args[4].GetNumber(), args[5].GetNumber(), args[6].GetNumber());
});

GFxFunction(GetNodeIsOffset, {
	result->SetBool(GetNodeIsOffset(args[0].GetString()));
});

GFxFunction(GetBoneInit, {
	result->SetString(GetBoneInit(args[0].GetString()));
});

GFxFunction(ToggleNodeName, {
	ToggleNodeName(result, args[0].GetString());
});

GFxRequest(RotateBoneTransform, {
	RotateAdjustmentXYZ(args[3].GetString(), args[2].GetUInt(), args[0].GetInt() - 6, args[1].GetNumber());
});

GFxRequest(ResetBoneTransform, {
	ResetAdjustmentTransform(args[1].GetString(), args[0].GetUInt());
)};

GFxRequest(SaveAdjustment, {
	SaveAdjustmentFile(result, args[0].GetString(), args[1].GetUInt());
});

GFxRequest(LoadAdjustment, {
	LoadAdjustmentPathGFx(result, args[0].GetString());
});

GFxRequest(NewAdjustment, {
	PushNewAdjustment("New Adjustment");
});

GFxRequest(RemoveAdjustment, {
	EraseAdjustment(args[1].GetUInt());
});

GFxRequest(ResetAdjustment, {
	ClearAdjustment(args[2].GetInt());
});

GFxRequest(NegateBoneTransform, {
	NegateTransform(args[1].GetString(), args[0].GetUInt());
});

GFxRequest(GetAdjustmentNegate, {
	GetAdjustmentNegate(result);
});

GFxRequest(SetAdjustmentNegate, {
	SetAdjustmentNegate(result, args[1].GetString(), args[2].GetUInt());
});

GFxFunction(MoveAdjustment, {
	result->SetBool(ShiftAdjustment(args[0].GetUInt(), args[1].GetBool()));
});

GFxRequest(RenameAdjustment, {
	SetAdjustmentName(result, args[1].GetUInt(), args[0].GetString());
});

GFxRequest(SetLocalAdjustmentName, {
	SetLocalAdjustmentName(args[0].GetUInt());
});

GFxRequest(GetIdleMods, {
	GetIdleMenuCategories(result);
});

GFxRequest(SetIdleMod, {
	SetIdleMod(result, args[1].GetInt());
});

GFxRequest(GetIdles, {
	GetIdleMenu(result, args[0].GetInt());
});

GFxRequest(PlayIdle, {
	PlayIdleAnimation(args[1].GetUInt());
});

GFxRequest(ResetIdle, {
	ResetIdleAnimation();
	ResetJsonPose();
});

GFxRequest(GetIdleName, {
	result.SetManagedString(arguments->movie->movieRoot, GetCurrentIdleName());
});

GFxRequest(GetPoseName, {
	result.SetManagedString(arguments->movie->movieRoot, GetCurrentPoseName());
});

GFxRequest(GetPoseAdjustments, {
	GetPoseAdjustments(result);
});

GFxRequest(SavePose, {
	SaveJsonPose(args[0].GetString(), args[1], args[2].GetUInt());
});

GFxRequest(LoadPose, {
	LoadPoseGFx(result, args[0].GetString());
});

GFxRequest(ResetPose, {
	ResetJsonPose();
});

GFxRequest(PlayAPose, {
	PlayAPose();
});

GFxRequest(GetSkeletonAdjustments, {
	GetSkeletonAdjustments(result, args[0].GetString(), args[1].GetString(), args[2].GetBool());
});

GFxRequest(LoadSkeletonAdjustment, {
	LoadSkeletonAdjustment(result, args[0].GetString(), args[1].GetBool(), args[2].GetBool(), args[3].GetBool());
});

GFxRequest(ResetSkeletonAdjustment, {
	LoadSkeletonAdjustment(result, nullptr, false, false, args[0].GetBool());
});

GFxRequest(SaveObjectPosition, {
	SaveObjectTranslation();
});

GFxRequest(GetPositioning, {
	GetPositioning(result);
});

GFxRequest(SetPositioning, {
	AdjustObjectPosition(result, args[0].GetInt(), args[1], true);
});

GFxRequest(RotateIdle, {
	AdjustObjectPosition(result, kAdjustRotationZ, args[0], false);
});

GFxRequest(SelectPositioning, {
	SelectPositioningMenuOption(args[0].GetInt());
});

GFxRequest(ResetPositioning, {
	SetDefaultObjectTranslation();
});

GFxRequest(GetFolder, {
	GetFolder(result, args[0].GetString(), args[1].GetString());
});

GFxFunction(SetCursorVisible, {
	static BSFixedString cursorMenu("CursorMenu");
	SetMenuVisible(cursorMenu, "root1.Cursor_mc.visible", args[0].GetBool());
});

GFxRequest(GetCursorPosition, {
	GetCursorPosition(result);
});

GFxFunction(SetCursorPosition, {
	SetCursor(args[0].GetInt(), args[1].GetInt());
});

GFxFunction(GetLock, {
	result->SetBool(GetFfcLock((FfcType)args[0].GetInt()));
});

GFxFunction(ToggleMenus, {
	result->SetBool(ToggleMenusHidden());
});

GFxRequest(GetCamera, {
	GetCamera(result);
});

GFxRequest(SetCamera, {
	SetCamera(result, args[0].GetInt(), args[1].GetNumber());
});

GFxRequest(SaveCameraFile, {
	if (!SaveCameraFile(args[0].GetString()))
		return result.SetError(CAMERA_ERROR); //TODO need better error messaging
});

GFxRequest(LoadCameraPath, {
	LoadCameraGFx(result, args[0].GetString());
});

GFxRequest(SaveCameraState, {
	SaveCameraState(args[1].GetInt());
});

GFxRequest(LoadCameraState, {
	LoadCameraState(args[1].GetInt());
});

GFxRequest(GetLightSelect, {
	GetLightSelect(result);
});

GFxRequest(SetLightSelect, {
	SetLightSelect(result, args[1].GetInt());
});

GFxRequest(GetLightEdit, {
	GetLightEdit(result, args[0].GetInt());
});

GFxRequest(GetLightCategories, {
	GetLightCategories(result);
});

GFxRequest(GetLightForms, {
	GetLightForms(result, args[0].GetInt());
});

GFxRequest(CreateLight, {
	CreateLight(result, args[1].GetUInt());
});

GFxRequest(AddLight, {
	AddLight(result);
});

GFxRequest(EditLight, {
	EditLight(result, args[0].GetUInt(), args[1].GetNumber(), args[2].GetInt());
});

GFxRequest(SwapLight, {
	SwapLight(result, args[1].GetUInt(), args[2].GetInt());
});

GFxRequest(DeleteLight, {
	DeleteLight(result, args[2].GetInt());
});

GFxRequest(ResetLight, {
	ResetLight(result, args[0].GetInt());
});

GFxRequest(RenameLight, {
	RenameLight(result, args[0].GetString(), args[1].GetInt());
});

GFxFunction(GetLightVisible, {
	result->SetBool(GetLightVisible(args[0].GetInt()));
});

GFxFunction(ToggleLightVisible, {
	result->SetBool(ToggleLightVisible(args[0].GetInt()));
});

GFxFunction(GetAllLightsVisible, {
	result->SetBool(GetAllLightsVisible());
});

GFxFunction(ToggleAllLightsVisible, {
	result->SetBool(ToggleAllLightsVisible());
}); 

GFxRequest(GetLightsGlobal, {
	GetLightSettings(result);
});

GFxRequest(SetLightsGlobal, {
	EditLightSettings(result, args[0].GetUInt(), args[1].GetNumber());
});

GFxRequest(ResetLightSettings, {
	ResetLightSettings();
});

GFxRequest(UpdateAllLights, {
	UpdateAllLights();
});

GFxRequest(DeleteAllLights, {
	DeleteAllLights();
});

GFxRequest(SaveLights, {
	SaveLightsJson(args[0].GetString());
});

GFxRequest(LoadLights, {
	LoadLightsPath(args[0].GetString());
});

GFxRequest(GetExportTypes, {
	GetPoseExportTypes(result);
});

GFxRequest(MergeAdjustmentDown, {
	MergeAdjustment(result, args[2].GetUInt());
});

//GFxRequest(MirrorAdjustment, {
//	MirrorAdjustment(result, args[2].GetUInt());
//});

GFxRequest(OpenActorContainer, {
	OpenActorContainer(result);
});

GFxRequest(GetItemMods, {
	GetItemMods(result);
});

GFxRequest(GetItemGroups, {
	GetItemGroups(result, args[0].GetString());
});

GFxRequest(GetItemList, {
	GetItemList(result, args[0].GetString(), args[1].GetInt());
});

GFxRequest(AddItem, {
	AddItem(result, args[1].GetUInt(), args[2].GetBool());
});

GFxRequest(EquipSearchedItem, {
	AddItem(result, args[1].GetUInt(), !args[2].GetBool());
});

GFxRequest(GetLastSearchResult, {
	GetLastSearchResult(result);
});

GFxRequest(SearchItems, {
	SearchItems(result, args[0].GetString());
});

GFxRequest(GetMatSwapEquipment, {
	GetMatSwapEquipment(result);
});

GFxRequest(GetMatSwaps, {
	GetMatSwaps(result, args[0].GetUInt());
});

GFxRequest(ApplyMatSwap, {
	ApplyMatSwap(result, args[1].GetUInt(), args[2].GetUInt());
});

GFxRequest(GetEquipment, {
	GetEquipment(result);
});

GFxRequest(RemoveEquipment, {
	RemoveEquipment(result, args[1].GetUInt());
});

GFxRequest(RemoveAllEquipment, {
	RemoveAllEquipment(result);
});

GFxRequest(GetIdleFavorites, {
	GetIdleFavorites(result);
});

GFxRequest(AddIdleFavorite, {
	AppendIdleFavorite(result);
});

GFxRequest(PlayIdleFavorite, {
	PlayIdleFavorite(result, args[1].GetString());
});

GFxRequest(AddPoseFavorite, {
	AppendPoseFavorite(result);
});

GFxRequest(ShowLooksMenu, {
	ShowLooksMenu(result);
});

GFxRequest(SetBoneDisplay, {
	SetBoneDisplay(result, args[0].GetBool());
});

GFxRequest(SelectNodeMarker, {
	SelectNodeMarker(result, args[0].GetString(), args[1].GetBool());
});

GFxRequest(UnselectNodeMarker, {
	UnselectNodeMarker(result);
});

GFxRequest(GetBoneFilter, {
	GetBoneFilter(result);
});

GFxRequest(SetBoneFilter, {
	SetBoneFilter(result, args[0].GetInt(), args[1].GetBool());
});

//GFxRequest(OverNodeMarker, {
//	OverNodeMarker(result, args[0].GetString());
//});
//
//GFxRequest(OutNodeMarker, {
//	OutNodeMarker(result, args[0].GetString());
//});

GFxRequest(UpdateCameraRotation, {
	UpdateCameraRotation(result, args[0].GetNumber(), args[1].GetNumber());
});

GFxRequest(UpdateCameraPan, {
	UpdateCameraPan(result, args[0].GetNumber(), args[1].GetNumber());
});

GFxRequest(UpdateCameraZoom, {
	UpdateCameraZoom(result, args[0].GetNumber());
});

GFxRequest(ClearBoneEdit, {
	ClearBoneEdit(result);
});

GFxRequest(StartBoneEdit, {
	StartBoneEdit(args[0].GetUInt(), args[1].GetString());
});

GFxRequest(EndBoneEdit, {
	EndBoneEdit(args[0].GetUInt(), args[1].GetString());
});

GFxRequest(UndoBoneEdit, {
	UndoBoneEdit(result);
});

GFxRequest(RedoBoneEdit, {
	RedoBoneEdit(result);
});

GFxRequest(SetWidget, {
	SetWidget(result, args[0].GetString(), args[1].GetBool());
});

GFxRequest(UpdateTransform, {
	UpdateTransform(args[0].GetUInt(), args[1].GetString(), args[2].GetInt(), args[3].GetNumber());
});

GFxFunction(IsFreeCamera, {
	result->SetBool(GetFreeCameraState());
});

GFxRequest(GetBodyMorphs, {
	GetBodyMorphs(result);
});

GFxRequest(SetBodyMorph, {
	SetBodyMorph(result, args[0].GetInt(), args[1].GetInt());
});

GFxRequest(SaveBodyslidePreset, {
	SaveBodyslidePreset(result, args[0].GetString());
});

GFxRequest(LoadBodyslidePreset, {
	LoadBodyslidePreset(result, args[0].GetString());
});

GFxRequest(LoadSliderSet, {
	LoadSliderSet(result, args[0].GetString());
});

GFxRequest(ResetBodyMorphs, {
	ResetBodyMorphs(result);
});

//NiMatrix43 scaleMatrix(float val, int type) {
//	switch (type) {
//	case 1: return { val, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0 };
//	case 2: return { 1, 0, 0, 0, 0, val, 0, 0, 0, 0, 1, 0 };
//	case 3: return { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, val, 0 };
//	}
//}
//
//void test(float val, int type) {
//	NiTransform t {
//		scaleMatrix(val, type),
//		{0, 0, 0},
//		1.0f
//	};
//	
//	UInt32 formId = selected.refr->formID;
//	auto node = SAF::NodeKey("HEAD", false);
//	auto adjustment = saf->GetAdjustment(formId, 1);
//	saf->SetTransform(adjustment, node, t);
//	saf->GetActorAdjustments(formId)->UpdateNode(node.name);
//}
//
//GFxFunction(Test, {
//	test(args[0].GetNumber(), args[1].GetInt());
//});

#define GFxRegister(T) RegisterFunction<T ## Scaleform>(value, view->movieRoot, #T)

bool RegisterScaleform(GFxMovieView* view, GFxValue* value)
{ 
	GFxRegister(GetMenu);
	RegisterFunction<LoadMenuScaleform>(value, view->movieRoot, "LoadMenu");
	GFxRegister(LoadMenuValue);
	GFxRegister(GetExtensions);
	GFxRegister(LoadExtension);
	GFxRegister(CallPapyrusForm);
	GFxRegister(GetPathStem);
	GFxRegister(GetPathRelative);
	GFxRegister(SaveState);
	GFxRegister(ClearState);
	GFxRegister(GetOptions);
	GFxRegister(SetOption);
	GFxRegister(GetFaceMorphs);
	GFxRegister(SetFaceMorph);
	GFxRegister(GetFaceMorphCategories);
	GFxRegister(SetFaceMorphCategory);
	GFxRegister(SaveFaceMorphs);
	GFxRegister(LoadFaceMorphs);
	GFxRegister(ResetFaceMorphs);
	GFxRegister(GetTongueBones);
	GFxRegister(SamPlaySound);
	GFxRegister(SamOpenMenu);
	GFxRegister(SamCloseMenu);
	GFxRegister(SamIsMenuOpen);
	GFxRegister(GetHacks);
	GFxRegister(SetHack);
	GFxRegister(GetEyes);
	GFxRegister(SetEyes);
	GFxRegister(GetAdjustmentScale);
	GFxRegister(SetAdjustmentScale);
	GFxRegister(GetAdjustments);
	GFxRegister(SaveAdjustment);
	GFxRegister(LoadAdjustment);
	GFxRegister(NewAdjustment);
	GFxRegister(RemoveAdjustment);
	GFxRegister(ResetAdjustment);
	GFxRegister(NegateBoneTransform);
	GFxRegister(GetAdjustmentNegate);
	GFxRegister(SetAdjustmentNegate);
	GFxRegister(MoveAdjustment);
	GFxRegister(RenameAdjustment);
	GFxRegister(SetLocalAdjustmentName);
	GFxRegister(GetBoneCategories);
	GFxRegister(GetBoneNames);
	GFxRegister(GetBoneInit);
	GFxRegister(GetNodeIsOffset);
	GFxRegister(ToggleNodeName);
	GFxRegister(GetBoneTransform);
	GFxRegister(SetBoneTransform);
	GFxRegister(RotateBoneTransform);
	GFxRegister(ResetBoneTransform);
	GFxRegister(GetIdleMods);
	GFxRegister(SetIdleMod);
	GFxRegister(GetIdles);
	GFxRegister(PlayIdle);
	GFxRegister(PlayAPose);
	GFxRegister(ResetIdle);
	GFxRegister(GetPoseAdjustments);
	GFxRegister(GetIdleName);
	GFxRegister(GetPoseName);
	GFxRegister(SavePose);
	GFxRegister(LoadPose);
	GFxRegister(ResetPose);
	GFxRegister(GetSkeletonAdjustments);
	GFxRegister(LoadSkeletonAdjustment);
	GFxRegister(ResetSkeletonAdjustment);
	GFxRegister(GetPositioning);
	GFxRegister(SetPositioning);
	GFxRegister(RotateIdle);
	GFxRegister(SaveObjectPosition);
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
	GFxRegister(SaveCameraFile);
	GFxRegister(LoadCameraPath);
	GFxRegister(SaveCameraState);
	GFxRegister(LoadCameraState);
	GFxRegister(GetLightSelect);
	GFxRegister(SetLightSelect);
	GFxRegister(GetLightEdit);
	GFxRegister(GetLightCategories);
	GFxRegister(GetLightForms);
	GFxRegister(GetLightsGlobal);
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
	GFxRegister(SetLightsGlobal);
	GFxRegister(UpdateAllLights);
	GFxRegister(ResetLightSettings);
	GFxRegister(DeleteAllLights);
	GFxRegister(SaveLights);
	GFxRegister(LoadLights);
	GFxRegister(GetExportTypes);
	GFxRegister(MergeAdjustmentDown);
	//GFxRegister(MirrorAdjustment);
	GFxRegister(OpenActorContainer);
	GFxRegister(GetItemMods);
	GFxRegister(GetItemGroups);
	GFxRegister(GetItemList);
	GFxRegister(AddItem);
	GFxRegister(EquipSearchedItem);
	GFxRegister(GetLastSearchResult);
	GFxRegister(SearchItems);
	GFxRegister(GetMatSwapEquipment);
	GFxRegister(GetMatSwaps);
	GFxRegister(ApplyMatSwap);
	GFxRegister(GetEquipment);
	GFxRegister(RemoveEquipment);
	GFxRegister(RemoveAllEquipment);
	GFxRegister(GetIdleFavorites);
	GFxRegister(AddIdleFavorite);
	GFxRegister(PlayIdleFavorite);
	GFxRegister(AddPoseFavorite);
	GFxRegister(ShowLooksMenu);
	GFxRegister(SetBoneDisplay);
	GFxRegister(SelectNodeMarker);
	GFxRegister(UnselectNodeMarker);
	GFxRegister(GetBoneFilter);
	GFxRegister(SetBoneFilter);
	//GFxRegister(OverNodeMarker);
	//GFxRegister(OutNodeMarker);
	GFxRegister(UpdateCameraRotation);
	GFxRegister(UpdateCameraPan);
	GFxRegister(UpdateCameraZoom);
	GFxRegister(ClearBoneEdit);
	GFxRegister(UndoBoneEdit);
	GFxRegister(RedoBoneEdit);
	GFxRegister(StartBoneEdit);
	GFxRegister(EndBoneEdit);
	GFxRegister(SetWidget);
	GFxRegister(UpdateTransform);
	GFxRegister(IsFreeCamera);
	GFxRegister(GetBodyMorphs);
	GFxRegister(SetBodyMorph);
	GFxRegister(SaveBodyslidePreset);
	GFxRegister(LoadBodyslidePreset);
	GFxRegister(ResetBodyMorphs);
	GFxRegister(LoadSliderSet);
	
	//GFxRegister(Test);

	return true;
}