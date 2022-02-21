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

#include "f4se_common/SafeWrite.h"

#include "sam.h"
#include "hacks.h"
#include "papyrus.h"
#include "eyes.h"
#include "pose.h"
#include "mfg.h"
#include "idle.h"
#include "SAF/util.h"

void ModifyFacegenMorph::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[2].GetType() == GFxValue::kType_Int);
	SetFaceMorph(args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
}

void GetMorphCategories::Invoke(Args* args)
{
	GetMorphCategoriesGFx(args->movie->movieRoot, args->result);
}

void GetMorphs::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	GetMorphsGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
}

void SavePreset::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	SaveMfg(args->args[0].GetString());
}

void LoadPreset::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	LoadMfg(args->args[0].GetString());
	GetMorphsGFx(args->movie->movieRoot, args->result, args->args[1].GetInt());
}

void ResetMorphs::Invoke(Args * args)
{
	ResetMfg();
}

void SamPlaySound::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	PlayUISound(args->args[0].GetString());
}

void SamOpenMenu::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	BSFixedString menuName(args->args[0].GetString());
	CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Open);
}

void SamCloseMenu::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	BSFixedString menuName(args->args[0].GetString());
	CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
}

void SamIsMenuOpen::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	args->result->SetBool((*g_ui)->IsMenuOpen(args->args[0].GetString()));
}

void GetHacks::Invoke(Args * args)
{
	GetHacksGFx(args->movie->movieRoot, args->result);
}

void GetBlinkHack::Invoke(Args * args)
{
	args->result->SetInt(GetBlinkState());
}

void SetBlinkHack::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Bool);
	SetBlinkState(args->args[0].GetBool());
}

void GetMorphHack::Invoke(Args * args)
{
	args->result->SetInt(GetForceMorphUpdate());
}

void SetMorphHack::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Bool);
	SetForceMorphUpdate(args->args[0].GetBool());
}

void GetEyeTrackingHack::Invoke(Args * args)
{
	args->result->SetInt(GetDisableEyecoordUpdate());
}

void SetEyeTrackingHack::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Bool);
	SetDisableEyecoordUpdate(args->args[0].GetBool());
}

void GetEyeCoords::Invoke(Args * args)
{
	args->movie->movieRoot->CreateArray(args->result);
	float coords[2];
	if (GetEyecoords(coords)) {
		args->result->PushBack(&GFxValue(coords[0] * 4));
		args->result->PushBack(&GFxValue(coords[1] * 5));
	} else {
		args->result->PushBack(&GFxValue(0.0));
		args->result->PushBack(&GFxValue(0.0));
	}
}

void SetEyeCoords::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Number);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Number);
	SetEyecoords(args->args[0].GetNumber() / 4, args->args[1].GetNumber() / 5);
}

void GetAdjustment::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	GetAdjustmentGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
}

void SetAdjustmentPersistence::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Bool);
	SetPersistence(args->args[0].GetInt(), args->args[1].GetBool());
}

void SetAdjustmentScale::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	SetScale(args->args[0].GetInt(), args->args[1].GetInt());
}

void GetAdjustmentList::Invoke(Args* args)
{
	GetAdjustmentsGFx(args->movie->movieRoot, args->result);
}

void GetCategoryList::Invoke(Args * args)
{
	GetCategoriesGFx(args->movie->movieRoot, args->result);
}

void GetNodeList::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	GetNodesGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
}

void GetNodeTransform::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 3);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[2].GetType() == GFxValue::kType_Int);
	GetTransformGFx(args->movie->movieRoot, args->result, args->args[0].GetInt(), args->args[1].GetInt(), args->args[2].GetInt());
}

void SetNodePosition::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 5);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[2].GetType() == GFxValue::kType_Number);
	ASSERT(args->args[3].GetType() == GFxValue::kType_Number);
	ASSERT(args->args[4].GetType() == GFxValue::kType_Number);

	SetAdjustmentPos(args->args[0].GetString(), args->args[1].GetInt(), args->args[2].GetNumber(), args->args[3].GetNumber(), args->args[4].GetNumber());
}


