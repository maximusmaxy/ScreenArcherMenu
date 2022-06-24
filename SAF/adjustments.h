#pragma once

#include "f4se/NiObjects.h"
#include "f4se/NiNodes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"

#include "serialization.h"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>

namespace SAF {

	typedef std::unordered_set<std::string> NodeSet;
	typedef std::unordered_map<std::string, NiAVObject*> NodeMap;
	typedef std::unordered_map<std::string, NiTransform> TransformMap;

	enum {
		kAdjustmentSerializeDisabled = 0,
		kAdjustmentSerializeAdd,		//Adjustments that are not saved in a json file
		kAdjustmentSerializeLoad,		//Adjustments saved in a json file
		kAdjustmentSerializeRemove,		//Default/Unique adjustments that are removed
		kAdjustmentSerializeDefault,	//v0.5 Need to store default adjustments for ordering purposes
	};

	class PersistentAdjustment {
	public:
		std::string name;
		std::string file;
		std::string mod;
		float scale;
		UInt8 type;
		TransformMap map;

		PersistentAdjustment() : type(kAdjustmentSerializeDisabled) {}

		PersistentAdjustment(std::string file, UInt8 type) :
			name(file),
			file(file),
			mod(std::string()),
			scale(1.0),
			type(type)
		{}

		PersistentAdjustment(std::string name, std::string file, std::string mod, float scale, UInt8 type) :
			name(name),
			file(file),
			mod(mod),
			scale(scale),
			type(type)
		{}

		PersistentAdjustment(std::string name, std::string file, std::string mod, float scale, UInt8 type, TransformMap map) :
			name(name),
			file(file),
			mod(mod),
			scale(scale),
			type(type),
			map(map)
		{}
	};

	typedef std::unordered_map<UInt32, std::vector<PersistentAdjustment>> PersistentMap;

	struct NodeMapRef {
		NodeMap map;
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
		const char* key;
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

	class NodeSets
	{
	public:
		NodeSet overrides;
		NodeSet offsets;
		NodeSet pose;
		NodeSet all;
		NodeSet base;
		NodeSet allOrBase;
		BSFixedString rootName;

		std::unordered_map<std::string, std::string> baseMap;
		std::unordered_map<std::string, std::string> fixedConversion;
	};

	struct AdjustmentUpdateData
	{
		std::unordered_set<std::string>* defaults;
		std::vector<std::pair<std::string, std::string>>* uniques;
		std::vector<PersistentAdjustment>* persistents;

		AdjustmentUpdateData() : defaults(nullptr), uniques(nullptr), persistents(nullptr) {};
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
		bool isDefault = false;
		bool updated = false;
		float scale = 1.0f;
		
		TransformMap map;
		
		Adjustment() {}

		Adjustment(UInt32 handle, std::string name) :
			handle(handle),
			name(name)
		{}

		NiTransform GetTransform(std::string name);
		NiTransform GetTransformOrDefault(std::string name);
		void SetTransform(std::string name, NiTransform transform);
		bool HasTransform(std::string name);
		void ResetTransform(std::string name);
		NiTransform GetScaledTransform(std::string name, float scale);

		void SetTransformPos(std::string name, float x, float y, float z);
		void SetTransformRot(std::string name, float yaw, float pitch, float roll);
		void SetTransformSca(std::string name, float scale);

		TransformMap GetMap();
		void SetMap(TransformMap map);
		void CopyMap(TransformMap* map, NodeSet* set);

		void ForEachTransform(const std::function<void(std::string, NiTransform*)>& functor);
		void ForEachTransformOrDefault(const std::function<void(std::string, NiTransform*)>& functor, NodeSet* nodeset);

		void Rename(std::string name);
		void SetScale(float scale);

		void Clear();

		PersistentAdjustment GetPersistence();
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
		NodeMap nodeMap;
		NodeMap* nodeMapBase = nullptr;

		std::unordered_set<std::string> removedAdjustments;

		UInt32 poseHandle = 0;

		ActorAdjustments() {}

		ActorAdjustments(Actor* actor, TESNPC* npc) :
			actor(actor),
			npc(npc),
			formId(actor->formID)
		{}

		ActorAdjustmentState IsValid();

