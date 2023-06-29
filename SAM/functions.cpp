#include "functions.h"

#include "hacks.h"
#include "papyrus.h"
#include "pose.h"
#include "mfg.h"
#include "idle.h"
#include "positioning.h"
#include "compatibility.h"
#include "options.h"
#include "camera.h"
#include "lights.h"
#include "inventory.h"
#include "eyes.h"
#include "io.h"
#include "bodymorphs.h"
#include "looks.h"
#include "scaleform.h"
#include "constants.h"
#include "coc.h"

#include <filesystem>

GFxFunctions samFunctions;

//menu
GFxReq getMenu("GetMenu", [](auto& result, auto args) {
	GetMenuData(result, args->args[0].GetString());
});
GFxReq getExtensions("GetExtensions", [](auto& result, auto args) {
	GetExtensionMenus(result);
});
GFxFunc loadMenu("LoadMenu", [](auto args) {
	samManager.PushMenu(args->args[0].GetString());
});
GFxFunc loadMenuValue("LoadMenuValue", [](auto args) {
	samManager.PushMenu(args->args[1].GetString());
});
GFxReq loadExtension("LoadExtension", [](auto& result, auto args) {
	samManager.PushMenu(args->args[1].GetString());
});
GFxFunc saveState("SaveState", [](auto args) {
	samManager.SaveData(&args->args[0]);
});
GFxFunc clearState("ClearState", [](auto args) {
	samManager.ClearData();
});
GFxFunc closeMenu("CloseMenu", [](auto args) {
	samManager.CloseMenu();
});
GFxFunc playSound("PlaySound", [](auto args) {
	PlayUISound(args->args[0].GetString());
});
GFxReq setWidget("SetWidget", [](auto& result, auto args) {
	SetWidget(result, args->args[0].GetString(), args->args[1].GetBool());
});
GFxReq callPapyrusForm("CallPapyrusForm", [](auto& result, auto args) {
	CallPapyrusForm(result, args->args[0].GetString(), args->args[1].GetString(), args->args[2]);
});
GFxReq getPathStem("GetPathStem", [](auto& result, auto args) {
	result.SetManagedString(result.root, std::filesystem::path(args->args[0].GetString()).stem().string().c_str());
});
GFxReq getPathRelative("GetPathRelative", [](auto& result, auto args) {
	std::string relative =
		GetRelativePath(strlen(args->args[0].GetString()), strlen(args->args[1].GetString()), args->args[2].GetString());
	result.SetManagedString(result.root, relative.c_str());
});
GFxReq getOptions("GetOptions", [](auto& result, auto args) {
	GetMenuOptions(result);
});
GFxReq setOption("SetOption", [](auto& result, auto args) {
	SetMenuOption(result, args->args[0].GetInt(), args->args[1].GetBool());
});
GFxReq getFolder("GetFolder", [](auto& result, auto args) {
	GetFolder(result, args->args[0].GetString(), args->args[1].GetString());
});
GFxFunc setCursorVisible("SetCursorVisible", [](auto args) {
	BSFixedString cursorMenu("CursorMenu");
	SetMenuVisible(cursorMenu, "root1.Cursor_mc.visible", args->args[0].GetBool());
});
GFxReq getCursorPosition("GetCursorPosition", [](auto& result, auto args) {
	GetCursorPosition(result);
});
GFxFunc setCursorPosition("SetCursorPosition", [](auto args) {
	SetCursor(args->args[0].GetInt(), args->args[1].GetInt());
});
GFxFunc getLock("GetLock", [](auto args) {
	args->result->SetBool(GetFfcLock((FfcType)args->args[0].GetInt()));
});
GFxFunc toggleMenus("ToggleMenus", [](auto args) {
	args->result->SetBool(ToggleMenusHidden());
});
GFxReq setBoneDisplay("SetBoneDisplay", [](auto& result, auto args) {
	SetBoneDisplay(result, args->args[0].GetBool());
});
GFxReq selectNodeMarker("SelectNodeMarker", [](auto& result, auto args) {
	SelectNodeMarker(result, args->args[0].GetString(), args->args[1].GetBool());
});
GFxReq unselectNodeMarker("UnselectNodeMarker", [](auto& result, auto args) {
	UnselectNodeMarker(result);
});
GFxReq getBoneFilter("GetBoneFilter", [](auto& result, auto args) {
	GetBoneFilter(result);
});
GFxReq setBoneFilter("SetBoneFilter", [](auto& result, auto args) {
	SetBoneFilter(result, args->args[0].GetInt(), args->args[1].GetBool());
});
GFxFunc isFreeCamera("IsFreeCamera", [](auto args) {
	args->result->SetBool(menuOptions.cameracontrol && GetFreeCameraState());
});
GFxFunc filterMenu("FilterMenu", [](auto args) {
	FilterMenuNamesBySubstring(args->movie->movieRoot, &args->args[0], args->args[1].GetString(), args->result);
});
GFxFunc getTranslation("GetTranslation", [](auto args) {
	GetTranslation(args->movie->movieRoot, args->result, args->args[0].GetString());
});
GFxFunc getTranslations("GetTranslations", [](auto args) {
	GetTranslations(args->movie->movieRoot, &args->args[0]);
});