void SetNodeRotation::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 5);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[2].GetType() == GFxValue::kType_Number);
	ASSERT(args->args[3].GetType() == GFxValue::kType_Number);
	ASSERT(args->args[4].GetType() == GFxValue::kType_Number);
	//heading attitude bank, y z x
	SetAdjustmentRot(args->args[0].GetString(), args->args[1].GetInt(), args->args[3].GetNumber(), args->args[4].GetNumber(), args->args[2].GetNumber());
}


void SetNodeScale::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 3);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[2].GetType() == GFxValue::kType_Number);
	SetAdjustmentSca(args->args[0].GetString(), args->args[1].GetInt(), args->args[2].GetNumber());
}

void ResetTransform::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	ASSERT(args->args[1].GetType() == GFxValue::kType_Int);
	ResetAdjustmentTransform(args->args[0].GetString(), args->args[1].GetInt());
}

void SaveAdjustment::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 2);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	ASSERT(args->args[1].GetType() == GFxValue::kType_String);

	SaveAdjustmentFile(args->args[1].GetString(), args->args[0].GetInt());
}

void LoadAdjustment::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_String);
	LoadAdjustmentFile(args->args[0].GetString());
}

void NewAdjustment::Invoke(Args * args)
{
	PushNewAdjustment("New Adjustment");
	GetAdjustmentsGFx(args->movie->movieRoot, args->result);
}

void RemoveAdjustment::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	EraseAdjustment(args->args[0].GetInt());
}

void ResetAdjustment::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	ClearAdjustment(args->args[0].GetInt());
}

void GetIdleCategories::Invoke(Args* args)
{
	GetIdleMenuCategoriesGFx(args->movie->movieRoot, args->result);
}

void GetIdles::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Int);
	GetIdleMenuGFx(args->movie->movieRoot, args->result, args->args[0].GetInt());
}

void PlayIdle::Invoke(Args* args)
{
	ASSERT(args->numArgs >= 1);
	ASSERT(args->args[0].GetType() == GFxValue::kType_UInt);
	PlayIdleAnimation(args->args[0].GetUInt());
}

void ResetIdle::Invoke(Args* args)
{
	ResetIdleAnimation();
}

void HideMenu::Invoke(Args * args)
{
	ASSERT(args->numArgs >= 4);
	ASSERT(args->args[0].GetType() == GFxValue::kType_Bool);
	ASSERT(args->args[1].GetType() == GFxValue::kType_String);
	ASSERT(args->args[2].GetType() == GFxValue::kType_UInt);
	ASSERT(args->args[3].GetType() == GFxValue::kType_UInt);
	UInt64 high = args->args[2].GetUInt();
	UInt64 low = args->args[3].GetUInt();
	UInt64 handle = (high << 32) | low;
	bool hide = args->args[0].GetBool();
	SetMenuMovement(hide, args->args[1].GetString(), handle);
	static BSFixedString cursorMenu("CursorMenu");
	SetMenuVisible(cursorMenu, "root1.Cursor_mc.visible", !hide);
}

#include "SAF/adjustments.h"

void Test::Invoke(Args * args)
{
	if (!selected.refr) return;
	std::shared_ptr<SAF::ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<SAF::Adjustment> adjustment = adjustments->GetAdjustment(1);
	if (!adjustment) return;

	SAF::NodeSet overrides = adjustments->nodeSets->overrides;
	for (auto& name : overrides) {
		adjustments->NegateTransform(adjustment, name);
	}
	adjustments->UpdateAllAdjustments(adjustment);
}

void Test2::Invoke(Args* args)
{
	if (!selected.refr) return;
	std::shared_ptr<SAF::ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<SAF::Adjustment> adjustment = adjustments->GetAdjustment(1);
	if (!adjustment) return;

	SAF::NodeSet overrides = adjustments->nodeSets->overrides;
	for (auto& name : overrides) {
		adjustments->NegateTransform2(adjustment, name);
	}
	adjustments->UpdateAllAdjustments(adjustment);
}

