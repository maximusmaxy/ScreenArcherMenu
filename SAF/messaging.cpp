#include "messaging.h"

#include "adjustments.h"
#include "f4se/GameRTTI.h"

namespace SAF {

	SafMessaging safMessaging;

	class SafEventReciever :
		public BSTEventSink<TESObjectLoadedEvent>,
		public BSTEventSink<TESLoadGameEvent>,
		public BSTEventSink<TESInitScriptEvent>
	{
	public:
		EventResult	ReceiveEvent(TESObjectLoadedEvent* evn, void* dispatcher)
		{
			if (!g_adjustmentManager.gameLoaded) return kEvent_Continue;

			TESForm* form = LookupFormByID(evn->formId);
			Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
			if (!actor)
				return kEvent_Continue;

			g_adjustmentManager.ActorLoaded(actor, evn->loaded);

			return kEvent_Continue;
		}

		EventResult	ReceiveEvent(TESInitScriptEvent* evn, void* dispatcher)
		{
			Actor* actor = DYNAMIC_CAST(evn->reference, TESForm, Actor);
			if (!actor)
				return kEvent_Continue;

			g_adjustmentManager.ActorLoaded(actor, true);

			return kEvent_Continue;
		}

		EventResult	ReceiveEvent(TESLoadGameEvent* evn, void* dispatcher)
		{
			g_adjustmentManager.GameLoaded();

			return kEvent_Continue;
		}
	};

	SafEventReciever safEventReciever;

	AdjustmentPtr GetAdjustment(UInt32 formId, UInt32 handle)
	{
		std::lock_guard<std::shared_mutex> lock(g_adjustmentManager.actorMutex);

		return g_adjustmentManager.GetAdjustment(formId, handle);
	}

	ActorAdjustmentsPtr GetActorAdjustment(UInt32 formId)
	{
		auto adjustments = g_adjustmentManager.GetActorAdjustments(formId);
		
		if (!adjustments)
			adjustments = g_adjustmentManager.CreateActorAdjustment(formId);

		return adjustments;
	}

	UInt32 CreateAdjustment(ActorAdjustmentsPtr adjustments, const char* name, const char* mod) {
		return adjustments->CreateAdjustment(name, mod);
	}

	UInt32 SaveAdjustment(ActorAdjustmentsPtr adjustments, UInt32 handle, const char* filename) {
		return adjustments->SaveAdjustment(filename, handle);
	}

	UInt32 LoadAdjustment(ActorAdjustmentsPtr adjustments, const char* filename) {
		auto adjustment = adjustments->LoadAdjustmentPath(filename);
		if (!adjustment)
			return 0;

		return adjustment->handle;
	}

	void RemoveAdjustment(ActorAdjustmentsPtr adjustments, UInt32 handle) {
		adjustments->RemoveAdjustment(handle);
	}

	void SetTransform(AdjustmentPtr adjustment, const NodeKey& nodeKey, NiTransform& transform)
	{
		adjustment->SetTransform(nodeKey, transform);
	}

	void NegateTransform(ActorAdjustmentsPtr adjustments, AdjustmentPtr adjustment, NodeKey& nodeKey)
	{
		adjustments->NegateTransform(adjustment, nodeKey);
	}

	void RotateTransformXYZ(ActorAdjustmentsPtr adjustments, AdjustmentPtr adjustment, NodeKey& nodeKey, UInt32 type, float scalar)
	{
		adjustments->RotateTransformXYZ(adjustment, nodeKey, type, scalar);
	}

	void ResetAdjustment(AdjustmentPtr adjustment)
	{
		adjustment->Clear();
	}

	void SavePose(ActorAdjustmentsPtr adjustments, const char* filename, ExportSkeleton* exports)
	{
		adjustments->SavePose(filename, exports);
	}

	void SaveOSPose(ActorAdjustmentsPtr adjustments, const char* filename, ExportSkeleton* exports)
	{
		adjustments->SaveOutfitStudioPose(filename, exports);
	}

	UInt32 LoadPose(ActorAdjustmentsPtr adjustments, const char* filename)
	{
		return adjustments->LoadPose(filename);
	}
	
	void ResetPose(ActorAdjustmentsPtr adjustments)
	{
		adjustments->ResetPose();
	}

	void LoadSkeletonAdjustment(UInt32 formId, bool isFemale, const char* filename, bool npc, bool clear, bool enable)
	{
		g_adjustmentManager.LoadRaceAdjustment(formId, isFemale, filename, npc, clear, enable);
	}
	
	UInt32 MoveAdjustment(ActorAdjustmentsPtr adjustments, SInt32 from, SInt32 to)
	{
		return adjustments->MoveAdjustment(from, to);
	}

	void RenameAdjustment(AdjustmentPtr adjustment, const char* name)
	{
		adjustment->Rename(name);
	}

	void LoadTongueAdjustment(ActorAdjustmentsPtr adjustments, TransformMap* transforms)
	{
		g_adjustmentManager.LoadTongueAdjustment(adjustments, transforms);
	}
	
	void ScaleAdjustment(AdjustmentPtr adjustment, float scale)
	{
		adjustment->SetScale(scale);
	}

	void MergeAdjustmentDown(ActorAdjustmentsPtr adjustments, UInt32 handle)
	{
		UInt32 remove = adjustments->MergeAdjustmentDown(handle);
		if (!remove)
			return;

		adjustments->RemoveAdjustment(remove);
		adjustments->UpdateAllAdjustments();
	}

	SafMessagingInterface safMessagingInterface{
		&g_adjustmentManager,

		GetAdjustment,
		GetActorAdjustment,

		SetTransform,
		NegateTransform,
		RotateTransformXYZ,

		CreateAdjustment,
		SaveAdjustment,
		LoadAdjustment,
		RemoveAdjustment,
		ResetAdjustment,
		SavePose,
		SaveOSPose,
		LoadPose,
		ResetPose,
		LoadSkeletonAdjustment,
		MoveAdjustment,
		RenameAdjustment,
		LoadTongueAdjustment,
		ScaleAdjustment,
		MergeAdjustmentDown
	};

	void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		//case F4SEMessagingInterface::kMessage_PostLoad:
			//if (safMessaging.messaging)
			//	safMessaging.messaging->RegisterListener(safMessaging.pluginHandle, nullptr, SAFMessageHandler);
			//break;
		case F4SEMessagingInterface::kMessage_GameLoaded:
			GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&safEventReciever);
			GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&safEventReciever);
			//GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(&safEventReciever);
			break;
		case F4SEMessagingInterface::kMessage_GameDataReady:
			g_adjustmentManager.LoadFiles();

			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafAdjustmentManager, &safMessagingInterface, sizeof(uintptr_t), nullptr);
			break;
		}
	}
}