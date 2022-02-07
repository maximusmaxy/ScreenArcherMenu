#pragma once

#include "f4se/NiObjects.h"
#include "f4se/NiNodes.h"
#include "f4se/GameReferences.h"
#include "f4se/GameObjects.h"
#include "f4se/GameTypes.h"
#include "f4se/PluginAPI.h"
#include "f4se/PapyrusArgs.h"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <mutex>

namespace SAF {

	typedef std::unordered_set<std::string> NodeSet;
	typedef std::unordered_map<std::string, NiAVObject*> NodeMap;

	typedef NiNode* (*GetBaseModelRootNode)(TESNPC* npc, TESObjectREFR* refr);
	extern RelocAddr<GetBaseModelRootNode> getBaseModelRootNode;

	typedef const char* (*GetRefrModelFilename)(TESObjectREFR* refr);
	extern RelocAddr<GetRefrModelFilename> getRefrModelFilename;

	enum {
		kSafMessageManager = 1
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
		SimpleLock lock;

	public:
		std::string name;
		UInt32 handle = -1;
		std::string esp;
		bool hidden = false;
		bool persistent = false;
		bool isWeighted = false;
		bool isDefault = false;
		float weight = 1.0f;
		std::unordered_map<std::string, NiTransform> map;

		Adjustment() {}

		Adjustment(UInt32 handle, std::string name) :
			handle(handle),
			name(name)
		{}

		NiTransform GetTransform(std::string name);
		NiTransform GetTransformOrDefault(std::string name);
		void SetTransform(std::string name, NiTransform transform);
		void ResetTransform(std::string name);

		void SetTransformPos(std::string name, float x, float y, float z);
		void SetTransformRot(std::string name, float heading, float attitude, float bank);
		void SetTransformSca(std::string name, float scale);

		void ForEachTransform(const std::function<void(std::string, NiTransform*)>& functor);

		void Clear();
	};

	class ActorAdjustments
	{
	private:
		UInt32 nextHandle = 1; //0 handle is null
		SimpleLock lock;

	public:
		UInt32 race;
		bool isFemale;

		Actor* actor;
		TESNPC* npc;

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

		bool UpdateKey(UInt32 race, bool isFemale);
		void UpdateDefault(std::vector<std::string>* adjustments);
		void UpdateSaved(std::vector<std::pair<std::string, std::string>>* adjustments);
		void UpdateAdjustments(std::string name);
		void UpdateAllAdjustments();

		std::shared_ptr<Adjustment> LoadAdjustment(std::string filename);
		UInt32 LoadAdjustment(std::string filename, std::string espName, bool persistent, bool hidden);
		void SaveAdjustment(std::string filename, UInt32 handle);
		void SaveListAdjustment(std::string filename, UInt32 index);

		void ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor);

		void GetModAdjustmentsPapyrus(std::string espName, VMArray<UInt32>* result);
		bool RemoveMod(std::string espName);

		void OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform);

		bool IsPersistent();
	};

	class AdjustmentManager
	{
	private:
		SimpleLock lock;
		bool loaded = false;

	public:
		std::unordered_map<UInt64, NodeSets> nodeSets;
		std::unordered_map<UInt64, std::vector<std::string>> defaultAdjustments;
		std::unordered_map<std::string, std::unordered_map<std::string, NiTransform>> defaultAdjustmentCache;
		std::unordered_map<UInt32, NodeMap> actorNodeMapCache;
		std::unordered_map<std::string, NodeMap> baseNodeMapCache;
		std::unordered_map<UInt32, std::shared_ptr<ActorAdjustments>> actorAdjustmentCache;
		std::unordered_map<UInt32, std::vector<std::pair<std::string, std::string>>> savedAdjustmentCache;

		void Load();
		void RemoveMod(std::string espName);

		void UpdateActor(Actor* actor, TESNPC* npc, bool loaded);
		std::shared_ptr<ActorAdjustments> GetActorAdjustments(UInt32 formId);

		NodeSets* GetNodeSets(UInt64 key);
		void SetNodeSets(UInt64 key, NodeSets nodeSets);

		std::unordered_map<std::string, NiTransform>* GetDefaultAdjustment(std::string);
		std::vector<std::string>* GetDefaultAdjustments(UInt64 key);
		void SetDefaultAdjustments(UInt64 key, std::vector<std::string> defaultAdjustments);
		std::vector<std::pair<std::string, std::string>>* GetSavedAdjustments(UInt32 formId);
		
		NodeMap CreateNodeMap(NiNode* root, NodeSet* set);
		NodeMap* GetActorNodeMap(TESObjectREFR* actor, NodeSet* nodeSet);
		NodeMap* GetActorBaseMap(TESObjectREFR* actor, NodeSet* nodeSet);

		void SerializeSave(const F4SESerializationInterface* ifc);
		void SerializeLoad(const F4SESerializationInterface* ifc);
		void SerializeRevert(const F4SESerializationInterface* ifc);
	};

	extern AdjustmentManager g_adjustmentManager;
}