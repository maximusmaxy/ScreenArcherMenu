#include "messaging.h"

#include "adjustments.h"
#include "f4se/GameRTTI.h"

namespace SAF {

	SAFMessaging safMessaging;

	const char* safName = "SAF";

	std::string SAFDispatcher::GetNodeKeyName(const NodeKey& nodeKey) {
		return manager->GetNodeKeyName(nodeKey);
	}

	const NodeKey SAFDispatcher::GetNodeKeyFromString(const char* str) {
		return manager->GetNodeKeyFromString(str);
	}

	void SAFDispatcher::CreateAdjustment(UInt32 formId, const char* name) {
		AdjustmentCreateMessage message{ formId, name, modName };
		messaging->Dispatch(pluginHandle, kSafAdjustmentCreate, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle)
	{
		AdjustmentSaveMessage message{ formId, filename, handle };
		messaging->Dispatch(pluginHandle, kSafAdjustmentSave, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::LoadAdjustment(UInt32 formId, const char* filename) {
		AdjustmentCreateMessage message{ formId, filename, modName };
		messaging->Dispatch(pluginHandle, kSafAdjustmentLoad, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::RemoveAdjustment(UInt32 formId, UInt32 handle) {
		AdjustmentMessage message{ formId, handle };
		messaging->Dispatch(pluginHandle, kSafAdjustmentErase, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::ResetAdjustment(UInt32 formId, UInt32 handle) {
		AdjustmentMessage message{ formId, handle };
		messaging->Dispatch(pluginHandle, kSafAdjustmentReset, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::TransformAdjustment(UInt32 formId, UInt32 handle, const NodeKey nodeKey, UInt32 type, float a, float b, float c) {
		AdjustmentTransformMessage message{ formId, handle, nodeKey, type, a, b, c };
		messaging->Dispatch(pluginHandle, kSafAdjustmentTransform, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::CreateActorAdjustments(UInt32 formId) {
		AdjustmentActorMessage message{ formId };
		messaging->Dispatch(pluginHandle, kSafAdjustmentActor, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::NegateAdjustmentGroup(UInt32 formId, UInt32 handle, const char* group) {
		AdjustmentNegateMessage message{ formId, handle, group };
		messaging->Dispatch(pluginHandle, kSafAdjustmentNegate, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::SavePose(UInt32 formId, const char* filename, ExportSkeleton* exports) {
		PoseExportMessage message{ formId, filename, exports };
		messaging->Dispatch(pluginHandle, kSafPoseSave, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::SaveOSPose(UInt32 formId, const char* filename, ExportSkeleton* exports) {
		PoseExportMessage message{ formId, filename, exports };
		messaging->Dispatch(pluginHandle, kSafOSPoseSave, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::LoadPose(UInt32 formId, const char* filename) {
		PoseMessage message{ formId, filename };
		messaging->Dispatch(pluginHandle, kSafPoseLoad, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::ResetPose(UInt32 formId) {
		PoseMessage message{ formId, nullptr };
		messaging->Dispatch(pluginHandle, kSafPoseReset, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::LoadSkeletonAdjustment(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable) {
		SkeletonMessage message{ raceId, isFemale, filename, npc, clear, enable };
		messaging->Dispatch(pluginHandle, kSafSkeletonAdjustmentLoad, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex) {
		MoveMessage message{ formId, fromIndex, toIndex };
		messaging->Dispatch(pluginHandle, kSafAdjustmentMove, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::RenameAdjustment(UInt32 formId, UInt32 handle, const char* name) {
		AdjustmentSaveMessage message{ formId, name, handle };
		messaging->Dispatch(pluginHandle, kSafAdjustmentRename, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::LoadTongueAdjustment(UInt32 formId, TransformMap* transforms) {
		TransformMapMessage message{ formId, transforms };
		messaging->Dispatch(pluginHandle, kSafAdjustmentTongue, &message, sizeof(uintptr_t), safName);
	}

	void SAFDispatcher::ScaleAdjustment(UInt32 formId, UInt32 handle, float scale) {
		AdjustmentTransformMessage message{ formId, handle, NodeKey(), 0, scale };
		messaging->Dispatch(pluginHandle, kSafAdjustmentScale, &message, sizeof(uintptr_t), safName);
	}

	void SAFMessageHandler(F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case kSafAdjustmentCreate:
		{
			auto data = static_cast<AdjustmentCreateMessage*>(msg->data);
			UInt32 result = g_adjustmentManager.CreateNewAdjustment(data->formId, data->name, data->esp);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafAdjustmentSave:
		{
			auto data = static_cast<AdjustmentSaveMessage*>(msg->data);
			g_adjustmentManager.SaveAdjustment(data->formId, data->filename, data->handle);
			break;
		}
		case kSafAdjustmentLoad:
		{
			auto data = static_cast<AdjustmentCreateMessage*>(msg->data);
			UInt32 result = g_adjustmentManager.LoadAdjustment(data->formId, data->name, data->esp);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafAdjustmentErase:
		{
			auto data = static_cast<AdjustmentMessage*>(msg->data);
			g_adjustmentManager.RemoveAdjustment(data->formId, data->handle);
			break;
		}
		case kSafAdjustmentReset:
		{
			auto data = static_cast<AdjustmentMessage*>(msg->data);
			g_adjustmentManager.ResetAdjustment(data->formId, data->handle);
			break;
		}
		case kSafAdjustmentTransform:
		{
			auto data = static_cast<AdjustmentTransformMessage*>(msg->data);
			g_adjustmentManager.SetTransform(data);
			break;
		}
		case kSafAdjustmentActor:
		{
			auto data = static_cast<AdjustmentActorMessage*>(msg->data);
			std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.CreateActorAdjustment(data->formId);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafAdjustmentActor, &adjustments, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafPoseSave:
		{
			auto data = static_cast<PoseExportMessage*>(msg->data);
			g_adjustmentManager.SavePose(data->formId, data->filename, data->exports);
			break;
		}
		case kSafOSPoseSave:
		{
			auto data = static_cast<PoseExportMessage*>(msg->data);
			g_adjustmentManager.SaveOSPose(data->formId, data->filename, data->exports);
			break;
		}
		case kSafPoseLoad:
		{
			auto data = static_cast<PoseMessage*>(msg->data);
			UInt32 result = g_adjustmentManager.LoadPose(data->formId, data->filename);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafPoseReset:
		{
			auto data = static_cast<PoseMessage*>(msg->data);
			g_adjustmentManager.ResetPose(data->formId);
			break;
		}
		case kSafSkeletonAdjustmentLoad:
		{
			auto data = static_cast<SkeletonMessage*>(msg->data);
			g_adjustmentManager.LoadRaceAdjustment(data->raceId, data->isFemale, data->filename, data->npc, data->clear, data->enable);
			break;
		}
		case kSafAdjustmentMove:
		{
			auto data = static_cast<MoveMessage*>(msg->data);
			UInt32 result = g_adjustmentManager.MoveAdjustment(data->formId, data->from, data->to);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafAdjustmentRename:
		{
			auto data = static_cast<AdjustmentSaveMessage*>(msg->data);
			g_adjustmentManager.RenameAdjustment(data->formId, data->handle, data->filename);
			break;
		}
		case kSafAdjustmentTongue:
		{
			auto data = static_cast<TransformMapMessage*>(msg->data);
			g_adjustmentManager.LoadTongueAdjustment(data->formId, data->transforms);
			break;
		}
		case kSafAdjustmentScale:
		{
			auto data = static_cast<AdjustmentTransformMessage*>(msg->data);
			g_adjustmentManager.SetAdjustmentScale(data->formId, data->handle, data->a);
			break;
		}
		}
	}

	class SAFEventReciever :
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

	SAFEventReciever safEventReciever;

	void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case F4SEMessagingInterface::kMessage_PostLoad:
			if (safMessaging.messaging)
				safMessaging.messaging->RegisterListener(safMessaging.pluginHandle, nullptr, SAFMessageHandler);
			break;
		case F4SEMessagingInterface::kMessage_GameLoaded:
			GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&safEventReciever);
			GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&safEventReciever);
			//GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(&safEventReciever);
			break;
		case F4SEMessagingInterface::kMessage_GameDataReady:
			g_adjustmentManager.LoadFiles();
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafAdjustmentManager, &g_adjustmentManager, sizeof(uintptr_t), nullptr);
			break;
		}
	}

	void SAFDispatcher::Recieve(F4SEMessagingInterface::Message* msg) {
		switch (msg->type)
		{
		case kSafAdjustmentManager:
			manager = static_cast<AdjustmentManager*>(msg->data);
			break;
		case kSafAdjustmentActor:
			actorAdjustments = *(std::shared_ptr<ActorAdjustments>*)msg->data;
			break;
		case kSafResult:
			result = *(UInt32*)msg->data;
			break;
		}
	}

	std::shared_ptr<ActorAdjustments> SAFDispatcher::GetActorAdjustments(UInt32 formId) {
		std::lock_guard<std::mutex> lock(mutex);

		if (!manager)
			return nullptr;

		std::shared_ptr<ActorAdjustments> adjustments = manager->GetActorAdjustments(formId);

		if (!adjustments) {
			CreateActorAdjustments(formId);
			adjustments = actorAdjustments;
			actorAdjustments = nullptr;
		}

		return adjustments;
	}

	UInt32 SAFDispatcher::GetResult() {
		UInt32 _result = result;
		result = 0;
		return _result;
	}
}