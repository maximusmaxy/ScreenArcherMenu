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

	typedef NiNode* (*GetBaseModelRootNode)(TESNPC* npc, TESObjectREFR* refr);
	extern RelocAddr<GetBaseModelRootNode> getBaseModelRootNode;

	typedef const char* (*GetRefrModelFilename)(TESObjectREFR* refr);
	extern RelocAddr<GetRefrModelFilename> getRefrModelFilename;

	enum {
		kSafAdjustmentManager = 1
	};

	class NodeSets
	{
	public:
		NodeSet overrides;
		NodeSet offsets;
		NodeSet all;
		std::unordered_map<std::string, std::string> base;
	};

	class Adjustment
	{
	private:
		std::shared_mutex mutex;
		std::unordered_map<std::string, NiTransform> map;

	public:
		std::string name;
		UInt32 handle = -1;
		std::string esp;
		bool hidden = false;
		bool persistent = false;
		bool isWeighted = false;
		bool isDefault = false;
		float weight = 1.0f;
		
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

		void SetTransformPos(std::string name, float x, float y, float z);
		void SetTransformRot(std::string name, float heading, float attitude, float bank);
		void SetTransformSca(std::string name, float scale);

		void SetMap(std::unordered_map<std::string, NiTransform> map);
		void CopyMap(std::unordered_map<std::string, NiTransform>* map);

		void ForEachTransform(const std::function<void(std::string, NiTransform*)>& functor);

		void Clear();
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

		std::vector<std::shared_ptr<Adjustment>> list;
		std::unordered_map<UInt32, std::shared_ptr<Adjustment>> map;

		NodeSets* nodeSets;
		NodeMap* nodeMap;
		NodeMap* nodeMapBase;

		ActorAdjustments() {}

		ActorAdjustments(UInt32 race, bool isFemale, Actor* actor, TESNPC* npc) :
			race(race),
			isFemale(isFemale),
			actor(actor),
			npc(npc)
		{}

		std::shared_ptr<Adjustment> CreateAdjustment(std::string name);
		UInt32 CreateAdjustment(std::string name, std::string esp, bool persistent, bool hidden);
		std::shared_ptr<Adjustment> GetAdjustment(UInt32 handle);
		std::shared_ptr<Adjustment> GetListAdjustment(UInt32 index);
		void RemoveAdjustment(UInt32 handle);
		void RemoveListAdjustment(UInt32 index);
		void Clear();

		bool UpdateKey(UInt32 race, bool isFemale);
		void UpdateDefault(std::vector<std::string>* adjustments);
		void UpdateSaved(std::vector<std::pair<std::string, std::string>>* adjustments);
		void UpdateUnique(std::vector<std::pair<std::string, std::string>>* uniques);
		void UpdateAdjustments(std::string name);
		void UpdateAllAdjustments();

		std::shared_ptr<Adjustment> LoadAdjustment(std::string filename, bool cached = false);
		UInt32 LoadAdjustment(std::string filename, std::string espName, bool persistent, bool hidden, bool cached = false);

		void SaveAdjustment(std::string filename, UInt32 handle);
		void SaveListAdjustment(std::string filename, UInt32 index);

		void ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor);

		bool RemoveMod(std::string espName);

		void OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform);

		bool IsPersistent();
	};

	class AdjustmentManager
	{
	private:
		std::shared_mutex actorMutex;
		std::shared_mutex fileMutex;
		bool loaded = false;

	public:
		std::unordered_map<UInt64, NodeSets> nodeSets;
		std::unordered_map<UInt64, std::vector<std::string>> defaultAdjustments;
		std::unordered_map<UInt32, std::vector<std::pair<std::string, std::string>>> uniqueAdjustments;

		std::unordered_map<UInt32, std::vector<std::pair<std::string, std::string>>> savedAdjustments;
		std::unordered_map<UInt32, NodeMap> actorNodeMapCache;
		std::unordered_map<std::string, NodeMap> baseNodeMapCache;
		std::unordered_map<UInt32, std::shared_ptr<ActorAdjustments>> actorAdjustmentCache;
		std::unordered_map<std::string, std::unordered_map<std::string, NiTransform>> adjustmentFileCache;

		std::vector<std::shared_ptr<ActorAdjustments>> actorUpdateQueue;

		void Load();
		void RemoveMod(std::string espName);

		void UpdateActor(Actor* actor, TESNPC* npc, bool loaded);
		void UpdateQueue();
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);

		NodeSets* GetNodeSets(UInt32 race, bool isFemale);

		std::unordered_map<std::string, NiTransform>* GetAdjustmentFile(std::string);
		void SetAdjustmentFile(std::string filename, std::unordered_map<std::string, NiTransform> map);

		std::vector<std::string>* GetDefaultAdjustments(UInt32 race, bool isFemale);
		
		NodeMap CreateNodeMap(NiNode* root, NodeSet* set);
		NodeMap* GetActorNodeMap(Actor* actor, TESNPC* npc, NodeSet* nodeSet);
		NodeMap* GetActorBaseMap(Actor* actor, TESNPC* npc, NodeSet* nodeSet);

		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);
	};

	extern AdjustmentManager g_adjustmentManager;
}