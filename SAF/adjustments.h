#pragma once

#include "f4se/NiObjects.h"
#include "f4se/NiNodes.h"
#include "f4se/NiTypes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"

#include "serialization.h"
#include "conversions.h"
#include "util.h"
#include "types.h"
#include "settings.h"

#include <memory>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>

namespace SAF {

	class Adjustment;
	class ActorAdjustment;
	class AdjustmentManager;

	typedef std::unordered_map<NodeKey, NiTransform, NodeKeyHash, NodeKeyEqual> TransformMap;
	typedef std::unordered_map<BSFixedString, NiTransform, BSFixedStringHash, BSFixedStringKeyEqual> StringTransformMap;
	typedef std::map<std::string, TransformMap, CaseInsensitiveCompareStr> InsensitiveTransformMap;

	NiTransform* GetFromTransformMap(TransformMap& map, const NodeKey& key);
	NiTransform* GetFromBaseMap(StringTransformMap& map, const BSFixedString& key);
	BSFixedString* GetFromBSFixedStringMap(BSFixedStringMap& map, const BSFixedString& key);
	NiAVObject* GetFromNodeMap(NodeMap& map, const BSFixedString& key);

	bool TransformMapIsDefault(SAF::TransformMap& map);
	std::string GetAdjustmentNameFromPath(const char* path);

	//Old adjustment serialization types
	//enum {
	//	kAdjustmentSerializeDisabled = 0,
	//	kAdjustmentSerializeAdd,		//Adjustments that are not saved in a json file
	//	kAdjustmentSerializeLoad,		//Adjustments saved in a json file
	//	kAdjustmentSerializeRemove,		//Default/Unique adjustments that are removed
	//	kAdjustmentSerializeDefault,	//v0.5 Need to store default adjustments for ordering purposes
	//};

	//Using this in version 1.0+
	enum {
		kAdjustmentTypeNone = 0,
		kAdjustmentTypeDefault,
		kAdjustmentTypeSkeleton,
		kAdjustmentTypeRemovedFile,
		kAdjustmentTypeRace,
		kAdjustmentTypePose,
		kAdjustmentTypeTongue
	};

	enum {
		kAdjustmentUpdateNone = 0,
		kAdjustmentUpdateSerialization,	//Update adjustment serialization to version 1.0+
		kAdjustmentUpdateFile			//Update adjustment file to version 1.0+
	};

	struct AdjustmentMessage {
		UInt32 formId;
		UInt32 handle;
	};

	struct AdjustmentCreateMessage {
		UInt32 formId;
		const char* name;
		const char* esp;
	};

	struct AdjustmentSaveMessage {
		UInt32 formId;
		const char* filename;
		UInt32 handle;
	};

	enum {
		kAdjustmentTransformPosition = 1,
		kAdjustmentTransformRotation,
		kAdjustmentTransformScale,
		kAdjustmentTransformReset,
		kAdjustmentTransformNegate,
		kAdjustmentTransformRotate
	};

	struct AdjustmentTransformMessage {
		UInt32 formId;
		UInt32 handle;
		NodeKey nodeKey;
		UInt32 type;
		float a;
		float b;
		float c;
	};

	struct AdjustmentActorMessage {
		UInt32 formId;
	};

	struct AdjustmentNegateMessage {
		UInt32 formId;
		UInt32 handle;
		const char* group;
	};

	struct PoseMessage {
		UInt32 formId;
		const char* filename;
	};

	struct SkeletonMessage {
		UInt32 raceId;
		bool isFemale;
		const char* filename;
		bool npc;
		bool clear;
		bool enable;
	};

	struct MoveMessage {
		UInt32 formId;
		UInt32 from;
		UInt32 to;
	};

	struct TransformMapMessage {
		UInt32 formId;
		TransformMap* transforms;
	};

	struct ExportSkeleton
	{
		const char* skeleton;
		std::unordered_set<UInt32> handles;
		NodeSet* nodes;

		ExportSkeleton() : skeleton(nullptr), nodes(nullptr) {};
	};

	struct PoseExportMessage {
		UInt32 formId;
		const char* filename;
		ExportSkeleton* exports;
	};

	struct LoadedAdjustment
	{
		const char* filename;
		TransformMap* map;
		UInt32 updateType;