bool RegisterScaleform(GFxMovieView* view, GFxValue* value)
{ 
	RegisterFunction<ModifyFacegenMorph>(value, view->movieRoot, "ModifyFacegenMorph");
	RegisterFunction<GetMorphCategories>(value, view->movieRoot, "GetMorphCategories");
	RegisterFunction<GetMorphs>(value, view->movieRoot, "GetMorphs");
	RegisterFunction<SavePreset>(value, view->movieRoot, "SaveMorphPreset");
	RegisterFunction<LoadPreset>(value, view->movieRoot, "LoadMorphPreset");
	RegisterFunction<ResetMorphs>(value, view->movieRoot, "ResetMorphs");
	RegisterFunction<SamPlaySound>(value, view->movieRoot, "PlaySound");
	RegisterFunction<SamOpenMenu>(value, view->movieRoot, "OpenMenu");
	RegisterFunction<SamCloseMenu>(value, view->movieRoot, "CloseMenu");
	RegisterFunction<SamIsMenuOpen>(value, view->movieRoot, "IsMenuOpen");
	RegisterFunction<GetHacks>(value, view->movieRoot, "GetHacks");
	RegisterFunction<GetBlinkHack>(value, view->movieRoot, "GetBlinkHack");
	RegisterFunction<SetBlinkHack>(value, view->movieRoot, "SetBlinkHack");
	RegisterFunction<GetMorphHack>(value, view->movieRoot, "GetMorphHack");
	RegisterFunction<SetMorphHack>(value, view->movieRoot, "SetMorphHack");
	RegisterFunction<GetEyeTrackingHack>(value, view->movieRoot, "GetEyeTrackingHack");
	RegisterFunction<SetEyeTrackingHack>(value, view->movieRoot, "SetEyeTrackingHack");
	RegisterFunction<GetEyeCoords>(value, view->movieRoot, "GetEyeCoords");
	RegisterFunction<SetEyeCoords>(value, view->movieRoot, "SetEyeCoords");
	RegisterFunction<GetAdjustment>(value, view->movieRoot, "GetAdjustment");
	RegisterFunction<SetAdjustmentPersistence>(value, view->movieRoot, "SetAdjustmentPersistence");
	RegisterFunction<SetAdjustmentScale>(value, view->movieRoot, "SetAdjustmentScale");
	RegisterFunction<GetAdjustmentList>(value, view->movieRoot, "GetAdjustmentList");
	RegisterFunction<SaveAdjustment>(value, view->movieRoot, "SaveAdjustment");
	RegisterFunction<LoadAdjustment>(value, view->movieRoot, "LoadAdjustment");
	RegisterFunction<NewAdjustment>(value, view->movieRoot, "NewAdjustment");
	RegisterFunction<RemoveAdjustment>(value, view->movieRoot, "RemoveAdjustment");
	RegisterFunction<ResetAdjustment>(value, view->movieRoot, "ResetAdjustment");
	RegisterFunction<GetCategoryList>(value, view->movieRoot, "GetCategoryList");
	RegisterFunction<GetNodeList>(value, view->movieRoot, "GetNodeList");
	RegisterFunction<GetNodeTransform>(value, view->movieRoot, "GetNodeTransform");
	RegisterFunction<SetNodePosition>(value, view->movieRoot, "SetNodePosition");
	RegisterFunction<SetNodeRotation>(value, view->movieRoot, "SetNodeRotation");
	RegisterFunction<SetNodeScale>(value, view->movieRoot, "SetNodeScale");
	RegisterFunction<ResetTransform>(value, view->movieRoot, "ResetTransform");
	RegisterFunction<GetIdleCategories>(value, view->movieRoot, "GetIdleCategories");
	RegisterFunction<GetIdles>(value, view->movieRoot, "GetIdles");
	RegisterFunction<PlayIdle>(value, view->movieRoot, "PlayIdle");
	RegisterFunction<ResetIdle>(value, view->movieRoot, "ResetIdle");
	RegisterFunction<HideMenu>(value, view->movieRoot, "HideMenu");
	RegisterFunction<Test>(value, view->movieRoot, "Test");
	RegisterFunction<Test2>(value, view->movieRoot, "Test2");
	return true;
}