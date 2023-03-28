#pragma once

#include "adjustments.h"

namespace SAF {
	struct SafMessagingInterface {
		AdjustmentManager* manager;

		AdjustmentPtr(*GetAdjustment)(UInt32 formId, UInt32 handle);
		ActorAdjustmentsPtr(*GetActorAdjustments)(UInt32 formId);

		void (*SetTransform)(AdjustmentPtr adjustment, const NodeKey& nodeKey, NiTransform& transform);
		void (*NegateTransform)(ActorAdjustmentsPtr, AdjustmentPtr, NodeKey& nodeKey);
		void (*RotateTransformXYZ)(ActorAdjustmentsPtr, AdjustmentPtr, NodeKey& nodeKey, UInt32 type, float scalar);

		UInt32 (*CreateAdjustment)(ActorAdjustmentsPtr, const char* name, const char* mod);
		UInt32 (*SaveAdjustment)(ActorAdjustmentsPtr, UInt32 handle, const char* filename);
		UInt32 (*LoadAdjustment)(ActorAdjustmentsPtr, const char* filename);
		void (*RemoveAdjustment)(ActorAdjustmentsPtr, UInt32 handle);
		void (*ResetAdjustment)(AdjustmentPtr);
		void (*SavePose)(ActorAdjustmentsPtr, const char* filename, ExportSkeleton* exports);
		void (*SaveOSPose)(ActorAdjustmentsPtr, const char* filename, ExportSkeleton* exports);
		UInt32 (*LoadPose)(ActorAdjustmentsPtr, const char* filename);
		void (*ResetPose)(ActorAdjustmentsPtr);
		void (*LoadSkeletonAdjustment)(UInt32 formId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
		UInt32 (*MoveAdjustment)(ActorAdjustmentsPtr, SInt32 from, SInt32 to);
		void (*RenameAdjustment)(AdjustmentPtr, const char* name);
		void (*LoadTongueAdjustment)(ActorAdjustmentsPtr, TransformMap* transforms);
		void (*ScaleAdjustment)(AdjustmentPtr, float scale);
		void (*MergeAdjustmentDown)(ActorAdjustmentsPtr, UInt32 handle);
		//void (*MirrorAdjustment)(ActorAdjustmentsPtr, AdjustmentPtr);
	};

	enum {
		kSafAdjustmentManager = 1,
	};

	extern SafMessagingInterface safMessagingInterface;

	class SafMessaging
	{
	public:
		PluginHandle pluginHandle;
		F4SEMessagingInterface* messaging;
		F4SEPapyrusInterface* papyrus;
		F4SESerializationInterface* serialization;

		SafMessaging() :
			pluginHandle(kPluginHandle_Invalid),
			messaging(nullptr),
			papyrus(nullptr),
			serialization(nullptr)
		{};
	};

	extern SafMessaging safMessaging;

	void F4SEMessageHandler(F4SEMessagingInterface::Message* msg);
}