		LoadedAdjustment() : filename(nullptr), map(nullptr), updateType(kAdjustmentUpdateNone) {}
		LoadedAdjustment(TransformMap* map) : filename(nullptr), map(map), updateType(kAdjustmentUpdateNone) {}
		LoadedAdjustment(TransformMap* map, const char* name) : filename(name), map(map), updateType(kAdjustmentUpdateNone) {}
	};

	class PersistentAdjustment {
	public:
		std::string name;
		std::string file;
		std::string mod;
		float scale;
		UInt8 type;
		bool updated;
		UInt8 updateType;
		TransformMap map;

		PersistentAdjustment() : type(kAdjustmentTypeNone) {}

		PersistentAdjustment(std::shared_ptr<Adjustment> adjustment, UInt32 updateType);

		PersistentAdjustment(const char* file, UInt8 type) :
			name(file),
			file(file),
			mod(std::string()),
			scale(1.0),
			type(type),
			updateType(kAdjustmentUpdateNone),
			updated(false)
		{}

		bool StoreMap();
		bool IsValid();
	};

	typedef std::unordered_map<UInt32, std::unordered_map<UInt64, std::vector<PersistentAdjustment>>> PersistentMap;

	struct NodeMapRef {
		NodeMap map;
		StringTransformMap transforms;
		UInt32 refs;

		NodeMapRef() {}
		NodeMapRef(NodeMap map) : map(map), refs(1) {}
	};

	typedef NiNode* (*GetBaseModelRootNode)(TESNPC* npc, TESObjectREFR* refr);
	extern RelocAddr<GetBaseModelRootNode> getBaseModelRootNode;

	typedef const char* (*GetRefrModelFilename)(TESObjectREFR* refr);
	extern RelocAddr<GetRefrModelFilename> getRefrModelFilename;

	struct ProcessListsActor {
		UInt64 unk1;	//0
		UInt64 unk2;	
		UInt64 unk3;	//10
		UInt64 unk4;
		UInt64 unk5;	//20
		UInt64 unk6;
		UInt64 unk7;	//30
		UInt64 unk8;
		UInt32* handles;	//40
		UInt64 unk10;
		UInt32 count;	//50
	};

	enum {
		kSafAdjustmentManager = 1,
		kSafAdjustmentCreate,
		kSafAdjustmentSave,
		kSafAdjustmentLoad,
		kSafAdjustmentErase,
		kSafAdjustmentReset,
		kSafAdjustmentTransform,
		kSafAdjustmentActor,
		kSafAdjustmentNegate,
		kSafPoseSave,
		kSafOSPoseSave,
		kSafPoseLoad,
		kSafPoseReset,
		kSafResult,
		kSafSkeletonAdjustmentLoad,
		kSafAdjustmentRotate,
		kSafAdjustmentMove,
		kSafAdjustmentRename,
		kSafAdjustmentTongue,
		kSafAdjustmentScale
	};

	class NodeSets
	{
	public:
		NodeSet offsets;
		NodeSet pose;
		NodeSet all;

		BSFixedStringSet baseStrings;
		BSFixedStringSet allStrings;
		BSFixedStringSet baseNodeStrings;

		BSFixedStringMap offsetMap;
		NodeKeyMap nodeKeys;

		BSFixedString rootName;
	};

	struct AdjustmentUpdateData
	{
		InsensitiveStringSet* race;
		InsensitiveStringSet* defaultRace;
		InsensitiveStringSet* defaultActor;
		std::vector<PersistentAdjustment>* persistents;

		AdjustmentUpdateData() : race(nullptr), persistents(nullptr), defaultRace(nullptr), defaultActor(nullptr) {};
	};

	class Adjustment
	{
	private:
		std::shared_mutex mutex;
	
	public:
		UInt32 handle = -1;

		std::string name;
		std::string file;
		std::string mod;
		bool updated = false;
		float scale = 1.0f;
		UInt32 type = kAdjustmentTypeDefault;
		
		TransformMap map;
		TransformMap scaled;
		
		Adjustment() {}

		Adjustment(UInt32 handle, std::string name) :
			handle(handle),
			name(name)
		{}

		NiTransform* GetTransform(const NodeKey& key);
		NiTransform GetTransformOrDefault(const NodeKey& key);

