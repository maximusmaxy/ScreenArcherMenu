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
	class ActorAdjustments;
	class AdjustmentManager;

	typedef std::shared_ptr<Adjustment> AdjustmentPtr;
	typedef std::shared_ptr<ActorAdjustments> ActorAdjustmentsPtr;

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

	struct ExportSkeleton
	{
		const char* skeleton;
		std::unordered_set<UInt32> handles;
		NodeSet* nodes;

		ExportSkeleton() : skeleton(nullptr), nodes(nullptr) {};
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

		PersistentAdjustment(AdjustmentPtr adjustment, UInt32 updateType);

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

		BSFixedStringSet center;
		BSFixedStringMap mirror;
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
		void SetPoseTransform(BSFixedString name, NiTransform& transform);
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
		//void UpdateScale(const NodeKey& name, NiTransform& transform);

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

		std::vector<AdjustmentPtr> list;
		std::unordered_map<UInt32, AdjustmentPtr> map;

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

		AdjustmentPtr CreateAdjustment(const char* name);
		UInt32 CreateAdjustment(const char* name, const char* espName);
		AdjustmentPtr GetAdjustment(UInt32 handle);
		AdjustmentPtr GetListAdjustment(UInt32 index);
		void RemoveAdjustment(UInt32 handle);
		void RemoveAdjustment(const char* name);
		bool HasAdjustment(const char* name);
		//std::unordered_set<std::string> GetAdjustmentNames();
		UInt32 GetAdjustmentIndex(UInt32 handle);
		UInt32 MoveAdjustment(SInt32 fromIndex, SInt32 toIndex);
		UInt32 MergeAdjustmentDown(UInt32 handle);

		bool SetMirroredTransform(AdjustmentPtr adjustment, BSFixedString name, NiTransform* result);
		bool SwapMirroredTransform(AdjustmentPtr adjustment,
			BSFixedString left, BSFixedString right, NiTransform* leftResult, NiTransform* rightResult);
		bool MirrorAdjustment(UInt32 handle);

		AdjustmentPtr GetFile(const char* filename);
		AdjustmentPtr GetFileOrCreate(const char* filename);
		void RemoveFile(const char* filename, UInt32 handle);

		void Clear();
		void UpdatePersistentAdjustments(AdjustmentUpdateData& data);
		void GetOffsetTransform(BSFixedString name, NiTransform* offsetTransform);
		void GetPoseTransforms(BSFixedString name, NiTransform* offsetTransform, NiTransform* poseTransform);
		void GetPoseTransforms(BSFixedString name, NiTransform* offsetTransform, NiTransform* poseTransform,
			std::vector<AdjustmentPtr>& adjustmentList);
		void UpdateNode(BSFixedString name);
		void UpdateAllAdjustments();
		void UpdateAllAdjustments(AdjustmentPtr adjustment);
		
		void UpdateAdjustmentVersion(TransformMap* map, UInt32 updateType);
		AdjustmentPtr LoadAdjustment(const char* filename, bool cached = false);
		AdjustmentPtr LoadAdjustmentPath(const char* filename, bool cached = false);
		UInt32 LoadAdjustmentHandle(const char* filename, const char* espName, bool cached = false);
		UInt32 LoadAdjustmentPathHandle(const char* filename, const char* espName, bool cached = false);
		UInt32 SaveAdjustment(const char* filename, UInt32 handle);
		bool LoadUpdatedAdjustment(const char* filename, TransformMap* map);
		bool CacheUpdatedAdjustment(const char* filename);

		void ForEachAdjustment(const std::function<void(AdjustmentPtr)>& functor);
		AdjustmentPtr FindAdjustment(const std::function<bool(AdjustmentPtr)>& functor);

		bool RemoveMod(BSFixedString espName);

		bool HasNode(BSFixedString name);
		bool IsNodeOffset(NodeKey& nodeKey);
		void NegateTransform(AdjustmentPtr adjustment, NodeKey& name);
		void OverrideTransform(AdjustmentPtr adjustment, const NodeKey& name, NiTransform& transform);
		void RotateTransformXYZ(AdjustmentPtr adjustment, NodeKey& name, UInt32 type, float scalar);

		void LoadRaceAdjustment(const char* name, bool clear, bool enable);
		void RemoveAdjustmentsByType(UInt32 type, bool checkRace);
		UInt32 GetHandleByType(UInt32 type);
		AdjustmentPtr GetAdjustmentByType(UInt32 type);
		bool ShouldRemoveFile(AdjustmentPtr adjustment);

		void SavePose(const char* filename, ExportSkeleton* nodes);
		void SaveOutfitStudioPose(const char* filename, ExportSkeleton* nodes);
		UInt32 LoadPose(const char* filename);
		void ResetPose();

		void GetPersistentAdjustments(std::vector<PersistentAdjustment>& persistentAdjustments);
	};

	class AdjustmentManager
	{
	public:
		std::shared_mutex actorMutex;
		std::shared_mutex fileMutex;
		std::shared_mutex persistenceMutex;
		std::shared_mutex nodeMapMutex;

		bool filesLoaded = false;
		bool gameLoaded = false;
		Settings settings;
		BSFixedStdStringMap nodeCasingMap;

		std::unordered_map<UInt64, NodeSets> nodeSets;

		std::unordered_map<UInt64, InsensitiveStringSet> raceAdjustments;
		PersistentMap persistentAdjustments;

		std::unordered_map<NiNode*, NodeMapRef> nodeMapCache;
		std::unordered_map<UInt32, ActorAdjustmentsPtr> actorAdjustmentCache;
		InsensitiveTransformMap adjustmentFileCache;

		std::unordered_map<UInt64, InsensitiveStringSet> defaultRaceCache;
		std::unordered_map<UInt64, InsensitiveStringSet> defaultActorCache;

		std::string GetNodeKeyName(const NodeKey& nodeKey);
		const NodeKey GetNodeKeyFromString(const char* str);

		void LoadFiles();
		void GameLoaded();
		void ActorLoaded(Actor* actor, bool loaded);
		bool UpdateActor(ActorAdjustmentsPtr adjustments);
		bool UpdateActorCache(ActorAdjustmentsPtr adjustments);
		bool UpdateActorNodes(ActorAdjustmentsPtr adjustments);
		bool CopyActor(ActorAdjustmentsPtr src, ActorAdjustmentsPtr dst);
		bool CopyActorCache(ActorAdjustmentsPtr adjustments);

		AdjustmentPtr GetAdjustment(UInt32 formId, UInt32 handle);

		//UInt32 CreateNewAdjustment(UInt32 formId, const char* name, const char* mod);
		//void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		//UInt32 LoadAdjustment(UInt32 formId, const char* filename, const char* mod);
		//void RemoveAdjustment(UInt32 formId, UInt32 handle);
		//void ResetAdjustment(UInt32 formId, UInt32 handle);
		//void SetTransform(AdjustmentSetMessage* message);
		//void SetTransform(AdjustmentTransformMessage* message);
		ActorAdjustmentsPtr CreateActorAdjustment(UInt32 formId);
		//void SavePose(UInt32 formId, const char* filename, ExportSkeleton* exports);
		//void SaveOSPose(UInt32 formId, const char* filename, ExportSkeleton* exports);
		//UInt32 LoadPose(UInt32 formId, const char* filename);
		//void ResetPose(UInt32 formId);
		void LoadRaceAdjustment(UInt32 formId, bool isFemale, const char* path, bool npc, bool clear, bool enable);
		//UInt32 MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex);
		//void RenameAdjustment(UInt32 formId, UInt32 handle, const char* name);
		void LoadTongueAdjustment(ActorAdjustmentsPtr adjustments, TransformMap* transforms);
		//void SetAdjustmentScale(UInt32 formId, UInt32 handle, float scale);
		//void MergeAdjustmentDown(UInt32 formId, UInt32 handle);
		//void MirrorAdjustment(UInt32 formId, UInt32 handle);
		
		ActorAdjustmentsPtr GetActorAdjustments(UInt32 formId);
		ActorAdjustmentsPtr GetActorAdjustments(TESObjectREFR* refr);
		void ForEachActorAdjustments(const std::function<void(ActorAdjustmentsPtr adjustments)>& functor);

		NodeSets* GetNodeSets(UInt32 race, bool isFemale);

		TransformMap* GetAdjustmentFile(const char* filename);
		TransformMap* SetAdjustmentFile(const char* filename, TransformMap& map);

		InsensitiveStringSet* GetRaceAdjustments(UInt32 race, bool isFemale);
		InsensitiveStringSet* GetDefaultRaceAdjustments(UInt32 race, bool isFemale);
		InsensitiveStringSet* GetDefaultActorAdjustments(UInt32 formId);

		bool HasFile(UInt32 race, bool isFemale, UInt32 formId, const char* filename);

		std::vector<PersistentAdjustment>* GetPersistentAdjustments(ActorAdjustmentsPtr adjustments);

		void CreateNodeMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings, NodeMap* poseMap, NodeMap* offsetMap);
		StringTransformMap* GetCachedTransformMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings);
		void RemoveNodeMap(NiNode* root);
		
		void RemoveMod(BSFixedString espName);
		void RemoveAllAdjustments();
		
		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);

		void LoadPersistentIfValid(UInt32 formId, UInt64 raceGenderId, PersistentAdjustment& persistent);
		void StorePersistentAdjustments(ActorAdjustmentsPtr adjustments);
		bool StorePersistentIfValid(PersistentMap& map, ActorAdjustmentsPtr adjustments);
		//void StorePersistentIfValid(PersistentMap& map, UInt32 id, std::vector<PersistentAdjustment>& persistents);
		bool IsPersistentValid(PersistentAdjustment& persistent);
		void ValidatePersistents(PersistentMap& map, UInt32 formId, UInt64 raceGenderId);
	};

	extern AdjustmentManager g_adjustmentManager;

	void SaveCallback(const F4SESerializationInterface* ifc);
	void LoadCallback(const F4SESerializationInterface* ifc);
	void RevertCallback(const F4SESerializationInterface* ifc);
}