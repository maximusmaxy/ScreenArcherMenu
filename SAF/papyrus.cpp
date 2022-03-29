#include "papyrus.h"

#include "f4se/PapyrusVM.h"
#include "f4se/PapyrusNativeFunctions.h"
#include "f4se/PapyrusArgs.h"

#include "GameReferences.h"
#include "GameObjects.h"

#include "adjustments.h"
#include "conversions.h"
#include "hacks.h"
#include "eyes.h"

#include "f4se_common/Utilities.h"

namespace SAF {
	DECLARE_STRUCT(Transform, "SAF")

	UInt32 PapyrusCreateAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, BSFixedString espName, bool persistent, bool hidden)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return 0;
		UInt32 handle = adjustments->CreateAdjustment(std::string(name), std::string(espName), persistent, hidden);
		return handle;
	}

	bool PapyrusHasAdjustment(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return false;
		return (adjustments->GetAdjustment(handle) != nullptr);
	}

	void PapyrusRemoveAdjustment(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		adjustments->RemoveAdjustment(handle);
	}

	BSFixedString PapyrusAdjustmentName(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle) {
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return BSFixedString();
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return BSFixedString();
		return BSFixedString(adjustment->name.c_str());
	}

	VMArray<SInt32> PapyrusGetAdjustments(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString espName)
	{
		VMArray<SInt32> result;
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return result;

		adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
			SInt32 adjustmentHandle = adjustment->handle;
			result.Push(&adjustmentHandle);
		});

		return result;
	}

	void PapyrusAdjustmentPersistence(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, bool isPersistent)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;
		adjustment->persistent = isPersistent;
	}

	void PapyrusAdjustmentHidden(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, bool isHidden)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;
		adjustment->hidden = isHidden;
	}

	void PapyrusRemoveModAdjustments(StaticFunctionTag*, BSFixedString espName)
	{
		g_adjustmentManager.RemoveMod(espName);
	}

	VMArray<BSFixedString> PapyrusNodeNames(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<BSFixedString> result;
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return result;
		for (auto& nodeName: adjustments->nodeSets->all) {
			result.Push(&BSFixedString(nodeName.c_str()));
		}
		return result;
	}

	Transform TransformToPapyrus(NiTransform& transform, BSFixedString name, bool override) {
		Transform result;

		result.Set<BSFixedString>("name", name);
		result.Set<bool>("override", override);
		result.Set<float>("x", transform.pos.x);
		result.Set<float>("y", transform.pos.y);
		result.Set<float>("z", transform.pos.z);
		float yaw, pitch, roll;
		NiToEuler(transform.rot, yaw, pitch, roll);
		result.Set<float>("yaw", yaw);
		result.Set<float>("pitch", pitch);
		result.Set<float>("roll", roll);
		result.Set<float>("scale", transform.scale);

		return result;
	}

	NiTransform TransformFromPapyrus(Transform& transform) {
		NiTransform result;

		transform.Get<float>("x", &result.pos.x);
		transform.Get<float>("y", &result.pos.y);
		transform.Get<float>("z", &result.pos.z);
		float yaw, pitch, roll;
		transform.Get<float>("yaw", &yaw);
		transform.Get<float>("pitch", &pitch);
		transform.Get<float>("roll", &roll);
		NiFromEuler(result.rot, yaw, pitch, roll);
		transform.Get<float>("scale", &result.scale);

		return result;
	}

	Transform PapyrusGetTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	{
		Transform result;
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return result;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return result;

		std::string nodeName(name);
		NiTransform transform = adjustment->GetTransformOrDefault(nodeName);
		bool isOverride = adjustments->nodeSets->overrides.count(nodeName) > 0;

		return TransformToPapyrus(transform, name, isOverride);
	}

	void PapyrusSetTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle, Transform transform)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		std::string nodeName(name);
		adjustment->SetTransform(nodeName, TransformFromPapyrus(transform));
		adjustments->UpdateAdjustments(nodeName);
	}

	void PapyrusOverrideTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle, Transform transform)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		std::string nodeName(name);
		adjustments->OverrideTransform(adjustment, nodeName, TransformFromPapyrus(transform));
		adjustments->UpdateAdjustments(nodeName);
	}

	void PapyrusResetTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		std::string nodeName(name);
		adjustment->ResetTransform(nodeName);
		adjustments->UpdateAdjustments(nodeName);
	}

	VMArray<Transform> PapyrusGetAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		VMArray<Transform> result;
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return result;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return result;

		adjustment->ForEachTransformOrDefault([&](std::string name, NiTransform* transform) {
			bool isOverride = adjustments->nodeSets->overrides.count(name) > 0;
			result.Push(&TransformToPapyrus(*transform, BSFixedString(name.c_str()), isOverride));
		}, &adjustments->nodeSets->all);

		return result;
	}

	void PapyrusSetAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, VMArray<Transform> transforms)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		UInt32 length = transforms.Length();
		for (UInt32 i = 0; i < length; ++i) {
			Transform result;
			transforms.Get(&result, i);
			BSFixedString name;
			result.Get<BSFixedString>("name", &name);
			std::string nodeName(name);
			adjustment->SetTransform(nodeName, TransformFromPapyrus(result));
			adjustments->UpdateAdjustments(nodeName);
		}
	}

	void PapyrusOverrideAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, VMArray<Transform> transforms)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		UInt32 length = transforms.Length();
		for (UInt32 i = 0; i < length; ++i) {
			Transform result;
			transforms.Get(&result, i);
			BSFixedString name;
			result.Get<BSFixedString>("name", &name);
			std::string nodeName(name);
			if (adjustments->nodeSets->overrides.count(nodeName))
				adjustments->OverrideTransform(adjustment, nodeName, TransformFromPapyrus(result));
			else
				adjustment->SetTransform(nodeName, TransformFromPapyrus(result));
			adjustments->UpdateAdjustments(nodeName);
		}
	}

	void PapyrusResetAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;
		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		adjustment->Clear();
		adjustments->UpdateAllAdjustments();
	}

	UInt32 PapyrusLoadAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename, BSFixedString espName, bool persistent, bool hidden)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return 0;

		UInt32 handle = adjustments->LoadAdjustment(std::string(filename), std::string(espName), persistent, hidden);
		
		return handle;
	}

	void PapyrusSaveAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename, UInt32 handle)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;

		adjustments->SaveAdjustment(std::string(filename), handle);
	}

	void PapyrusLoadPose(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;

		adjustments->LoadPose(std::string(filename));
		adjustments->UpdateAllAdjustments();
	}

	void PapyrusSavePose(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;

		std::unordered_set<UInt32> handles;
		adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
			if (!adjustment->saved && !adjustment->isDefault) {
				handles.insert(adjustment->handle);
			}
		});

		adjustments->SavePose(std::string(filename), handles);
	}

	void PapyrusResetPose(StaticFunctionTag*, TESObjectREFR* refr)
	{
		std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments) return;

		adjustments->ResetPose();
		adjustments->UpdateAllAdjustments();
	}

	VMArray<float> PapyrusGetEyeCoords(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<float> result;
		float coords[2];
		if (GetEyecoords(refr, coords)) {
			result.Push(&coords[0]);
			result.Push(&coords[1]);
		}
		else
		{
			float f = 0.0f;
			result.Push(&f);
			result.Push(&f);
		}

		return result;
	}

	void PapyrusSetEyeCoords(StaticFunctionTag*, TESObjectREFR* refr, float x, float y)
	{
		SetEyecoords(refr, x, y);
	}

	bool PapyrusGetBlinkHack(StaticFunctionTag*)
	{
		return GetBlinkState() == 1;
	}

	void PapyrusSetBlinkHack(StaticFunctionTag*, bool enabled)
	{
		SetBlinkState(enabled);
	}

	bool PapyrusGetEyeTrackingHack(StaticFunctionTag*)
	{
		return GetDisableEyecoordUpdate() == 1;
	}

	void PapyrusSetEyeTrackingHack(StaticFunctionTag*, bool enabled)
	{
		SetDisableEyecoordUpdate(enabled);
	}

	bool PapyrusGetMorphsHack(StaticFunctionTag*)
	{
		return GetForceMorphUpdate() == 1;
	}

	void PapyrusSetMorphsHack(StaticFunctionTag*, bool enabled)
	{
		SetForceMorphUpdate(enabled);
	}

	bool RegisterPapyrus(VirtualMachine* vm)
	{
		vm->RegisterFunction(new NativeFunction5 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString, BSFixedString, bool, bool>("CreateAdjustment", "SAF", PapyrusCreateAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, bool, TESObjectREFR*, UInt32>("HasAdjustment", "SAF", PapyrusHasAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, UInt32>("RemoveAdjustment", "SAF", PapyrusRemoveAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, BSFixedString, TESObjectREFR*, UInt32>("GetAdjustmentName", "SAF", PapyrusAdjustmentName, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, VMArray<SInt32>, TESObjectREFR*, BSFixedString>("GetAdjustments", "SAF", PapyrusGetAdjustments, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, bool>("SetAdjustmentPersistence", "SAF", PapyrusAdjustmentPersistence, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, bool>("SetAdjustmentHidden", "SAF", PapyrusAdjustmentHidden, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, BSFixedString>("RemoveAllAdjustments", "SAF", PapyrusRemoveModAdjustments, vm));
			
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<BSFixedString>, TESObjectREFR*>("GetAllNodeNames", "SAF", PapyrusNodeNames, vm));

		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, Transform, TESObjectREFR*, BSFixedString, UInt32>("GetNodeTransform", "SAF", PapyrusGetTransform, vm));
		vm->RegisterFunction(new NativeFunction4 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32, Transform>("SetNodeTransform", "SAF", PapyrusSetTransform, vm));
		vm->RegisterFunction(new NativeFunction4 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32, Transform>("OverrideNodeTransform", "SAF", PapyrusOverrideTransform, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32>("ResetNodeTransform", "SAF", PapyrusResetTransform, vm));

		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, VMArray<Transform>, TESObjectREFR*, UInt32>("GetAllNodeTransforms", "SAF", PapyrusGetAll, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, VMArray<Transform>>("SetAllNodeTransforms", "SAF", PapyrusSetAll, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, VMArray<Transform>>("OverrideAllNodeTransforms", "SAF", PapyrusOverrideAll, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, UInt32>("ResetAllNodeTransforms", "SAF", PapyrusResetAll, vm));

		vm->RegisterFunction(new NativeFunction5 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString, BSFixedString, bool, bool>("LoadAdjustment", "SAF", PapyrusLoadAdjustment, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32>("SaveAdjustment", "SAF", PapyrusSaveAdjustment, vm));

		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString>("LoadPose", "SAF", PapyrusLoadPose, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString>("SavePose", "SAF", PapyrusSavePose, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, TESObjectREFR*>("ResetPose", "SAF", PapyrusResetPose, vm));

		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<float>, TESObjectREFR*>("GetEyeCoords", "SAF", PapyrusGetEyeCoords, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, float, float>("SetEyeCoords", "SAF", PapyrusSetEyeCoords, vm));

		vm->RegisterFunction(new NativeFunction0 <StaticFunctionTag, bool>("GetBlinkHack", "SAF", PapyrusGetBlinkHack, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, bool>("SetBlinkHack", "SAF", PapyrusSetBlinkHack, vm));
		vm->RegisterFunction(new NativeFunction0 <StaticFunctionTag, bool>("GetEyeTrackingHack", "SAF", PapyrusGetEyeTrackingHack, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, bool>("SetEyeTrackingHack", "SAF", PapyrusSetEyeTrackingHack, vm));
		vm->RegisterFunction(new NativeFunction0 <StaticFunctionTag, bool>("GetMorphsHack", "SAF", PapyrusGetMorphsHack, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, bool>("SetMorphsHack", "SAF", PapyrusSetMorphsHack, vm));

		return true;
	}
}