		NiTransform* GetScaledTransform(const NodeKey& key);
		NiTransform GetScaledTransformOrDefault(const NodeKey& key);

		void SetTransform(const NodeKey& name, NiTransform& transform);
		bool HasTransform(const NodeKey& name);
		void ResetTransform(const NodeKey& name);

		void SetTransformPos(const NodeKey& name, float x, float y, float z);
		void SetTransformRot(const NodeKey& name, float yaw, float pitch, float roll);
		void SetTransformSca(const NodeKey& name, float scale);

		TransformMap* GetMap();
		void SetMap(TransformMap map);
		void CopyMap(TransformMap* map, NodeSet* set);
		void UpdateMapScale();

		void ForEachTransform(const std::function<void(const NodeKey*, NiTransform*)>& functor);
		void ForEachTransformOrDefault(const std::function<void(const NodeKey*, NiTransform*)>& functor, NodeSet* nodeset);

		void Rename(const char* name);
		void SetScale(float scale);
		void UpdateScale(const NodeKey& name, NiTransform& transform);

		bool IsVisible();

		void Clear();
	};

	enum ActorAdjustmentState {
		kActorInvalid = 0,
		kActorValid,
		kActorUpdated,
	};

	class ActorAdjustments
	{
	private:
		UInt32 nextHandle = 1; //0 handle is null
		std::shared_mutex mutex;

	public:
		UInt32 race = 0;
		bool isFemale = false;

		Actor* actor = nullptr;
		TESNPC* npc = nullptr;
		NiNode* root = nullptr;
		NiNode* baseRoot = nullptr;
		UInt32 formId = 0;

		std::vector<std::shared_ptr<Adjustment>> list;
		std::unordered_map<UInt32, std::shared_ptr<Adjustment>> map;

		NodeSets* nodeSets = nullptr;
		NodeMap poseMap;
		NodeMap offsetMap;
		StringTransformMap* baseMap = nullptr;

		ActorAdjustments() {}

		ActorAdjustments(Actor* actor, TESNPC* npc) :
			actor(actor),
			npc(npc),
			formId(actor->formID)
		{}

		ActorAdjustmentState IsValid();
		UInt64 GetRaceGender();

		std::shared_ptr<Adjustment> CreateAdjustment(const char* name);
		UInt32 CreateAdjustment(const char* name, const char* espName);
		std::shared_ptr<Adjustment> GetAdjustment(UInt32 handle);
		std::shared_ptr<Adjustment> GetListAdjustment(UInt32 index);
		void RemoveAdjustment(UInt32 handle);
		void RemoveAdjustment(const char* name);
		bool HasAdjustment(const char* name);
		//std::unordered_set<std::string> GetAdjustmentNames();
		UInt32 GetAdjustmentIndex(UInt32 handle);
		UInt32 MoveAdjustment(SInt32 fromIndex, SInt32 toIndex);

		std::shared_ptr<Adjustment> GetFile(const char* filename);
		std::shared_ptr<Adjustment> GetFileOrCreate(const char* filename);
		void RemoveFile(const char* filename, UInt32 handle);

		void Clear();
		void UpdatePersistentAdjustments(AdjustmentUpdateData& data);
		void GetOffsetTransform(BSFixedString name, NiTransform* offsetTransform);
		void GetPoseTransforms(BSFixedString name, NiTransform* offsetTransform, NiTransform* poseTransform);
		void GetPoseTransforms(BSFixedString name, NiTransform* offsetTransform, NiTransform* poseTransform,
			std::vector<std::shared_ptr<Adjustment>>& adjustmentList);
		void UpdateNode(BSFixedString name);
		void UpdateAllAdjustments();
		void UpdateAllAdjustments(std::shared_ptr<Adjustment> adjustment);
		
		void UpdateAdjustmentVersion(TransformMap* map, UInt32 updateType);
		std::shared_ptr<Adjustment> LoadAdjustment(const char* filename, bool cached = false);
		std::shared_ptr<Adjustment> LoadAdjustmentPath(const char* filename, bool cached = false);
		UInt32 LoadAdjustmentHandle(const char* filename, const char* espName, bool cached = false);
		UInt32 LoadAdjustmentPathHandle(const char* filename, const char* espName, bool cached = false);
		void SaveAdjustment(const char* filename, UInt32 handle);
		bool LoadUpdatedAdjustment(const char* filename, TransformMap* map);

