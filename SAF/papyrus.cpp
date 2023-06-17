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
#include "io.h"
#include "types.h"
#include "messaging.h"

#include "f4se_common/Utilities.h"

#include <mutex>
#include <winnt.h>

namespace SAF {
	DECLARE_STRUCT(Transform, "SAF")

	UInt32 transformHandle = 0;

	std::unordered_map<UInt32, NiTransform> transformMap;
	std::mutex transformMutex;

	std::unordered_map<BSFixedString, std::vector<UInt32>, BSFixedStringHash, BSFixedStringKeyEqual> registeredTransforms;
	std::mutex registerMutex;

	UInt32 PapyrusCreateAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, BSFixedString espName)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return 0;

		UInt32 handle = adjustments->CreateAdjustment(name.c_str(), espName.c_str());
		return handle;
	}

	bool PapyrusHasAdjustment(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return false;

		return (adjustments->GetAdjustment(handle) != nullptr);
	}

	void PapyrusRemoveAdjustment(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		ActorAdjustmentsPtr adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		adjustments->RemoveAdjustment(handle);
		adjustments->UpdateAllAdjustments();
	}

	BSFixedString PapyrusAdjustmentName(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle) {
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return BSFixedString();

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return BSFixedString();

		return BSFixedString(adjustment->name.c_str());
	}

	VMArray<UInt32> PapyrusGetAdjustments(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString espName)
	{
		VMArray<UInt32> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
			if (!_stricmp(adjustment->mod.c_str(), espName.c_str())) {
				result.Push(&adjustment->handle);
			}
		});

		return result;
	}

	VMArray<UInt32> PapyrusGetAdjustmentsByName(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name)
	{
		VMArray<UInt32> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
			if (!_stricmp(adjustment->name.c_str(), name.c_str())) {
				result.Push(&adjustment->handle);
			}
		});

		return result;
	}

	VMArray<UInt32> PapyrusGetAdjustmentsByType(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString strType)
	{
		VMArray<UInt32> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		//TODO make this a <BSFixedString, UInt32> map
		static BSFixedString defaultType("Default");
		static BSFixedString skeletonType("Skeleton");
		static BSFixedString raceType("Race");

		UInt32 type;
		if (strType == defaultType)
			type = kAdjustmentTypeDefault;
		else if (strType == skeletonType)
			type = kAdjustmentTypeSkeleton;
		else if (strType == raceType)
			type = kAdjustmentTypeRace;
		else
			return result;

		adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
			if (adjustment->type == type) {
				result.Push(&adjustment->handle);
			}
		});

		return result;
	}

	UInt32 PapyrusGetAdjustmentByUniqueType(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString strType)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return 0;

		//TODO make this a <BSFixedString, UInt32> map
		static BSFixedString poseType("Pose");
		static BSFixedString tongueType("Tongue");

		UInt32 type;
		if (strType == poseType)
			type = kAdjustmentTypePose;
		else if (strType == tongueType)
			type = type = kAdjustmentTypeTongue;
		else
			return 0;

		auto adjustment = adjustments->FindAdjustment([&](AdjustmentPtr adjustment) {
			return (adjustment->type == type);
		});

		if (!adjustment)
			return 0;

		return adjustment->handle;
	}

	void PapyrusRemoveModAdjustments(StaticFunctionTag*, BSFixedString espName)
	{
		g_adjustmentManager.RemoveMod(espName);
	}

	void PapyrusRemoveAllAdjustments(StaticFunctionTag*)
	{
		g_adjustmentManager.RemoveAllAdjustments();
	}

	UInt32 PapyrusGetAdjustmentFile(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return 0;

		auto adjustment = adjustments->GetFile(filename.c_str());
		if (!adjustment)
			return 0;

		return adjustment->handle;
	}

	void PapyrusRemoveAdjustmentFile(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		adjustments->RemoveFile(filename.c_str(), 0);
		adjustments->UpdateAllAdjustments();
	}

	VMArray<BSFixedString> PapyrusAllNodeNames(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<BSFixedString> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		for (auto& nodeName : adjustments->nodeSets->allStrings) {
			if (adjustments->HasNode(nodeName)) {
				result.Push(&BSFixedString(nodeName));
			}
		}

		return result;
	}

	VMArray<BSFixedString> PapyrusBaseNodeNames(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<BSFixedString> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		for (auto& nodeName : adjustments->nodeSets->baseStrings) {
			if (adjustments->HasNode(nodeName)) {
				result.Push(&BSFixedString(nodeName));
			}
		}

		return result;
	}

	VMArray<BSFixedString> PapyrusCenterNodeNames(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<BSFixedString> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		for (auto& nodeName : adjustments->nodeSets->center) {
			if (adjustments->HasNode(nodeName)) {
				result.Push(&BSFixedString(nodeName));
			}
		}

		return result;
	}

	VMArray<BSFixedString> PapyrusLeftNodeNames(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<BSFixedString> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		for (auto& kvp : adjustments->nodeSets->mirror) {
			if (adjustments->HasNode(kvp.first)) {
				result.Push(&BSFixedString(kvp.first));
			}
		}

		return result;
	}

	VMArray<BSFixedString> PapyrusRightNodeNames(StaticFunctionTag*, TESObjectREFR* refr)
	{
		VMArray<BSFixedString> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		for (auto& kvp : adjustments->nodeSets->mirror) {
			if (adjustments->HasNode(kvp.second)) {
				result.Push(&BSFixedString(kvp.second));
			}
		}

		return result;
	}

	Transform TransformToPapyrus(NiTransform& transform, BSFixedString name, bool offset) {
		Transform result;

		result.Set<BSFixedString>("name", name);
		result.Set<bool>("offset", offset);
		result.Set<float>("x", transform.pos.x);
		result.Set<float>("y", transform.pos.y);
		result.Set<float>("z", transform.pos.z);

		float yaw, pitch, roll;
		MatrixToEulerYPR(transform.rot, yaw, pitch, roll);
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
		MatrixFromEulerYPR(result.rot, yaw, pitch, roll);

		transform.Get<float>("scale", &result.scale);

		return result;
	}

	const NodeKey GetPapyrusKeyTransform(BSFixedString name, Transform& transform, NiTransform* out)
	{
		NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(name);
		if (!nodeKey.key)
			return nodeKey;
		*out = TransformFromPapyrus(transform);

		//if node key is not an offset, but the transform specifies offset, make it an offset key instead
		bool isOffset;
		transform.Get<bool>("offset", &isOffset);
		return (!nodeKey.offset && isOffset) ? NodeKey(nodeKey.name, isOffset) : nodeKey;
	}

	Transform PapyrusGetTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	{
		Transform result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return result;

		const NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(name);

		NiTransform transform = adjustment->GetTransformOrDefault(nodeKey);

		return TransformToPapyrus(transform, name, nodeKey.offset);
	}

	void PapyrusSetTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle, Transform transform)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		NiTransform papyrusTransform;
		NodeKey nodeKey = GetPapyrusKeyTransform(name, transform, &papyrusTransform);
		if (nodeKey.key && adjustments->HasNode(nodeKey.name)) {
			adjustment->SetTransform(nodeKey, papyrusTransform);
			adjustments->UpdateNode(nodeKey.name);
		}
	}

	void PapyrusOverrideTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle, Transform transform)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		NiTransform papyrusTransform;
		NodeKey nodeKey = GetPapyrusKeyTransform(name, transform, &papyrusTransform);
		if (nodeKey.key && adjustments->HasNode(nodeKey.name)) {
			if (nodeKey.offset) {
				//if an offset just do a regular set
				adjustment->SetTransform(nodeKey, papyrusTransform);
			}
			else {
				adjustments->OverrideTransform(adjustment, nodeKey, papyrusTransform);
			}
			adjustments->UpdateNode(nodeKey.name);
		}
	}

	void PapyrusResetTransform(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(name);
		if (nodeKey.key) {
			adjustment->ResetTransform(nodeKey);
			adjustments->UpdateNode(nodeKey.name);
		}
	}

	VMArray<Transform> PapyrusGetAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		VMArray<Transform> result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return result;

		adjustment->ForEachTransformOrDefault([&](const NodeKey* nodeKey, NiTransform* transform) {
			result.Push(&TransformToPapyrus(*transform, nodeKey->name, nodeKey->offset));
			}, &adjustments->nodeSets->all);

		return result;
	}

	void PapyrusSetAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, VMArray<Transform> transforms)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		UInt32 length = transforms.Length();
		for (UInt32 i = 0; i < length; ++i) {
			Transform result;
			transforms.Get(&result, i);

			BSFixedString name;
			result.Get<BSFixedString>("name", &name);

			NiTransform papyrusTransform;
			NodeKey nodeKey = GetPapyrusKeyTransform(name, result, &papyrusTransform);
			if (nodeKey.key && adjustments->HasNode(nodeKey.name))
			{
				adjustment->SetTransform(nodeKey, papyrusTransform);
			}
		}
		adjustments->UpdateAllAdjustments();
	}

	void PapyrusOverrideAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, VMArray<Transform> transforms)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		UInt32 length = transforms.Length();
		for (UInt32 i = 0; i < length; ++i) {
			Transform result;
			transforms.Get(&result, i);

			BSFixedString name;
			result.Get<BSFixedString>("name", &name);

			NiTransform papyrusTransform;
			NodeKey nodeKey = GetPapyrusKeyTransform(name, result, &papyrusTransform);
			if (nodeKey.key && adjustments->HasNode(nodeKey.name))
			{
				if (nodeKey.offset) {
					adjustment->SetTransform(nodeKey, papyrusTransform);
				}
				else {
					adjustments->OverrideTransform(adjustment, nodeKey, papyrusTransform);
				}
			}
		}
		adjustments->UpdateAllAdjustments();
	}

	void PapyrusResetAll(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		adjustment->Clear();
		adjustments->UpdateAllAdjustments();
	}

	UInt32 PapyrusLoadAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename, BSFixedString espName)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return 0;

		UInt32 handle = adjustments->LoadAdjustmentHandle(filename.c_str(), espName.c_str());
		adjustments->UpdateAllAdjustments();

		return handle;
	}

	void PapyrusCacheAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		adjustments->CacheUpdatedAdjustment(filename.c_str());
	}

	UInt32 PapyrusLoadCachedAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename, BSFixedString espName)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return 0;

		UInt32 handle = adjustments->LoadAdjustmentHandle(filename.c_str(), espName.c_str(), true);
		adjustments->UpdateAllAdjustments();

		return handle;
	}

	UInt32 PapyrusCopyAdjustment(StaticFunctionTag*, TESObjectREFR* srcRefr, UInt32 handle, TESObjectREFR* dstRefr) 
	{
		auto srcActor = g_adjustmentManager.GetActorAdjustments(srcRefr);
		if (!srcActor)
			return 0;

		auto srcAdjustment = srcActor->GetAdjustment(handle);
		if (!srcAdjustment)
			return 0;

		auto dstActor = g_adjustmentManager.GetActorAdjustments(dstRefr);
		if (!dstActor)
			return 0;

		auto dstAdjustment = dstActor->CreateFromAdjustment(srcAdjustment);
		dstActor->UpdateAllAdjustments();
		return dstAdjustment->handle;
	};

	bool PapyrusCopyAllAdjustments(StaticFunctionTag*, TESObjectREFR* srcRefr, TESObjectREFR* dstRefr)
	{
		auto srcActor = g_adjustmentManager.GetActorAdjustments(srcRefr);
		if (!srcActor)
			return false;

		auto dstActor = g_adjustmentManager.GetActorAdjustments(dstRefr);
		if (!dstActor)
			return false;

		dstActor->Clear();
		for (auto& srcAdjustment : srcActor->list) {
			dstActor->CreateFromAdjustment(srcAdjustment);
		}
		dstActor->UpdateAllAdjustments();

		return true;
	}

	void PapyrusSaveAdjustment(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename, UInt32 handle)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		adjustments->SaveAdjustment(filename.c_str(), handle);
		adjustments->UpdateAllAdjustments();
	}

	SInt32 PapyrusGetAdjustmentScale(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return 100;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return 100;

		return (SInt32)std::round(adjustment->scale * 100);
	}

	void PapyrusSetAdjustmentScale(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, SInt32 scale)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return;

		adjustment->SetScale(scale * 0.01);
		adjustments->UpdateAllAdjustments();
	}

	Transform PapyrusGetTransformScaled(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	{
		Transform result;
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return result;

		auto adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment)
			return result;

		const NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(name);

		NiTransform transform = adjustment->GetScaledTransformOrDefault(nodeKey);

		return TransformToPapyrus(transform, name, nodeKey.offset);
	}

	void PapyrusLoadPose(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		std::string path = GetPathWithExtension(POSES_PATH, filename, ".json");

		adjustments->LoadPose(path.c_str());
		adjustments->UpdateAllAdjustments();
	}

	void PapyrusSavePose(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString filename)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

		ExportSkeleton exports;
		exports.skeleton = "All";

		adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
			if (adjustment->type == kAdjustmentTypeDefault || adjustment->type == kAdjustmentTypePose) {
				exports.handles.insert(adjustment->handle);
			}
			});

		adjustments->SavePose(filename.c_str(), &exports);
	}

	void PapyrusResetPose(StaticFunctionTag*, TESObjectREFR* refr)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
		if (!adjustments)
			return;

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

	//NiTransform GetPapyrusTransform(UInt32 handle)
	//{
	//	std::lock_guard<std::mutex> lock(transformMutex);

	//	auto it = transformMap.find(handle);
	//	if (it != transformMap.end()) {
	//		return it->second;
	//	}
	//	else {
	//		auto emplaced = transformMap.emplace(handle, TransformIdentity());
	//		return emplaced.first->second;
	//	}
	//}

	//void SetPapyrusTransform(UInt32 handle, const NiTransform& transform)
	//{
	//	std::lock_guard<std::mutex> lock(transformMutex);

	//	transformMap[handle] = transform;
	//}

	//void ErasePapyrusTransform(UInt32 handle)
	//{
	//	std::lock_guard<std::mutex> lock(transformMutex);

	//	transformMap.erase(handle);
	//}

	//void ErasePapyrusTransforms(std::vector<UInt32>& handles)
	//{
	//	std::lock_guard<std::mutex> lock(transformMutex);

	//	for (auto& handle : handles) {
	//		transformMap.erase(handle);
	//	}
	//}

	//UInt32 PapyrusCreateTransformHandle(StaticFunctionTag*)
	//{
	//	return InterlockedIncrement(&transformHandle);
	//}

	//UInt32 PapyrusCreateTransformHandleLocal(StaticFunctionTag*, Transform transform)
	//{
	//	UInt32 handle = InterlockedIncrement(&transformHandle);

	//	SetPapyrusTransform(handle, TransformFromPapyrus(transform));

	//	return handle;
	//}

	//void PapyrusSetTransformHandle(StaticFunctionTag*, UInt32 aHandle, UInt32 bHandle)
	//{
	//	NiTransform transform = GetPapyrusTransform(bHandle);
	//	SetPapyrusTransform(aHandle, transform);
	//}

	//UInt32 PapyrusCopyTransformHandle(StaticFunctionTag*, UInt32 handle)
	//{
	//	NiTransform transform = GetPapyrusTransform(handle);
	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);

	//	SetPapyrusTransform(newHandle, transform);

	//	return newHandle;
	//}

	//void PapyrusDeleteTransformHandle(StaticFunctionTag*, UInt32 handle)
	//{
	//	ErasePapyrusTransform(handle);
	//}

	//void PapyrusRegisterTransformHandle(StaticFunctionTag*, BSFixedString espName, UInt32 handle)
	//{
	//	std::lock_guard<std::mutex> lock(registerMutex);

	//	registeredTransforms[espName].push_back(handle);
	//}

	//void PapyrusDeleteRegisteredTransformHandles(StaticFunctionTag*, BSFixedString espName)
	//{
	//	std::lock_guard<std::mutex> lock(registerMutex);

	//	auto it = registeredTransforms.find(espName);
	//	if (it != registeredTransforms.end()) {
	//		ErasePapyrusTransforms(it->second);
	//		registeredTransforms.erase(it);
	//	}
	//}

	//UInt32 PapyrusGetAdjustmentTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, UInt32 handle, BSFixedString name, bool offset)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return 0;

	//	auto adjustment = adjustments->GetAdjustment(handle);
	//	if (!adjustment)
	//		return 0;

	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, adjustment->GetTransformOrDefault(NodeKey(name, offset)));
	//	return newHandle;
	//}

	//void PapyrusSetAdjustmentTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, UInt32 aHandle, BSFixedString name, bool offset, UInt32 tHandle)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return;

	//	auto adjustment = adjustments->GetAdjustment(aHandle);
	//	if (!adjustment)
	//		return;

	//	NodeKey nodeKey(name, offset);

	//	NiTransform transform = GetPapyrusTransform(tHandle);
	//	adjustment->SetTransform(nodeKey, transform);
	//	adjustments->UpdateNode(nodeKey.name);
	//}

	//UInt32 PapyrusGetSkeletonTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return 0;

	//	auto node = GetFromNodeMap(adjustments->poseMap, name);
	//	if (!node)
	//		return 0;

	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, node->m_localTransform);
	//	return newHandle;
	//}

	//UInt32 PapyrusGetBaseTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return 0;

	//	auto transform = GetFromBaseMap(*adjustments->baseMap, name);
	//	if (!transform)
	//		return 0;

	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, *transform);
	//	return newHandle;
	//}

	//bool PapyrusCopyAdjustmentTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, UInt32 aHandle, BSFixedString name, bool offset, UInt32 tHandle)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return false;

	//	auto adjustment = adjustments->GetAdjustment(aHandle);
	//	if (!adjustment)
	//		return false;

	//	NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(name);
	//	if (!nodeKey.key)
	//		return false;

	//	SetPapyrusTransform(tHandle, adjustment->GetTransformOrDefault(nodeKey));

	//	return true;
	//}

	//bool PapyrusCopySkeletonTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return false;

	//	auto node = GetFromNodeMap(adjustments->poseMap, name);
	//	if (!node)
	//		return false;

	//	SetPapyrusTransform(handle, node->m_localTransform);

	//	return true;
	//}

	//bool PapyrusCopyBaseTransformHandle(StaticFunctionTag*, TESObjectREFR* refr, BSFixedString name, UInt32 handle)
	//{
	//	auto adjustments = g_adjustmentManager.GetActorAdjustments(refr);
	//	if (!adjustments)
	//		return false;

	//	auto transform = GetFromBaseMap(*adjustments->baseMap, name);
	//	if (!transform)
	//		return false;

	//	SetPapyrusTransform(handle, *transform);

	//	return true;
	//}

	//Transform PapyrusGetTransformLocal(StaticFunctionTag*, UInt32 handle)
	//{
	//	return TransformToPapyrus(GetPapyrusTransform(handle), "", false);
	//}

	//void PapyrusSetTransformLocal(StaticFunctionTag*, UInt32 handle, Transform transform)
	//{
	//	SetPapyrusTransform(handle, TransformFromPapyrus(transform));
	//}

	//UInt32 PapyrusGetAppliedTransform(StaticFunctionTag*, UInt32 aHandle, UInt32 bHandle)
	//{
	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, MultiplyNiTransform(GetPapyrusTransform(aHandle), GetPapyrusTransform(bHandle)));
	//	return newHandle;
	//}

	//void PapyrusApplyTransform(StaticFunctionTag*, UInt32 aHandle, UInt32 bHandle)
	//{
	//	SetPapyrusTransform(aHandle, MultiplyNiTransform(GetPapyrusTransform(aHandle), GetPapyrusTransform(bHandle)));
	//}

	//UInt32 PapyrusGetInverseTransform(StaticFunctionTag*, UInt32 handle)
	//{
	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, InvertNiTransform(GetPapyrusTransform(handle)));
	//	return newHandle;
	//}

	//void PapyrusInverseTransform(StaticFunctionTag*, UInt32 handle)
	//{
	//	SetPapyrusTransform(handle, InvertNiTransform(GetPapyrusTransform(handle)));
	//}

	//UInt32 PapyrusGetDifferenceTransform(StaticFunctionTag*, UInt32 aHandle, UInt32 bHandle)
	//{
	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, NegateNiTransform(GetPapyrusTransform(aHandle), GetPapyrusTransform(bHandle)));
	//	return newHandle;
	//}

	//void PapyrusDifferenceTransform(StaticFunctionTag*, UInt32 aHandle, UInt32 bHandle)
	//{
	//	SetPapyrusTransform(aHandle, NegateNiTransform(GetPapyrusTransform(aHandle), GetPapyrusTransform(bHandle)));
	//}

	//UInt32 PapyrusGetBetweenTransform(StaticFunctionTag*, UInt32 handle, float scalar)
	//{
	//	UInt32 newHandle = InterlockedIncrement(&transformHandle);
	//	SetPapyrusTransform(newHandle, SlerpNiTransform(GetPapyrusTransform(handle), scalar));
	//	return newHandle;
	//}

	//void PapyrusBetweenTransform(StaticFunctionTag*, UInt32 handle, float scalar)
	//{
	//	SetPapyrusTransform(handle, SlerpNiTransform(GetPapyrusTransform(handle), scalar));
	//}

	//VMArray<float> PapyrusGetTransformPos(StaticFunctionTag*, UInt32 handle)
	//{
	//	VMArray<float> result;

	//	NiTransform transform = GetPapyrusTransform(handle);
	//	result.Push(&transform.pos.x);
	//	result.Push(&transform.pos.y);
	//	result.Push(&transform.pos.z);

	//	return result;
	//}

	//void PapyrusSetTransformPos(StaticFunctionTag*, UInt32 handle, VMArray<float> pos)
	//{
	//	NiTransform transform = GetPapyrusTransform(handle);

	//	float result;
	//	pos.Get(&result, 0);
	//	transform.pos.x = result;

	//	pos.Get(&result, 1);
	//	transform.pos.x = result;

	//	pos.Get(&result, 2);
	//	transform.pos.x = result;

	//	SetPapyrusTransform(handle, transform);
	//}

	//VMArray<float> PapyrusGetTransformRotation(StaticFunctionTag*, UInt32 handle)
	//{
	//	VMArray<float> result;

	//	NiTransform transform = GetPapyrusTransform(handle);

	//	result.Push(&transform.rot.data[1][0]);
	//	result.Push(&transform.rot.data[2][0]);
	//	result.Push(&transform.rot.data[0][1]);
	//	result.Push(&transform.rot.data[1][1]);
	//	result.Push(&transform.rot.data[2][1]);
	//	result.Push(&transform.rot.data[0][2]);
	//	result.Push(&transform.rot.data[1][2]);
	//	result.Push(&transform.rot.data[2][2]);

	//	return result;
	//}

	//void PapyrusSetTransformRotation(StaticFunctionTag*, UInt32 handle, VMArray<float> rot)
	//{
	//	NiTransform transform = GetPapyrusTransform(handle);

	//	float result;

	//	rot.Get(&result, 0);
	//	transform.rot.data[0][0] = result;
	//	rot.Get(&result, 1);
	//	transform.rot.data[1][0] = result;
	//	rot.Get(&result, 2);
	//	transform.rot.data[2][0] = result;
	//	rot.Get(&result, 3);
	//	transform.rot.data[0][1] = result;
	//	rot.Get(&result, 4);
	//	transform.rot.data[1][1] = result;
	//	rot.Get(&result, 5);
	//	transform.rot.data[2][1] = result;
	//	rot.Get(&result, 6);
	//	transform.rot.data[0][2] = result;
	//	rot.Get(&result, 7);
	//	transform.rot.data[1][2] = result;
	//	rot.Get(&result, 8);
	//	transform.rot.data[2][2] = result;

	//	SetPapyrusTransform(handle, transform);
	//}

	//void PapyrusRotateTransformAxis(StaticFunctionTag*, UInt32 handle, UInt32 axis, float scalar)
	//{
	//	if (axis < kAxisX || axis > kAxisZ)
	//		return;

	//	NiTransform transform = GetPapyrusTransform(handle);
	//	MultiplyNiMatrix(transform.rot, GetXYZRotation(axis, scalar));
	//	SetPapyrusTransform(handle, transform);
	//}

	//float PapyrusGetTransformScale(StaticFunctionTag*, UInt32 handle)
	//{
	//	return GetPapyrusTransform(handle).scale;
	//}

	//void PapyrusSetTransformScale(StaticFunctionTag*, UInt32 handle, float scalar)
	//{
	//	NiTransform transform = GetPapyrusTransform(handle);
	//	transform.scale = scalar;
	//	SetPapyrusTransform(handle, transform);
	//}

	bool RegisterPapyrus(VirtualMachine* vm)
	{
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString, BSFixedString>("CreateAdjustment", "SAF", PapyrusCreateAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, bool, TESObjectREFR*, UInt32>("HasAdjustment", "SAF", PapyrusHasAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, UInt32>("RemoveAdjustment", "SAF", PapyrusRemoveAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, BSFixedString, TESObjectREFR*, UInt32>("GetAdjustmentName", "SAF", PapyrusAdjustmentName, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, VMArray<UInt32>, TESObjectREFR*, BSFixedString>("GetAdjustments", "SAF", PapyrusGetAdjustments, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, VMArray<UInt32>, TESObjectREFR*, BSFixedString>("GetAdjustmentsByName", "SAF", PapyrusGetAdjustmentsByName, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, VMArray<UInt32>, TESObjectREFR*, BSFixedString>("GetAdjustmentsByType", "SAF", PapyrusGetAdjustmentsByType, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString>("GetAdjustmentByUniqueType", "SAF", PapyrusGetAdjustmentByUniqueType, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, BSFixedString>("RemoveModAdjustments", "SAF", PapyrusRemoveModAdjustments, vm));
		vm->RegisterFunction(new NativeFunction0 <StaticFunctionTag, void>("RemoveAllAdjustments", "SAF", PapyrusRemoveAllAdjustments, vm));

		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString>("GetAdjustmentFile", "SAF", PapyrusGetAdjustmentFile, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString>("RemoveAdjustmentFile", "SAF", PapyrusRemoveAdjustmentFile, vm));

		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<BSFixedString>, TESObjectREFR*>("GetAllNodeNames", "SAF", PapyrusAllNodeNames, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<BSFixedString>, TESObjectREFR*>("GetBaseNodeNames", "SAF", PapyrusBaseNodeNames, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<BSFixedString>, TESObjectREFR*>("GetCenterNodeNames", "SAF", PapyrusCenterNodeNames, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<BSFixedString>, TESObjectREFR*>("GetLeftNodeNames", "SAF", PapyrusLeftNodeNames, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<BSFixedString>, TESObjectREFR*>("GetRightNodeNames", "SAF", PapyrusRightNodeNames, vm));

		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, Transform, TESObjectREFR*, BSFixedString, UInt32>("GetNodeTransform", "SAF", PapyrusGetTransform, vm));
		vm->RegisterFunction(new NativeFunction4 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32, Transform>("SetNodeTransform", "SAF", PapyrusSetTransform, vm));
		vm->RegisterFunction(new NativeFunction4 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32, Transform>("OverrideNodeTransform", "SAF", PapyrusOverrideTransform, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32>("ResetNodeTransform", "SAF", PapyrusResetTransform, vm));

		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, SInt32, TESObjectREFR*, UInt32>("GetAdjustmentScale", "SAF", PapyrusGetAdjustmentScale, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, SInt32>("SetAdjustmentScale", "SAF", PapyrusSetAdjustmentScale, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, Transform, TESObjectREFR*, BSFixedString, UInt32>("GetNodeTransformScaled", "SAF", PapyrusGetTransformScaled, vm));

		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, VMArray<Transform>, TESObjectREFR*, UInt32>("GetAllNodeTransforms", "SAF", PapyrusGetAll, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, VMArray<Transform>>("SetAllNodeTransforms", "SAF", PapyrusSetAll, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, UInt32, VMArray<Transform>>("OverrideAllNodeTransforms", "SAF", PapyrusOverrideAll, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, UInt32>("ResetAllNodeTransforms", "SAF", PapyrusResetAll, vm));

		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString, BSFixedString>("LoadAdjustment", "SAF", PapyrusLoadAdjustment, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString, UInt32>("SaveAdjustment", "SAF", PapyrusSaveAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString>("CacheAdjustment", "SAF", PapyrusCacheAdjustment, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString, BSFixedString>("LoadCachedAdjustment", "SAF", PapyrusLoadCachedAdjustment, vm));
		vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, UInt32, TESObjectREFR*, UInt32, TESObjectREFR*>("CopyAdjustment", "SAF", PapyrusCopyAdjustment, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, bool, TESObjectREFR*, TESObjectREFR*>("CopyAllAdjustments", "SAF", PapyrusCopyAllAdjustments, vm));

		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString>("LoadPose", "SAF", PapyrusLoadPose, vm));
		vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, TESObjectREFR*, BSFixedString>("SavePose", "SAF", PapyrusSavePose, vm));
		vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, TESObjectREFR*>("ResetPose", "SAF", PapyrusResetPose, vm));

		//vm->RegisterFunction(new NativeFunction0 <StaticFunctionTag, UInt32>("CreateTransformHandle", "SAF", PapyrusCreateTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, UInt32, Transform>("CreateTransformHandleLocal", "SAF", PapyrusCreateTransformHandleLocal, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, UInt32>("SetTransformHandle", "SAF", PapyrusSetTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, UInt32, UInt32>("CopyTransformHandle", "SAF", PapyrusCopyTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, UInt32>("DeleteTranformHandle", "SAF", PapyrusDeleteTransformHandle, vm));

		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, BSFixedString, UInt32>("RegisterTransformHandle", "SAF", PapyrusRegisterTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, BSFixedString>("DeleteRegisteredTransformHandles", "SAF", PapyrusDeleteRegisteredTransformHandles, vm));

		//vm->RegisterFunction(new NativeFunction4 <StaticFunctionTag, UInt32, TESObjectREFR*, UInt32, BSFixedString, bool>("GetAdjustmentTransformHandle", "SAF", PapyrusGetAdjustmentTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString>("GetSkeletonTransformHandle", "SAF", PapyrusGetSkeletonTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, TESObjectREFR*, BSFixedString>("GetBaseTransformHandle", "SAF", PapyrusGetBaseTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction5 <StaticFunctionTag, void, TESObjectREFR*, UInt32, BSFixedString, bool, UInt32>("SetAdjustmentTransformHandle", "SAF", PapyrusSetAdjustmentTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction5 <StaticFunctionTag, bool, TESObjectREFR*, UInt32, BSFixedString, bool, UInt32>("CopyAdjustmentTransformHandle", "SAF", PapyrusCopyAdjustmentTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, bool, TESObjectREFR*, BSFixedString, UInt32>("CopySkeletonTransformHandle", "SAF", PapyrusCopySkeletonTransformHandle, vm));
		//vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, bool, TESObjectREFR*, BSFixedString, UInt32>("CopyBaseTransformHandle", "SAF", PapyrusCopyBaseTransformHandle, vm));

		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, Transform, UInt32>("GetTransformLocal", "SAF", PapyrusGetTransformLocal, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, Transform>("SetTransformLocal", "SAF", PapyrusSetTransformLocal, vm));

		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, UInt32, UInt32>("GetAppliedTransform", "SAF", PapyrusGetAppliedTransform, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, UInt32>("ApplyTransform", "SAF", PapyrusApplyTransform, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, UInt32, UInt32>("GetInverseTransform", "SAF", PapyrusGetInverseTransform, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, void, UInt32>("InverseTransform", "SAF", PapyrusInverseTransform, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, UInt32, UInt32>("GetDifferenceTransform", "SAF", PapyrusGetDifferenceTransform, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, UInt32>("DifferenceTransform", "SAF", PapyrusDifferenceTransform, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, UInt32, UInt32, float>("GetBetweenTransform", "SAF", PapyrusGetBetweenTransform, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, float>("BetweenTransform", "SAF", PapyrusBetweenTransform, vm));

		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<float>, UInt32>("GetTransformPosition", "SAF", PapyrusGetTransformPos, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, VMArray<float>>("SetTransformPosition", "SAF", PapyrusSetTransformPos, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, VMArray<float>, UInt32>("GetTransformRotation", "SAF", PapyrusGetTransformRotation, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, VMArray<float>>("SetTransformRotation", "SAF", PapyrusSetTransformRotation, vm));
		//vm->RegisterFunction(new NativeFunction3 <StaticFunctionTag, void, UInt32, UInt32, float>("RotateTransformAxis", "SAF", PapyrusRotateTransformAxis, vm));
		//vm->RegisterFunction(new NativeFunction1 <StaticFunctionTag, float, UInt32>("GetTransformScale", "SAF", PapyrusGetTransformScale, vm));
		//vm->RegisterFunction(new NativeFunction2 <StaticFunctionTag, void, UInt32, float>("SetTransformScale", "SAF", PapyrusSetTransformScale, vm));

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

	void RevertPapyrus() {
		transformMap.clear();
		registeredTransforms.clear();
		transformHandle = 0;
	}
}