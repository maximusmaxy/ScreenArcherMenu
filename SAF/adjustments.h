#pragma once

#include "f4se/NiObjects.h"
#include "f4se/NiNodes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <shared_mutex>

namespace SAF {

	typedef std::unordered_set<std::string> NodeSet;
	typedef std::unordered_map<std::string, NiAVObject*> NodeMap;
	typedef std::unordered_map<std::string, NiTransform> TransformMap;

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
		kSafPoseReset
	};

	struct AdjustmentMessage {
		UInt32 formId;
		UInt32 handle;

		AdjustmentMessage(UInt32 formId, UInt32 handle) :
			formId(formId),
			handle(handle)
		{}
	};

	struct AdjustmentCreateMessage {
		UInt32 formId;
		const char* name;
		const char* mod;
		bool persistent;
		bool hidden;

		AdjustmentCreateMessage(UInt32 formId, const char* name, const char* mod, bool persistent, bool hidden) :
			formId(formId),
			name(name),
			mod(mod),
			persistent(persistent),
			hidden(hidden)
		{}
	};

	struct AdjustmentSaveMessage {
		UInt32 formId;
		const char* filename;
		UInt32 handle;

		AdjustmentSaveMessage(UInt32 formId, const char* filename, UInt32 handle) :
			formId(formId),
			filename(filename),
			handle(handle)
		{}
	};

	enum {
		kAdjustmentTransformPosition = 1,
		kAdjustmentTransformRotation,
		kAdjustmentTransformScale,
		kAdjustmentTransformReset,
		kAdjustmentTransformNegate
	};

	struct AdjustmentTransformMessage {
		UInt32 formId;
		UInt32 handle;
		const char* key;
		UInt32 type;
		float a;
		float b;
		float c;

		AdjustmentTransformMessage(UInt32 formId, UInt32 handle, const char* key, UInt32 type, float a, float b, float c) :
			formId(formId),
			handle(handle),
			key(key),
			type(type),
			a(a),
			b(b),
			c(c)
		{}
	};

	struct AdjustmentActorMessage {
		UInt32 formId;
		const char* mod;

		AdjustmentActorMessage(UInt32 formId, const char* mod) :
			formId(formId),
			mod(mod)
		{}
	};

	struct AdjustmentNegateMessage {
		UInt32 formId;
		UInt32 handle;
		const char* group;

		AdjustmentNegateMessage(UInt32 formId, UInt32 handle, const char* group) :
			formId(formId),
			handle(handle),
			group(group)
		{}
	};

	struct PoseMessage {
		UInt32 formId;
		const char* filename;

		PoseMessage(UInt32 formId, const char* filename) :
			formId(formId),
			filename(filename)
		{}
	};

	class NodeSets
	{
	public:
		NodeSet overrides;
		NodeSet offsets;
		NodeSet all;
		NodeSet base;
		NodeSet allOrBase;
		std::unordered_map<std::string, std::string> baseMap;
		std::unordered_map<std::string, std::string> fixedConversion;
		std::unordered_map<std::string, std::vector<std::string>> groups;
	};

	enum {
		kAdjustmentSerializeDisabled = 0,
		kAdjustmentSerializeAdd,		//Adjustments that are not saved in a json file
		kAdjustmentSerializeLoad,		//Adjustments saved in a json file
		kAdjustmentSerializeRemove,		//Default/Unique adjustments that are removed
	};

	struct PersistentAdjustment {
	public:
		std::string name;
		std::string mod;
		UInt8 type;
		TransformMap map;

		PersistentAdjustment() : type(kAdjustmentSerializeDisabled) {}

		PersistentAdjustment(std::string name, std::string mod, UInt8 type) :
			name(name),
			mod(mod),
			type(type)
		{}

		PersistentAdjustment(std::string name, std::string mod, UInt8 type, TransformMap map) :
			name(name),
			mod(mod),
			type(type),
			map(map)
		{}
	};

	class Adjustment
	{
	private:
		std::shared_mutex mutex;
	
	public:
		
		UInt32 handle = -1;

		std::string name;
		std::string mod;
		bool hidden = false;
		bool isDefault = false;
		bool persistent = false;
		bool saved = false;
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

		void Clear();

		PersistentAdjustment GetPersistence();
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
			npc(npc)
		{}

		std::shared_ptr<Adjustment> CreateAdjustment(std::string name);
		UInt32 CreateAdjustment(std::string name, std::string esp, bool persistent, bool hidden);
		std::shared_ptr<Adjustment> GetAdjustment(UInt32 handle);
		std::shared_ptr<Adjustment> GetListAdjustment(UInt32 index);
		void RemoveAdjustment(UInt32 handle);

		void Update();
		void UpdatePersistentAdjustments(std::vector<std::string>* defaults, std::vector<std::pair<std::string, std::string>>* uniques, std::vector<PersistentAdjustment>* persistents);
		void UpdateAdjustments(std::string name);
		void UpdateAllAdjustments();
		void UpdateAllAdjustments(std::shared_ptr<Adjustment> adjustment);
		
		std::shared_ptr<Adjustment> LoadAdjustment(std::string filename, bool cached = false);
		UInt32 LoadAdjustment(std::string filename, std::string espName, bool persistent, bool hidden, bool cached = false);
		void SaveAdjustment(std::string filename, UInt32 handle);

		void ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor);

		bool RemoveMod(BSFixedString espName);

		void NegateTransform(std::shared_ptr<Adjustment> adjustment, std::string name);
		void OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform);
		void NegateTransformGroup(std::shared_ptr<Adjustment> adjustment, std::string groupName);

		void GetPersistentAdjustments(std::unordered_map<UInt32, std::vector<PersistentAdjustment>>* persistentAdjustments);

		void SavePose(std::string filename, std::unordered_set<UInt32> handles);
		void LoadPose(std::string filename);
		void ResetPose();
	};

	class AdjustmentManager
	{
	private:
		std::shared_mutex actorMutex;
		std::shared_mutex fileMutex;
		std::shared_mutex persistenceMutex;
		std::shared_mutex nodeMapMutex;

		bool loaded = false;

	public:
		std::string offsetPostfix = "_Offset";
		std::string overridePostfix = "_Pose";

		std::unordered_map<UInt64, NodeSets> nodeSets;

		std::unordered_map<UInt64, std::vector<std::string>> defaultAdjustments;
		std::unordered_map<UInt32, std::vector<std::pair<std::string, std::string>>> uniqueAdjustments;
		std::unordered_map<UInt32, std::vector<PersistentAdjustment>> persistentAdjustments;

		std::unordered_map<NiNode*, NodeMapRef> nodeMapCache;
		std::unordered_map<UInt32, std::shared_ptr<ActorAdjustments>> actorAdjustmentCache;
		std::unordered_map<std::string, TransformMap> adjustmentFileCache;

		std::unordered_set<UInt32> actorUpdates;

		void Load();
		void RemoveMod(BSFixedString espName);

		void CreateNewAdjustment(UInt32 formId, const char* name, const char* mod, bool persistent, bool hidden);
		void SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle);
		void LoadAdjustment(UInt32 formId, const char* filename, const char* mod, bool persistent, bool hidden);
		void RemoveAdjustment(UInt32 formId, UInt32 handle);
		void ResetAdjustment(UInt32 formId, UInt32 handle);
		void SetTransform(AdjustmentTransformMessage* message);
		std::shared_ptr<ActorAdjustments> CreateActorAdjustment(UInt32 formId);
		void NegateAdjustments(UInt32 formId, UInt32 handle, const char* groupName);
		void LoadPose(UInt32 formId, const char* filename);
		void ResetPose(UInt32 formId);

		void UpdateActorAdjustments(std::shared_ptr<ActorAdjustments> adjustments, bool loaded);
		void UpdateActor(Actor* actor, TESNPC* npc, bool loaded);
		void UpdateAdjustments(std::shared_ptr<ActorAdjustments> adjustments);
		void UpdateQueue();
		
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(TESObjectREFR* refr);

		NodeSets* GetNodeSets(UInt32 race, bool isFemale);

		TransformMap* GetAdjustmentFile(std::string);
		void SetAdjustmentFile(std::string filename, TransformMap map);

		std::vector<std::string>* GetDefaultAdjustments(UInt32 race, bool isFemale);
		
		NodeMap CreateNodeMap(NiNode* root, NodeSets* set);
		NodeMap* GetCachedNodeMap(std::shared_ptr<ActorAdjustments> actorAdjustments, NodeSets* nodeSet);
		
		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);

		void StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments);
	};

	extern AdjustmentManager g_adjustmentManager;
}