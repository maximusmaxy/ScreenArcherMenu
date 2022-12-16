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

#include <memory>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>

namespace SAF {

	class Adjustment;
	class ActorAdjustment;
	class AdjustmentManager;

	struct NodeKey {
		BSFixedString name;
		bool offset;
		std::size_t key;

		NodeKey() : name(BSFixedString()), offset(false), key(0) {}

		NodeKey(BSFixedString name, bool offset) :
			name(name),
			offset(offset),
			//data is an aligned ptr so +1 should be safe?
			key(reinterpret_cast<std::size_t>(name.data) + (offset ? 0 : 1))
		{}

		void SetOffset(bool _offset) {
			offset = _offset;
			key = reinterpret_cast<std::size_t>(name.data) + (offset ? 0 : 1);
		}
	};

	const NodeKey GetNodeKeyFromString(const char*);
	std::string GetNodeKeyName(const NodeKey& nodeKey);

	struct NodeKeyHash {
		std::size_t operator()(const NodeKey& nodeKey) const
		{
			return nodeKey.key;
		}
	};

	struct NodeKeyEqual {
		bool operator()(const NodeKey& lhs, const NodeKey& rhs) const
		{
			return lhs.key == rhs.key;
		}
	};

	struct BSFixedStringHash {
		std::size_t operator()(const BSFixedString& str) const
		{
			return reinterpret_cast<std::size_t>(str.data);
		}
	};

	struct BSFixedStringKeyEqual {
		bool operator()(const BSFixedString& lhs, const BSFixedString& rhs) const
		{
			return lhs == rhs;
		}
	};

	typedef std::unordered_set<NodeKey, NodeKeyHash, NodeKeyEqual> NodeSet;
	typedef std::unordered_set<BSFixedString, BSFixedStringHash, BSFixedStringKeyEqual> BSFixedStringSet;
	typedef std::unordered_map<NodeKey, SamTransform, NodeKeyHash, NodeKeyEqual> TransformMap;
	typedef std::unordered_map<BSFixedString, BSFixedString, BSFixedStringHash, BSFixedStringKeyEqual> BSFixedStringMap;
	typedef std::unordered_map<BSFixedString, NiAVObject*, BSFixedStringHash, BSFixedStringKeyEqual> NodeMap;
	typedef std::unordered_map<BSFixedString, NodeKey, BSFixedStringHash, BSFixedStringKeyEqual> NodeKeyMap;

	SamTransform* GetFromTransformMap(TransformMap& map, const NodeKey& key);
	BSFixedString* GetFromBSFixedStringMap(BSFixedStringMap& map, const BSFixedString& key);
	NiAVObject* GetFromNodeMap(NodeMap& map, const BSFixedString& key);

	bool TransformMapIsDefault(SAF::TransformMap& map);

	struct CaseInsensitiveCompare {
		bool operator() (const std::string& a, const std::string& b) const
		{
			return _stricmp(a.c_str(), b.c_str()) < 0;
		}
	};

	typedef std::map<std::string, TransformMap, CaseInsensitiveCompare> InsensitiveTransformMap;
	typedef std::set<std::string, CaseInsensitiveCompare> InsensitiveStringSet;
	typedef std::map<std::string, UInt32, CaseInsensitiveCompare> InsensitiveUInt32Map;

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
		kAdjustmentTypeTongue,
		kAdjustmentTypeAutoSkeleton,	//Added in 1.1
		kAdjustmentTypeAutoRace			//Added in 1.1
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

	struct LoadedAdjustment
	{
		TransformMap* map;
		UInt32 updateType;

		LoadedAdjustment() : map(nullptr), updateType(kAdjustmentUpdateNone) {}
		LoadedAdjustment(TransformMap* map) : map(map), updateType(kAdjustmentUpdateNone) {}
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

		PersistentAdjustment(std::string file, UInt8 type) :
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
		TransformMap transforms;
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
		kSafPoseLoad,
		kSafPoseReset,
		kSafResult,
		kSafDefaultAdjustmentLoad,
		kSafAdjustmentRotate,
		kSafAdjustmentMove,
		kSafAdjustmentRename,
		kSafAdjustmentTongue
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
		InsensitiveStringSet* autoRace;
		InsensitiveStringSet* autoActor;
		std::vector<PersistentAdjustment>* persistents;

		AdjustmentUpdateData() : race(nullptr), persistents(nullptr), autoRace(nullptr), autoActor(nullptr) {};
	};

	struct ExportSkeleton
	{
		const char* skeleton;
		std::unordered_set<UInt32> handles;
		NodeSet* nodes;

		ExportSkeleton() : skeleton(nullptr), nodes(nullptr) {};
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

		SamTransform* GetTransform(const NodeKey& key);
		SamTransform GetTransformOrDefault(const NodeKey& key);

		SamTransform* GetScaledTransform(const NodeKey& key);
		SamTransform GetScaledTransformOrDefault(const NodeKey& key);