//hacks
GFxReq getHacks("GetHacks", [](auto& result, auto args) {
	GetHacks(result);
});
GFxReq setHack("SetHack", [](auto& result, auto args) {
	SetHack(args->args[0].GetInt(), args->args[1].GetBool());
});

//mfg
GFxReq getFaceMorphs("GetFaceMorphs", [](auto& result, auto args) {
	GetMorphs(result, args->args[0].GetInt());
});
GFxReq setFaceMorph("SetFaceMorph", [](auto& result, auto args) {
	SetFaceMorph(args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
});
GFxReq getFaceMorphCategories("GetFaceMorphCategories", [](auto& result, auto args) {
	GetMorphCategories(result);
});
GFxReq setFaceMorphCategory("SetFaceMorphCategory", [](auto& result, auto args) {
	SetFaceMorphCategory(result, args->args[0].GetInt(), args->args[1].GetUInt());
});
GFxReq saveFaceMorphs("SaveFaceMorphs", [](auto& result, auto args) {
	SaveMfg(args->args[0].GetString());
});
GFxReq loadFaceMorphs("LoadFaceMorphs", [](auto& result, auto args) {
	LoadMfgPath(args->args[0].GetString());
});
GFxReq resetFaceMorphs("ResetFaceMorphs", [](auto& result, auto args) {
	ResetMfg();
});
GFxReq getTongueBones("GetTongueBones", [](auto& result, auto args) {
	GetMorphsTongueNodes(result);
});

//eyes
GFxReq getEyes("GetEyes", [](auto& result, auto args) {
	GetEyes(result);
});
GFxReq setEyes("SetEyes", [](auto& result, auto args) {
	SetEyes(result, args->args[2].GetNumber(), args->args[3].GetNumber());
});

//adjustments
GFxReq getAdjustmentScale("GetAdjustmentScale", [](auto& result, auto args) {
	GetAdjustmentScale(result, args->args[0].GetUInt());
});
GFxReq setAdjustmentScale("SetAdjustmentScale", [](auto& result, auto args) {
	SetAdjustmentScale(result, args->args[2].GetUInt(), args->args[1].GetUInt());
});
GFxReq getAdjustments("GetAdjustments", [](auto& result, auto args) {
	GetAdjustments(result);
});
GFxReq getBoneCategories("GetBoneCategories", [](auto& result, auto args) {
	GetBoneCategories(result);
});
GFxReq getBoneNames("GetBoneNames", [](auto& result, auto args) {
	GetBoneNames(result, args->args[0].GetInt());
});
GFxReq getBoneTransform("GetBoneTransform", [](auto& result, auto args) {
	GetBoneTransform(result, args->args[1].GetString(), args->args[0].GetUInt());
});
GFxReq setBoneTransform("SetBoneTransform", [](auto& result, auto args) {
	//index, value, handle, key, yaw, pitch, roll
	SetBoneTransform(result, args->args[0].GetInt(), args->args[1].GetNumber(), args->args[2].GetUInt(), args->args[3].GetString(),
	args->args[4].GetNumber(), args->args[5].GetNumber(), args->args[6].GetNumber());
});
GFxFunc getNodeIsOffset("GetNodeIsOffset", [](auto args) {
	args->result->SetBool(GetNodeIsOffset(args->args[0].GetString()));
});
GFxFunc getBoneInit("GetBoneInit", [](auto args) {
	args->result->SetString(GetBoneInit(args->args[0].GetString()));
});
GFxFunc toggleNodeName("ToggleNodeName", [](auto args) {
	ToggleNodeName(args->result, args->args[0].GetString());
});
GFxReq rotateBoneTransform("RotateBoneTransform", [](auto& result, auto args) {
	RotateAdjustmentXYZ(args->args[3].GetString(), args->args[2].GetUInt(), args->args[0].GetInt() - 6, args->args[1].GetNumber());
});
GFxReq resetBoneTransform("ResetBoneTransform", [](auto& result, auto args) {
	ResetAdjustmentTransform(args->args[1].GetString(), args->args[0].GetUInt());
});
GFxReq updateTransform("UpdateTransform", [](auto& result, auto args) {
	UpdateTransform(args->args[0].GetUInt(), args->args[1].GetString(), args->args[2].GetInt(), args->args[3].GetNumber());
});
GFxReq saveAdjustment("SaveAdjustment", [](auto& result, auto args) {
	SaveAdjustmentFile(result, args->args[0].GetString(), args->args[1].GetUInt());
});
GFxReq loadAdjustment("LoadAdjustment", [](auto& result, auto args) {
	LoadAdjustmentPathGFx(result, args->args[0].GetString());
});
GFxReq newAdjustment("NewAdjustment", [](auto& result, auto args) {
	PushNewAdjustment("New Adjustment");
});
GFxReq removeAdjustment("RemoveAdjustment", [](auto& result, auto args) {
	EraseAdjustment(args->args[1].GetUInt());
});
GFxReq resetAdjustment("ResetAdjustment", [](auto& result, auto args) {
	ClearAdjustment(args->args[2].GetInt());
});
GFxReq negateBoneTransform("NegateBoneTransform", [](auto& result, auto args) {
	NegateTransform(args->args[1].GetString(), args->args[0].GetUInt());
});
GFxReq getAdjustmentNegate("GetAdjustmentNegate", [](auto& result, auto args) {
	GetAdjustmentNegate(result);
});
GFxReq setAdjustmentNegate("SetAdjustmentNegate", [](auto& result, auto args) {
	SetAdjustmentNegate(result, args->args[1].GetString(), args->args[2].GetUInt());
});
GFxFunc moveAdjustment("MoveAdjustment", [](auto args) {
	args->result->SetBool(ShiftAdjustment(args->args[0].GetUInt(), args->args[1].GetBool()));
});
GFxReq renameAdjustment("RenameAdjustment", [](auto& result, auto args) {
	SetAdjustmentName(result, args->args[1].GetUInt(), args->args[0].GetString());
});
GFxReq setLocalAdjustmentName("SetLocalAdjustmentName", [](auto& result, auto args) {
	SetLocalAdjustmentName(args->args[0].GetUInt());
});
GFxReq getPoseName("GetPoseName", [](auto& result, auto args) {
	result.SetManagedString(args->movie->movieRoot, GetCurrentPoseName());
});
GFxReq getPoseAdjustments("GetPoseAdjustments", [](auto& result, auto args) {
	GetPoseAdjustments(result);
});
GFxReq savePose("SavePose", [](auto& result, auto args) {
	SaveJsonPose(args->args[0].GetString(), args->args[1], args->args[2].GetUInt());
});
GFxReq loadPose("LoadPose", [](auto& result, auto args) {
	LoadPoseGFx(result, args->args[0].GetString());
});
GFxReq resetPose("ResetPose", [](auto& result, auto args) {
	ResetJsonPose();
});
GFxReq playAPose("PlayAPose", [](auto& result, auto args) {
	PlayAPose();
});
GFxReq getSkeletonAdjustments("GetSkeletonAdjustments", [](auto& result, auto args) {
	GetSkeletonAdjustments(result, args->args[0].GetString(), args->args[1].GetString(), args->args[2].GetBool());
});
GFxReq loadSkeletonAdjustment("LoadSkeletonAdjustment", [](auto& result, auto args) {
	LoadSkeletonAdjustment(result, args->args[0].GetString(), args->args[1].GetBool(), args->args[2].GetBool(), args->args[3].GetBool());
});
GFxReq resetSkeletonAdjustment("ResetSkeletonAdjustment", [](auto& result, auto args) {
	LoadSkeletonAdjustment(result, nullptr, false, false, args->args[0].GetBool());
});
GFxReq getExportTypes("GetExportTypes", [](auto& result, auto args) {
	GetPoseExportTypes(result);
});
GFxReq mergeAdjustmentDown("MergeAdjustmentDown", [](auto& result, auto args) {
	MergeAdjustment(result, args->args[2].GetUInt());
});
//GFxReq mirrorAdjustment("MirrorAdjustment", [](auto& result, auto args) {
//	MirrorAdjustment(result, args[2].GetUInt());
//});
GFxReq addPoseFavorite("AddPoseFavorite", [](auto& result, auto args) {
	AppendPoseFavorite(result);
});
GFxReq clearBoneEdit("ClearBoneEdit", [](auto& result, auto args) {
	ClearBoneEdit(result);
});
GFxReq startBoneEdit("StartBoneEdit", [](auto& result, auto args) {
	StartBoneEdit(args->args[0].GetUInt(), args->args[1].GetString());
});
GFxReq endBoneEdit("EndBoneEdit", [](auto& result, auto args) {
	EndBoneEdit(args->args[0].GetUInt(), args->args[1].GetString());
});
GFxReq undoBoneEdit("UndoBoneEdit", [](auto& result, auto args) {
	UndoBoneEdit(result);
});
GFxReq redoBoneEdit("RedoBoneEdit", [](auto& result, auto args) {
	RedoBoneEdit(result);
});

//idle
GFxReq getIdleMods("GetIdleMods", [](auto& result, auto args) {
	GetIdleMenuCategories(result);
});
GFxReq setIdleMod("SetIdleMod", [](auto& result, auto args) {
	SetIdleMod(result, args->args[1].GetInt());
});
GFxReq getIdles("GetIdles", [](auto& result, auto args) {
	GetIdleMenu(result, args->args[0].GetInt());
});
GFxReq playIdle("PlayIdle", [](auto& result, auto args) {
	PlayIdleAnimation(args->args[1].GetUInt());
});
GFxReq resetIdle("ResetIdle", [](auto& result, auto args) {
	ResetIdleAnimation();
	ResetJsonPose();
});
GFxReq getIdleName("GetIdleName", [](auto& result, auto args) {
	result.SetString(GetCurrentIdleName());
});
GFxReq getIdleFavorites("GetIdleFavorites", [](auto& result, auto args) {
	GetIdleFavorites(result);
});
GFxReq addIdleFavorite("AddIdleFavorite", [](auto& result, auto args) {
	AppendIdleFavorite(result);
});
GFxReq removeIdleFavorite("RemoveIdleFavorite", [](auto& result, auto args) {
	RemoveIdleFavorite(result, args->args[0].GetInt());
});
GFxReq playIdleFavorite("PlayIdleFavorite", [](auto& result, auto args) {
	PlayIdleFavorite(result, args->args[1].GetString());
});

//positioning
GFxReq saveObjectPosition("SaveObjectPosition", [](auto& result, auto args) {
	SaveObjectTranslation();
});
GFxReq getPositioning("GetPositioning", [](auto& result, auto args) {
	GetPositioning(result);
});
GFxReq setPositioning("SetPositioning", [](auto& result, auto args) {
	AdjustObjectPosition(result, args->args[0].GetInt(), args->args[1], true);
});
GFxReq rotateIdle("RotateIdle", [](auto& result, auto args) {
	AdjustObjectPosition(result, kAdjustRotationZ, args->args[0], false);
});
GFxReq selectPositioning("SelectPositioning", [](auto& result, auto args) {
	SelectPositioningMenuOption(args->args[0].GetInt());
});
GFxReq resetPositioning("ResetPositioning", [](auto& result, auto args) {
	SetDefaultObjectTranslation();
});

//camera
GFxReq getCamera("GetCamera", [](auto& result, auto args) {
	GetCamera(result);
});
GFxReq setCamera("SetCamera", [](auto& result, auto args) {
	SetCamera(result, args->args[0].GetInt(), args->args[1].GetNumber());
});
GFxReq saveCameraFile("SaveCameraFile", [](auto& result, auto args) {
	if (!SaveCameraFile(args->args[0].GetString()))
		return result.SetError(CAMERA_ERROR); //TODO need better error messaging
});
GFxReq loadCameraPath("LoadCameraPath", [](auto& result, auto args) {
	LoadCameraGFx(result, args->args[0].GetString());
});
GFxReq saveCameraState("SaveCameraState", [](auto& result, auto args) {
	SaveCameraState(args->args[1].GetInt());
});
GFxReq loadCameraState("LoadCameraState", [](auto& result, auto args) {
	LoadCameraState(args->args[1].GetInt());
});
GFxReq updateCameraRotation("UpdateCameraRotation", [](auto& result, auto args) {
	UpdateCameraRotation(result, args->args[0].GetNumber(), args->args[1].GetNumber());
});
GFxReq updateCameraPan("UpdateCameraPan", [](auto& result, auto args) {
	UpdateCameraPan(result, args->args[0].GetNumber(), args->args[1].GetNumber());
});
GFxReq updateCameraZoom("UpdateCameraZoom", [](auto& result, auto args) {
	UpdateCameraZoom(result, args->args[0].GetNumber());
});

//lights
GFxReq getLightSelect("GetLightSelect", [](auto& result, auto args) {
	GetLightSelect(result);
});
GFxReq setLightSelect("SetLightSelect", [](auto& result, auto args) {
	SetLightSelect(result, args->args[1].GetInt());
});
GFxReq getLightEdit("GetLightEdit", [](auto& result, auto args) {
	GetLightEdit(result, args->args[0].GetInt());
});
GFxReq getLightCategories("GetLightCategories", [](auto& result, auto args) {
	GetLightCategories(result);
});
GFxReq getLightForms("GetLightForms", [](auto& result, auto args) {
	GetLightForms(result, args->args[0].GetInt());
});
GFxReq createLight("CreateLight", [](auto& result, auto args) {
	CreateLight(result, args->args[1].GetUInt());
});
GFxReq addLight("AddLight", [](auto& result, auto args) {
	AddLight(result);
});
GFxReq editLight("EditLight", [](auto& result, auto args) {
	EditLight(result, args->args[0].GetUInt(), args->args[1].GetNumber(), args->args[2].GetInt());
});
GFxReq swapLight("SwapLight", [](auto& result, auto args) {
	SwapLight(result, args->args[1].GetUInt(), args->args[2].GetInt());
});
GFxReq deleteLight("DeleteLight", [](auto& result, auto args) {
	DeleteLight(result, args->args[2].GetInt());
});
GFxReq resetLight("ResetLight", [](auto& result, auto args) {
	ResetLight(result, args->args[0].GetInt());
});
GFxReq renameLight("RenameLight", [](auto& result, auto args) {
	RenameLight(result, args->args[0].GetString(), args->args[1].GetInt());
});
GFxFunc getLightVisible("GetLightVisible", [](auto args) {
	args->result->SetBool(GetLightVisible(args->args[0].GetInt()));
});
GFxFunc toggleLightVisible("ToggleLightVisible", [](auto args) {
	args->result->SetBool(ToggleLightVisible(args->args[0].GetInt()));
});
GFxFunc getAllLightsVisible("GetAllLightsVisible", [](auto args) {
	args->result->SetBool(GetAllLightsVisible());
});
GFxFunc toggleAllLightsVisible("ToggleAllLightsVisible", [](auto args) {
	args->result->SetBool(ToggleAllLightsVisible());
});
GFxReq getLightsGlobal("GetLightsGlobal", [](auto& result, auto args) {
	GetLightSettings(result);
});
GFxReq setLightsGlobal("SetLightsGlobal", [](auto& result, auto args) {
	EditLightSettings(result, args->args[0].GetUInt(), args->args[1].GetNumber());
});
GFxReq resetLightSettings("ResetLightSettings", [](auto& result, auto args) {
	ResetLightSettings();
});
GFxReq updateAllLights("UpdateAllLights", [](auto& result, auto args) {
	UpdateAllLights();
});
GFxReq deleteAllLights("DeleteAllLights", [](auto& result, auto args) {
	DeleteAllLights();
});
GFxReq saveLights("SaveLights", [](auto& result, auto args) {
	SaveLightsJson(args->args[0].GetString());
});
GFxReq loadLights("LoadLights", [](auto& result, auto args) {
	LoadLightsPath(args->args[0].GetString());
});

//inventory
GFxReq openActorContainer("OpenActorContainer", [](auto& result, auto args) {
	OpenActorContainer(result);
});
GFxReq getItemMods("GetItemMods", [](auto& result, auto args) {
	GetItemMods(result);
});
GFxReq getItemGroups("GetItemGroups", [](auto& result, auto args) {
	GetItemGroups(result, args->args[0].GetString());
});
GFxReq getItemList("GetItemList", [](auto& result, auto args) {
	GetItemList(result, args->args[0].GetString(), args->args[1].GetInt());
});
GFxReq addItem("AddItem", [](auto& result, auto args) {
	AddItem(result, args->args[1].GetUInt(), args->args[2].GetBool());
});
GFxReq equipSearchedItem("EquipSearchedItem", [](auto& result, auto args) {
	AddItem(result, args->args[1].GetUInt(), !args->args[2].GetBool());
});
GFxReq getStaticMods("GetStaticMods", [](auto& result, auto args) {
	GetStaticMods(result);
});
GFxReq getStaticGroups("GetStaticGroups", [](auto& result, auto args) {
	GetStaticGroups(result, args->args[0].GetString());
});
GFxReq getStaticItems("GetStaticItems", [](auto& result, auto args) {
	GetStaticItems(result, args->args[0].GetString(), args->args[1].GetInt());
});
GFxReq getLastSearchResult("GetLastSearchResult", [](auto& result, auto args) {
	GetLastSearchResult(result);
});
GFxReq getLastSearchResultStatic("GetLastSearchResultStatic", [](auto& result, auto args) {
	GetLastSearchResultStatic(result);
});
GFxReq searchItems("SearchItems", [](auto& result, auto args) {
	SearchItems(result, args->args[0].GetString());
});
GFxReq searchStatics("SearchStatics", [](auto& result, auto args) {
	SearchStatics(result, args->args[0].GetString());
});
GFxReq getMatSwapEquipment("GetMatSwapEquipment", [](auto& result, auto args) {
	GetMatSwapEquipment(result);
});
GFxReq getMatSwaps("GetMatSwaps", [](auto& result, auto args) {
	GetMatSwaps(result, args->args[0].GetUInt());
});
GFxReq applyMatSwap("ApplyMatSwap", [](auto& result, auto args) {
	ApplyMatSwap(result, args->args[1].GetUInt(), args->args[2].GetUInt());
});
GFxReq getEquipment("GetEquipment", [](auto& result, auto args) {
	GetEquipment(result);
});
GFxReq removeEquipment("RemoveEquipment", [](auto& result, auto args) {
	RemoveEquipment(result, args->args[1].GetUInt());
});
GFxReq removeAllEquipment("RemoveAllEquipment", [](auto& result, auto args) {
	RemoveAllEquipment(result);
});

//bodymorphs
GFxReq getBodyMorphs("GetBodyMorphs", [](auto& result, auto args) {
	GetBodyMorphs(result);
});
GFxReq setBodyMorph("SetBodyMorph", [](auto& result, auto args) {
	SetBodyMorph(result, args->args[0].GetInt(), args->args[1].GetInt());
});
GFxReq saveBodyslidePreset("SaveBodyslidePreset", [](auto& result, auto args) {
	SaveBodyslidePreset(result, args->args[0].GetString());
});
GFxReq loadBodyslidePreset("LoadBodyslidePreset", [](auto& result, auto args) {
	LoadBodyslidePreset(result, args->args[0].GetString());
});
GFxReq loadSliderSet("LoadSliderSet", [](auto& result, auto args) {
	LoadSliderSet(result, args->args[0].GetString());
});
GFxReq resetBodyMorphs("ResetBodyMorphs", [](auto& result, auto args) {
	ResetBodyMorphs(result);
});

//coc
GFxReq getCellMods("GetCellMods", [](auto& result, auto args) {
	GetCellMods(result);
});
GFxReq getCells("GetCells", [](auto& result, auto args) {
	GetCells(result, args->args[0].GetString());
});
GFxReq setCell("SetCell", [](auto& result, auto args) {
	SetCell(result, args->args[1].GetUInt());
});
GFxReq setCellEdid("SetCellEdid", [](auto& result, auto args) {
	SetCellEdid(result, args->args[1].GetString());
});
GFxReq getWorldspaceMods("GetWorldspaceMods", [](auto& result, auto args) {
	GetWorldspaceMods(result);
});
GFxReq getWorldspacesFromMod("GetWorldspacesFromMod", [](auto& result, auto args) {
	GetWorldspacesFromMod(result, args->args[0].GetString());
});
GFxReq getWorldspaceCells("GetWorldspaceCells", [](auto& result, auto args) {
	GetWorldspaceCells(result, args->args[0].GetString(), args->args[1].GetUInt());
});
GFxReq getLastSearchResultCell("GetLastSearchResultCell", [](auto& result, auto args) {
	GetLastSearchResultCell(result);
});
GFxReq searchCoc("SearchCells", [](auto& result, auto args) {
	SearchCells(result, args->args[0].GetString());
});
GFxReq getCellName("GetCellName", [](auto& result, auto args) {
	result.SetString(GetCurrentDisplayedCell());
});
GFxReq getCellFavorites("GetCellFavorites", [](auto& result, auto args) {
	GetCellFavorites(result);
});
GFxReq appendCellFavorite("AddCellFavorite", [](auto& result, auto args) {
	AppendCellFavorite(result);
});
GFxReq removeCellFavorite("RemoveCellFavorite", [](auto& result, auto args) {
	RemoveCellFavorite(result, args->args[0].GetInt());
});