		void ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor);

		bool RemoveMod(BSFixedString espName);

		bool HasNode(BSFixedString name);
		bool IsNodeOffset(NodeKey& nodeKey);
		void NegateTransform(std::shared_ptr<Adjustment> adjustment, NodeKey& name);
		void OverrideTransform(std::shared_ptr<Adjustment> adjustment, const NodeKey& name, NiTransform& transform);
		void RotateTransformXYZ(std::shared_ptr<Adjustment> adjustment, NodeKey& name, UInt32 type, float scalar);

		void LoadRaceAdjustment(const char* name, bool clear, bool enable);
		void RemoveAdjustmentsByType(UInt32 type, bool checkRace);
		UInt32 GetHandleByType(UInt32 type);
		std::shared_ptr<Adjustment> GetAdjustmentByType(UInt32 type);
		bool ShouldRemoveFile(std::shared_ptr<Adjustment> adjustment);

		void SavePose(const char* filename, ExportSkeleton* nodes);
		void SaveOutfitStudioPose(const char* filename, ExportSkeleton* nodes);
		UInt32 LoadPose(const char* filename);
		void ResetPose();

		void GetPersistentAdjustments(std::vector<PersistentAdjustment>& persistentAdjustments);
	};

	class AdjustmentManager
	{
	private:
		std::shared_mutex actorMutex;
		std::shared_mutex fileMutex;
		std::shared_mutex persistenceMutex;
		std::shared_mutex nodeMapMutex;

		bool filesLoaded = false;

	public:
		bool gameLoaded = false;
		Settings settings;
		BSFixedStdStringMap nodeCasingMap;

		std::unordered_map<UInt64, NodeSets> nodeSets;

		std::unordered_map<UInt64, InsensitiveStringSet> raceAdjustments;
		PersistentMap persistentAdjustments;

		std::unordered_map<NiNode*, NodeMapRef> nodeMapCache;
		std::unordered_map<UInt32, std::shared_ptr<ActorAdjustments>> actorAdjustmentCache;
		InsensitiveTransformMap adjustmentFileCache;

		std::unordered_map<UInt64, InsensitiveStringSet> defaultRaceCache;
		std::unordered_map<UInt64, InsensitiveStringSet> defaultActorCache;

		std::string GetNodeKeyName(const NodeKey& nodeKey);
		const NodeKey GetNodeKeyFromString(const char* str);

		void LoadFiles();
		void GameLoaded();
		void ActorLoaded(Actor* actor, bool loaded);
		bool UpdateActor(std::shared_ptr<ActorAdjustments> adjustments);
		bool UpdateActorCache(std::shared_ptr<ActorAdjustments> adjustments);
		bool UpdateActorNodes(std::shared_ptr<ActorAdjustments> adjustments);
		bool CopyActor(std::shared_ptr<ActorAdjustments> src, std::shared_ptr<ActorAdjustments> dst);
		bool CopyActorCache(std::shared_ptr<ActorAdjustments> adjustments);

		std::shared_ptr<Adjustment> GetAdjustment(UInt32 formId, UInt32 handle);
		UInt32 CreateNewAdjustment(UInt32 formId, const char* name, const char* mod);
		void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		UInt32 LoadAdjustment(UInt32 formId, const char* filename, const char* mod);
		void RemoveAdjustment(UInt32 formId, UInt32 handle);
		void ResetAdjustment(UInt32 formId, UInt32 handle);
		void SetTransform(AdjustmentTransformMessage* message);
		std::shared_ptr<ActorAdjustments> CreateActorAdjustment(UInt32 formId);
		void SavePose(UInt32 formId, const char* filename, ExportSkeleton* exports);
		void SaveOSPose(UInt32 formId, const char* filename, ExportSkeleton* exports);
		UInt32 LoadPose(UInt32 formId, const char* filename);
		void ResetPose(UInt32 formId);
		void LoadRaceAdjustment(UInt32 formId, bool isFemale, const char* path, bool npc, bool clear, bool enable);
		UInt32 MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex);
		void RenameAdjustment(UInt32 formId, UInt32 handle, const char* name);
		void LoadTongueAdjustment(UInt32 formId, TransformMap* transforms);
		void SetAdjustmentScale(UInt32 formId, UInt32 handle, float scale);
		
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(TESObjectREFR* refr);
		void ForEachActorAdjustments(const std::function<void(std::shared_ptr<ActorAdjustments> adjustments)>& functor);

		NodeSets* GetNodeSets(UInt32 race, bool isFemale);

		TransformMap* GetAdjustmentFile(const char* filename);
		TransformMap* SetAdjustmentFile(const char* filename, TransformMap& map);

		InsensitiveStringSet* GetRaceAdjustments(UInt32 race, bool isFemale);
		InsensitiveStringSet* GetDefaultRaceAdjustments(UInt32 race, bool isFemale);
		InsensitiveStringSet* GetDefaultActorAdjustments(UInt32 formId);

		bool HasFile(UInt32 race, bool isFemale, UInt32 formId, const char* filename);

		std::vector<PersistentAdjustment>* GetPersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments);

		void CreateNodeMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings, NodeMap* poseMap, NodeMap* offsetMap);
		StringTransformMap* GetCachedTransformMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings);
		void RemoveNodeMap(NiNode* root);
		
		void RemoveMod(BSFixedString espName);
		void RemoveAllAdjustments();
		
		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);

		void LoadPersistentIfValid(UInt32 formId, UInt64 raceGenderId, PersistentAdjustment& persistent);
		void StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments);
		bool StorePersistentIfValid(PersistentMap& map, std::shared_ptr<ActorAdjustments> adjustments);
		//void StorePersistentIfValid(PersistentMap& map, UInt32 id, std::vector<PersistentAdjustment>& persistents);
		bool IsPersistentValid(PersistentAdjustment& persistent);
		void ValidatePersistents(PersistentMap& map, UInt32 formId, UInt64 raceGenderId);
	};

	extern AdjustmentManager g_adjustmentManager;

	void SaveCallback(const F4SESerializationInterface* ifc);
	void LoadCallback(const F4SESerializationInterface* ifc);
	void RevertCallback(const F4SESerializationInterface* ifc);

	class SAFMessaging
	{
	public:
		PluginHandle pluginHandle;
		F4SEMessagingInterface* messaging;
		F4SEPapyrusInterface* papyrus;
		F4SESerializationInterface* serialization;

		SAFMessaging() :
			pluginHandle(kPluginHandle_Invalid),
			messaging(nullptr),
			papyrus(nullptr),
			serialization(nullptr)
		{};
	};

	extern SAFMessaging safMessaging;

	void F4SEMessageHandler(F4SEMessagingInterface::Message* msg);

	class SAFDispatcher
	{
	private:
		std::mutex mutex;
	public:
		AdjustmentManager* manager;
		std::shared_ptr<ActorAdjustments> actorAdjustments;
		UInt32 result = 0;

		PluginHandle pluginHandle;
		F4SEMessagingInterface* messaging;
		const char* modName;

		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);
		UInt32 GetResult();

		void Recieve(F4SEMessagingInterface::Message* msg);

		std::string GetNodeKeyName(const NodeKey& nodeKey);
		const NodeKey GetNodeKeyFromString(const char* str);

		void CreateAdjustment(UInt32 formId, const char* name);
		void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		void LoadAdjustment(UInt32 formId, const char* filename);
		void RemoveAdjustment(UInt32 formId, UInt32 handle);
		void ResetAdjustment(UInt32 formId, UInt32 handle);
		void TransformAdjustment(UInt32 formId, UInt32 handle, const NodeKey nodeKey, UInt32 type, float a, float b, float c);
		void CreateActorAdjustments(UInt32 formId);
		void NegateAdjustmentGroup(UInt32 formId, UInt32 handle, const char* group);
		void SavePose(UInt32 formId, const char* filename, ExportSkeleton* exports);
		void SaveOSPose(UInt32 formId, const char* filename, ExportSkeleton* exports);
		void LoadPose(UInt32 formId, const char* filename);
		void ResetPose(UInt32 formId);
		void LoadSkeletonAdjustment(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
		void MoveAdjustment(UInt32 formId, UInt32 from, UInt32 to);
		void RenameAdjustment(UInt32 formId, UInt32 handle, const char* name);
		void LoadTongueAdjustment(UInt32 formId, TransformMap* transforms);
		void ScaleAdjustment(UInt32 formId, UInt32 handle, float scale);
	};
}