		void SetTransform(const NodeKey& name, SamTransform& transform);
		bool HasTransform(const NodeKey& name);
		void ResetTransform(const NodeKey& name);

		void SetTransformPos(const NodeKey& name, float x, float y, float z);
		void SetTransformRot(const NodeKey& name, float yaw, float pitch, float roll);
		void SetTransformSca(const NodeKey& name, float scale);

		TransformMap* GetMap();
		void SetMap(TransformMap map);
		void CopyMap(TransformMap* map, NodeSet* set);

		void ForEachTransform(const std::function<void(const NodeKey*, SamTransform*)>& functor);
		void ForEachTransformOrDefault(const std::function<void(const NodeKey*, SamTransform*)>& functor, NodeSet* nodeset);

		void Rename(std::string name);
		void SetScale(float scale);
		void UpdateScale(const NodeKey& name, SamTransform& transform);

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
		TransformMap* baseMap = nullptr;

		ActorAdjustments() {}

		ActorAdjustments(Actor* actor, TESNPC* npc) :
			actor(actor),
			npc(npc),
			formId(actor->formID)
		{}

		ActorAdjustmentState IsValid();
		UInt64 GetRaceGender();

		std::shared_ptr<Adjustment> CreateAdjustment(std::string name);
		UInt32 CreateAdjustment(std::string name, std::string esp);
		std::shared_ptr<Adjustment> GetAdjustment(UInt32 handle);
		std::shared_ptr<Adjustment> GetListAdjustment(UInt32 index);
		void RemoveAdjustment(UInt32 handle);
		void RemoveAdjustment(std::string name);
		bool HasAdjustment(std::string name);
		std::unordered_set<std::string> GetAdjustmentNames();
		UInt32 GetAdjustmentIndex(UInt32 handle);
		UInt32 MoveAdjustment(UInt32 fromIndex, UInt32 toIndex);

		std::shared_ptr<Adjustment> GetFile(std::string filename);
		std::shared_ptr<Adjustment> GetFileOrCreate(std::string filename);
		void RemoveFile(std::string filename, UInt32 handle);

		void Clear();
		void UpdatePersistentAdjustments(AdjustmentUpdateData& data);
		void GetOffsetTransform(BSFixedString name, SamTransform* offsetTransform);
		void GetPoseTransforms(BSFixedString name, SamTransform* offsetTransform, SamTransform* poseTransform);
		void GetPoseTransforms(BSFixedString name, SamTransform* offsetTransform, SamTransform* poseTransform,
			std::vector<std::shared_ptr<Adjustment>>& adjustmentList);
		void UpdateNode(BSFixedString name);
		void UpdateAllAdjustments();
		void UpdateAllAdjustments(std::shared_ptr<Adjustment> adjustment);
		
		void UpdateAdjustmentVersion(TransformMap* map, UInt32 updateType);
		std::shared_ptr<Adjustment> LoadAdjustment(std::string filename, bool cached = false);
		UInt32 LoadAdjustmentHandle(std::string filename, std::string espName, bool cached = false);
		void SaveAdjustment(std::string filename, UInt32 handle);
		bool LoadUpdatedAdjustment(std::string filename, TransformMap* map);

		void ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor);

		bool RemoveMod(BSFixedString espName);

		bool HasNode(BSFixedString name);
		bool IsNodeOffset(NodeKey& nodeKey);
		void NegateTransform(std::shared_ptr<Adjustment> adjustment, NodeKey& name);
		void OverrideTransform(std::shared_ptr<Adjustment> adjustment, const NodeKey& name, SamTransform& transform);
		void RotateTransformXYZ(std::shared_ptr<Adjustment> adjustment, NodeKey& name, UInt32 type, float scalar);

		void LoadRaceAdjustment(std::string filename, bool clear, bool enable);
		void RemoveAdjustmentsByType(UInt32 type, bool checkRace);
		UInt32 GetHandleByType(UInt32 type);
		std::shared_ptr<Adjustment> GetAdjustmentByType(UInt32 type);
		bool ShouldRemoveFile(std::shared_ptr<Adjustment> adjustment);

		void SavePose(std::string filename, ExportSkeleton* nodes);
		void SaveOutfitStudioPose(std::string filename, ExportSkeleton* nodes);
		UInt32 LoadPose(std::string filename);
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

		std::unordered_map<UInt64, NodeSets> nodeSets;

		std::unordered_map<UInt64, InsensitiveStringSet> raceAdjustments;
		PersistentMap persistentAdjustments;

		std::unordered_map<NiNode*, NodeMapRef> nodeMapCache;
		std::unordered_map<UInt32, std::shared_ptr<ActorAdjustments>> actorAdjustmentCache;
		InsensitiveTransformMap adjustmentFileCache;

		std::unordered_map<UInt64, InsensitiveStringSet> autoRaceCache;
		std::unordered_map<UInt64, InsensitiveStringSet> autoActorCache;