		std::shared_ptr<Adjustment> CreateAdjustment(std::string name);
		UInt32 CreateAdjustment(std::string name, std::string esp);
		std::shared_ptr<Adjustment> GetAdjustment(UInt32 handle);
		std::shared_ptr<Adjustment> GetListAdjustment(UInt32 index);
		void RemoveAdjustment(UInt32 handle);
		void RemoveAdjustment(std::string name);
		bool HasAdjustment(std::string name);
		std::unordered_set<std::string> GetAdjustmentNames();
		UInt32 GetAdjustmentIndex(UInt32 handle);
		bool MoveAdjustment(UInt32 fromIndex, UInt32 toIndex);

		void Clear();
		void UpdatePersistentAdjustments(AdjustmentUpdateData& data);
		void UpdateAdjustments(std::string name);
		void UpdateAllAdjustments();
		void UpdateAllAdjustments(std::shared_ptr<Adjustment> adjustment);
		
		std::shared_ptr<Adjustment> LoadAdjustment(std::string filename, bool cached = false);
		UInt32 LoadAdjustmentHandle(std::string filename, std::string espName, bool cached = false);
		void SaveAdjustment(std::string filename, UInt32 handle);

		void ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor);

		bool RemoveMod(BSFixedString espName);

		bool HasNode(std::string name);
		void NegateTransform(std::shared_ptr<Adjustment> adjustment, std::string name);
		void OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform);
		void RotateTransformXYZ(std::shared_ptr<Adjustment> adjustment, std::string name, UInt32 type, float scalar);

		void LoadDefaultAdjustment(std::string filename, bool clear, bool enable);
		void RemoveDefaultAdjustments();

		void SavePose(std::string filename, std::unordered_set<UInt32> handles);
		bool LoadPose(std::string filename);
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

		std::string offsetPostfix = "_Offset";
		std::string overridePostfix = "_Pose";

		std::unordered_map<UInt64, NodeSets> nodeSets;

		std::unordered_map<UInt64, std::unordered_set<std::string>> defaultAdjustments;
		std::unordered_map<UInt32, std::vector<std::pair<std::string, std::string>>> uniqueAdjustments;
		PersistentMap persistentAdjustments;

		std::unordered_map<NiNode*, NodeMapRef> nodeMapCache;
		std::unordered_map<UInt32, std::shared_ptr<ActorAdjustments>> actorAdjustmentCache;
		std::unordered_map<std::string, TransformMap> adjustmentFileCache;

		void LoadFiles();
		void GameLoaded();
		void ActorLoaded(Actor* actor, bool loaded);
		bool UpdateActor(std::shared_ptr<ActorAdjustments> adjustments);
		bool UpdateActorCache(std::shared_ptr<ActorAdjustments> adjustments);

		void CreateNewAdjustment(UInt32 formId, const char* name, const char* mod);
		void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		bool LoadAdjustment(UInt32 formId, const char* filename, const char* mod);
		void RemoveAdjustment(UInt32 formId, UInt32 handle);
		void ResetAdjustment(UInt32 formId, UInt32 handle);
		void SetTransform(AdjustmentTransformMessage* message);
		std::shared_ptr<ActorAdjustments> CreateActorAdjustment(UInt32 formId);
		//void NegateAdjustments(UInt32 formId, UInt32 handle, const char* groupName);
		bool LoadPose(UInt32 formId, const char* filename);
		void ResetPose(UInt32 formId);
		void LoadDefaultAdjustment(UInt32 formId, bool isFemale, const char* filename, bool npc, bool clear, bool enable);
		bool MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex);
		void RenameAdjustment(UInt32 formId, UInt32 handle, const char* name);
		
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(TESObjectREFR* refr);
		void ForEachActorAdjustments(const std::function<void(std::shared_ptr<ActorAdjustments> adjustments)>& functor);

		NodeSets* GetNodeSets(UInt32 race, bool isFemale);

		TransformMap* GetAdjustmentFile(std::string);
		void SetAdjustmentFile(std::string filename, TransformMap map);

		std::unordered_set<std::string>* GetDefaultAdjustments(UInt32 race, bool isFemale);
		
		NodeMap CreateNodeMap(NiNode* root, NodeSets* set);
		NodeMap* GetCachedNodeMap(NiNode* root, NodeSets* set);
		void RemoveNodeMap(NiNode* root);

		void RemoveMod(BSFixedString espName);
		
		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);

		void StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments);
		void StorePersistentIfValid(PersistentMap& map, std::shared_ptr<ActorAdjustments> adjustments);
		bool IsPersistentValid(PersistentAdjustment& persistent);
	};

	extern AdjustmentManager g_adjustmentManager;
}