		void LoadFiles();
		void GameLoaded();
		void ActorLoaded(Actor* actor, bool loaded);
		bool UpdateActor(std::shared_ptr<ActorAdjustments> adjustments);
		bool UpdateActorCache(std::shared_ptr<ActorAdjustments> adjustments);
		bool UpdateActorNodes(std::shared_ptr<ActorAdjustments> adjustments);
		bool CopyActor(std::shared_ptr<ActorAdjustments> src, std::shared_ptr<ActorAdjustments> dst);
		bool CopyActorCache(std::shared_ptr<ActorAdjustments> adjustments);

		UInt32 CreateNewAdjustment(UInt32 formId, const char* name, const char* mod);
		void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		UInt32 LoadAdjustment(UInt32 formId, const char* filename, const char* mod);
		void RemoveAdjustment(UInt32 formId, UInt32 handle);
		void ResetAdjustment(UInt32 formId, UInt32 handle);
		void SetTransform(AdjustmentTransformMessage* message);
		std::shared_ptr<ActorAdjustments> CreateActorAdjustment(UInt32 formId);
		//void NegateAdjustments(UInt32 formId, UInt32 handle, const char* groupName);
		UInt32 LoadPose(UInt32 formId, const char* filename);
		void ResetPose(UInt32 formId);
		void LoadRaceAdjustment(UInt32 formId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
		UInt32 MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex);
		void RenameAdjustment(UInt32 formId, UInt32 handle, const char* name);
		void LoadTongueAdjustment(UInt32 formId, TransformMap* transforms);
		
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(TESObjectREFR* refr);
		void ForEachActorAdjustments(const std::function<void(std::shared_ptr<ActorAdjustments> adjustments)>& functor);

		NodeSets* GetNodeSets(UInt32 race, bool isFemale);

		TransformMap* GetAdjustmentFile(std::string);
		TransformMap* SetAdjustmentFile(std::string filename, TransformMap& map);

		InsensitiveStringSet* GetRaceAdjustments(UInt32 race, bool isFemale);
		InsensitiveStringSet* GetAutoRaceAdjustments(UInt32 race, bool isFemale);
		InsensitiveStringSet* GetAutoActorAdjustments(UInt32 formId);

		bool HasFile(UInt32 race, bool isFemale, UInt32 formId, std::string filename);

		std::vector<PersistentAdjustment>* GetPersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments);

		void CreateNodeMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings, NodeMap* poseMap, NodeMap* offsetMap);
		TransformMap* GetCachedTransformMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings);
		void RemoveNodeMap(NiNode* root);
		
		void RemoveMod(BSFixedString espName);
		void RemoveAllAdjustments();
		
		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);

		void LoadPersistentIfValid(UInt32 formId, UInt64 raceGenderId, PersistentAdjustment persistent, bool loaded);
		void StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments);
		bool StorePersistentIfValid(PersistentMap& map, std::shared_ptr<ActorAdjustments> adjustments);
		//void StorePersistentIfValid(PersistentMap& map, UInt32 id, std::vector<PersistentAdjustment>& persistents);
		bool IsPersistentValid(PersistentAdjustment& persistent);
		void ValidatePersistents(PersistentMap& map, UInt32 formId, UInt64 raceGenderId);
	};

	extern AdjustmentManager g_adjustmentManager;

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

	void SaveCallback(const F4SESerializationInterface* ifc);
	void LoadCallback(const F4SESerializationInterface* ifc);
	void RevertCallback(const F4SESerializationInterface* ifc);

	class SAFDispatcher
	{
	private:
		std::mutex mutex;
	public:
		AdjustmentManager* manager;
		std::shared_ptr<ActorAdjustments> actorAdjustments;
		UInt32 result = 0;

		PluginHandle handle;
		F4SEMessagingInterface* messaging;
		const char* modName;

		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);
		UInt32 GetResult();

		void Recieve(F4SEMessagingInterface::Message* msg);

		void CreateAdjustment(UInt32 formId, const char* name);
		void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		void LoadAdjustment(UInt32 formId, const char* filename);
		void RemoveAdjustment(UInt32 formId, UInt32 handle);
		void ResetAdjustment(UInt32 formId, UInt32 handle);
		void TransformAdjustment(UInt32 formId, UInt32 handle, const NodeKey nodeKey, UInt32 type, float a, float b, float c);
		void CreateActorAdjustments(UInt32 formId);
		void NegateAdjustmentGroup(UInt32 formId, UInt32 handle, const char* group);
		void LoadPose(UInt32 formId, const char* filename);
		void ResetPose(UInt32 formId);
		void LoadDefaultAdjustment(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
		void MoveAdjustment(UInt32 formId, UInt32 from, UInt32 to);
		void RenameAdjustment(UInt32 formId, UInt32 handle, const char* name);
		void LoadTongueAdjustment(UInt32 formId, TransformMap* transforms);
	};
}