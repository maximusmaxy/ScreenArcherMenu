#include "adjustments.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameTypes.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "io.h"
#include "util.h"
#include "conversions.h"

#include <algorithm>
#include <mutex>
#include <shared_mutex>

namespace SAF {

	AdjustmentManager g_adjustmentManager;

	RelocAddr<GetBaseModelRootNode> getBaseModelRootNode(0x323BB0);
	RelocAddr<GetRefrModelFilename> getRefrModelFilename(0x408360);

	NiTransform Adjustment::GetTransform(std::string name)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		return map[name];
	}

	NiTransform Adjustment::GetTransformOrDefault(std::string name)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		if (map.count(name))
			return map[name];
		return TransformIdentity();
	}

	void Adjustment::SetTransform(std::string name, NiTransform transform)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		map[name] = transform;
	}

	bool Adjustment::HasTransform(std::string name) {
		std::shared_lock<std::shared_mutex> lock(mutex);
		return map.count(name);
	}

	void Adjustment::ResetTransform(std::string name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform def = TransformIdentity();
		map[name] = def;
	}

	void Adjustment::SetTransformPos(std::string name, float x, float y, float z)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform transform = map.count(name) ? map[name] : TransformIdentity();

		transform.pos.x = x;
		transform.pos.y = y;
		transform.pos.z = z;

		map[name] = transform;
	}

	void Adjustment::SetTransformRot(std::string name, float yaw, float pitch, float roll)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform transform = map.count(name) ? map[name] : TransformIdentity();

		NiFromDegree(transform.rot, yaw, pitch, roll);

		map[name] = transform;
	}

	void Adjustment::SetTransformSca(std::string name, float scale)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform transform = map.count(name) ? map[name] : TransformIdentity();

		transform.scale = scale;
		map[name] = transform;
	}

	NiTransform Adjustment::GetScaledTransform(std::string name, float scalar)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		return SlerpNiTransform(map[name], scalar);
	}

	void Adjustment::ForEachTransform(const std::function<void(std::string, NiTransform*)>& functor)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		for (auto& transform : map) {
			functor(transform.first, &transform.second);
		}
	}

	void Adjustment::ForEachTransformOrDefault(const std::function<void(std::string, NiTransform*)>& functor, NodeSet* set)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		for (auto& nodeName : *set) {
			NiTransform transform = map.count(name) ? map[name] : TransformIdentity();
			functor(nodeName, &transform);
		}
	}

	TransformMap Adjustment::GetMap()
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		return map;
	}

	void Adjustment::SetMap(TransformMap newMap)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		map = newMap;
	}

	void Adjustment::CopyMap(TransformMap* newMap, NodeSet* set)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		
		map.clear();

		for (auto& kvp : *newMap) {
			if (set->count(kvp.first)) {
				map[kvp.first] = kvp.second;
			}
		}
	}

	void Adjustment::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		map.clear();
	}

	bool ActorAdjustments::ShouldUpdate()
	{
		UInt32 currentRace = npc->race.race->formID;
		bool currentIsFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);
		NiNode* currentRoot = actor->GetActorRootNode(false);
		if (currentRace == race && currentIsFemale == isFemale && currentRoot == root)
			return false;
		race = currentRace;
		isFemale = currentIsFemale;
		root = currentRoot;
		return true;
	}

	void ActorAdjustments::UpdateAdjustments(std::string name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (!nodeMap.count(name)) return;
		NiAVObject* node = nodeMap[name];
		if (!nodeMapBase->count(name)) return;
		NiAVObject* baseNode = (*nodeMapBase)[name];

		NiTransform transform = baseNode->m_localTransform;

		for (auto& adjustment : list) {
			if (adjustment->HasTransform(name)) {
				NiTransform adjust = adjustment->scale == 1.0f ? adjustment->GetTransform(name) :
					adjustment->GetScaledTransform(name, adjustment->scale);
				transform = transform * adjust;
			}
		}

		node->m_localTransform = transform;
	}

	void ActorAdjustments::UpdateAllAdjustments()
	{
		if (!nodeMapBase) return;

		NodeSet set = nodeSets->all;
		for (auto& name : set) {
			UpdateAdjustments(name);
		}
	}

	void ActorAdjustments::UpdateAllAdjustments(std::shared_ptr<Adjustment> adjustment)
	{
		auto map = adjustment->GetMap();
		for (auto& kvp : map) {
			UpdateAdjustments(kvp.first);
		}
	}

	void ActorAdjustments::UpdatePersistentAdjustments(std::vector<std::string>* defaults, std::vector<std::pair<std::string, std::string>>* uniques, std::vector<PersistentAdjustment>* persistents) {
		std::unordered_set<std::string> removeSet;
		
		if (persistents) {
			for (auto& persistent : *persistents) {
				switch (persistent.type) {
				case kAdjustmentSerializeAdd:
				{
					std::shared_ptr<Adjustment> adjustment = CreateAdjustment(persistent.name);
					adjustment->mod = persistent.mod;
					adjustment->map = persistent.map;
					adjustment->persistent = true;
					break;
				}
				case kAdjustmentSerializeLoad:
				{
					removeSet.insert(persistent.name);
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(persistent.name, true);
					adjustment->mod = persistent.mod;
					adjustment->persistent = true;
					adjustment->saved = true;
					break;
				}
				case kAdjustmentSerializeRemove:
				{
					removedAdjustments.insert(persistent.name);
					removeSet.insert(persistent.name);
					break;
				}
				}
			}
		}
		
		if (defaults) {
			for (auto& filename : *defaults) {
				if (!removeSet.count(filename)) {
					removeSet.insert(filename);
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, true);
					adjustment->isDefault = true;
				}
			}
		}
		
		if (uniques) {
			for (auto& kvp : *uniques) {
				if (!removeSet.count(kvp.first)) {
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(kvp.first, true);
					adjustment->isDefault = true;
				}
			}
		}
	}

	std::shared_ptr<Adjustment> ActorAdjustments::CreateAdjustment(std::string name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		std::shared_ptr<Adjustment> adjustment = std::make_shared<Adjustment>(nextHandle, name);
		map[nextHandle] = adjustment;
		list.push_back(adjustment);
		nextHandle++;
		return adjustment;
	}

	UInt32 ActorAdjustments::CreateAdjustment(std::string name, std::string modName, bool persistent, bool hidden)
	{
		std::shared_ptr<Adjustment> adjustment = CreateAdjustment(name);
		adjustment->mod = modName;
		adjustment->persistent = persistent;
		adjustment->hidden = hidden;
		return adjustment->handle;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::GetAdjustment(UInt32 handle)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		if (map.count(handle))
			return map[handle];
		return nullptr;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::GetListAdjustment(UInt32 index)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		if (index < list.size())
			return list[index];
		return nullptr;
	}

	void ActorAdjustments::RemoveAdjustment(UInt32 handle)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (!handle) return;

		auto it = list.begin();
		while (it != list.end()) {
			if ((*it)->handle == handle)
				break;
			++it;
		}

		if (it == list.end()) {
			_DMESSAGE("Attempted to remove invalid handle");
			return;
		}

		if ((*it)->isDefault) {
			removedAdjustments.insert((*it)->name);
		}

		if (handle == poseHandle)
			poseHandle = 0;

		list.erase(it);
		map.erase(handle);
	}

	std::shared_ptr<Adjustment> ActorAdjustments::LoadAdjustment(std::string filename, bool cached)
	{
		if (cached) {
			auto adjustmentFile = g_adjustmentManager.GetAdjustmentFile(filename);
			if (adjustmentFile) {
				if (removedAdjustments.count(filename))
					removedAdjustments.erase(filename);
				std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);

				if (nodeSets) {
					adjustment->CopyMap(adjustmentFile, &nodeSets->all);
				}
				
				return adjustment;
			}
		}

		std::unordered_map<std::string, NiTransform> adjustmentMap;

		if (LoadAdjustmentFile(filename, &adjustmentMap)) {
			if (removedAdjustments.count(filename))
				removedAdjustments.erase(filename);
			if (cached) {
				g_adjustmentManager.SetAdjustmentFile(filename, adjustmentMap);
			}
			std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);

			if (nodeSets) {
				adjustment->CopyMap(&adjustmentMap, &nodeSets->all);
			}
			
			return adjustment;
		}

		return nullptr;
	}

	UInt32 ActorAdjustments::LoadAdjustment(std::string filename, std::string modName, bool persistent, bool hidden, bool cached)
	{
		std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, cached);
		if (!adjustment) return 0;

		adjustment->mod = modName;
		adjustment->persistent = persistent;
		adjustment->hidden = hidden;
		adjustment->saved = true;
		return adjustment->handle;
	}

	void ActorAdjustments::SaveAdjustment(std::string filename, UInt32 handle)
	{
		std::shared_ptr<Adjustment> adjustment = GetAdjustment(handle);
		if (!adjustment) return;

		if (SaveAdjustmentFile(filename, adjustment)) {
			adjustment->saved = true;
		}
	}

	void ActorAdjustments::ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : list) {
			functor(adjustment);
		}
	}

	bool ActorAdjustments::RemoveMod(BSFixedString modName)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		bool updated = false;

		auto it = list.begin();
		while (it != list.end()) {
			if (BSFixedString((*it)->mod.c_str()) == modName) {
				updated = true;
				map.erase((*it)->handle);
				it = list.erase(it);
			}
			else {
				++it;
			}
		}

		return updated;
	}

	//Sets the override node such that it negates the base node back to the a-pose position
	void ActorAdjustments::NegateTransform(std::shared_ptr<Adjustment> adjustment, std::string name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (!nodeSets->baseMap.count(name)) return;
		std::string baseName = nodeSets->baseMap[name];

		if (!nodeMap.count(baseName)) return;
		NiAVObject* baseNode = nodeMap[baseName];

		if (!nodeMapBase->count(baseName)) return;
		NiAVObject* skeletonNode = (*nodeMapBase)[baseName];

		adjustment->SetTransform(name, NegateNiTransform(baseNode->m_localTransform, skeletonNode->m_localTransform));
	}

	//Sets the override node such that it negates the base node to the specified transform position
	void ActorAdjustments::OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (!nodeSets->baseMap.count(name)) return;
		std::string baseName = nodeSets->baseMap[name];

		if (!nodeMap.count(baseName)) return;
		NiAVObject* baseNode = nodeMap[baseName];

		adjustment->SetTransform(name, NegateNiTransform(baseNode->m_localTransform, transform));
	}

	void ActorAdjustments::NegateTransformGroup(std::shared_ptr<Adjustment> adjustment, std::string groupName)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (!nodeSets->groups.count(groupName)) return;

		for (auto& baseName : nodeSets->groups[groupName]) {
			std::string overrider = baseName + g_adjustmentManager.overridePostfix;

			if (nodeMap.count(baseName)) {

				NiAVObject* baseNode = nodeMap[baseName];
				if (nodeMapBase->count(baseName)) {

					NiAVObject* skeletonNode = (*nodeMapBase)[baseName];
					adjustment->SetTransform(overrider, NegateNiTransform(baseNode->m_localTransform, skeletonNode->m_localTransform));
				}
			}
		}
	}

	void ActorAdjustments::SavePose(std::string filename, std::vector<UInt32> handles) {
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (handles.size() == 0) return;

		//Order matters for applying translations so get them from the list in the correct order
		std::vector<std::shared_ptr<Adjustment>> adjustments;
		for (auto& adjustment : list) {
			if (std::find_if(handles.begin(), handles.end(), [adjustment](UInt32 h) { return adjustment->handle == h; }) != handles.end())
				adjustments.push_back(adjustment);
		}

		if (adjustments.size() == 0) return;

		TransformMap poseMap;
		for (auto& overrider : nodeSets->overrides) {
			if (nodeSets->baseMap.count(overrider)) {

				std::string baseName = nodeSets->baseMap[overrider];
				if (nodeMap.count(baseName)) {
					
					NiTransform transform = nodeMap[baseName]->m_localTransform;

					for (auto& adjustment : adjustments) {
						if (adjustment->HasTransform(overrider)) {
							transform = transform * adjustment->GetTransform(overrider);
						}
					}

					poseMap[baseName] = transform;
				}
			}
		}

		SavePoseFile(filename, &poseMap);
	}

	void ActorAdjustments::LoadPose(std::string filename) {
		std::lock_guard<std::shared_mutex> lock(mutex);

		TransformMap poseMap;
		
		if (LoadPoseFile(filename, &poseMap)) {

			std::shared_ptr<Adjustment> adjustment;

			for (auto& kvp : map) {
				if (!kvp.second->saved && kvp.second->handle != poseHandle) {
					kvp.second->Clear();
				}
			}

			if (poseHandle && map.count(poseHandle)) {
				adjustment = map[poseHandle];
				adjustment->Clear();
			}
			else {
				adjustment = std::make_shared<Adjustment>(nextHandle, filename);
				map[nextHandle] = adjustment;
				list.push_back(adjustment);
				nextHandle++;

				poseHandle = adjustment->handle;
			}

			for (auto& kvp : poseMap) {
				std::string base = kvp.first;
				std::string overrider = base + g_adjustmentManager.overridePostfix;
				if (nodeMap.count(base)) {
					adjustment->SetTransform(overrider, NegateNiTransform(nodeMap[base]->m_localTransform, kvp.second));
				}
			}
		}
	}

	void ActorAdjustments::ResetPose() {
		if (!poseHandle) return;

		RemoveAdjustment(poseHandle);
	}

	std::shared_ptr<ActorAdjustments> AdjustmentManager::GetActorAdjustments(UInt32 formId) {
		std::shared_lock<std::shared_mutex> lock(actorMutex);

		if (actorAdjustmentCache.count(formId))
			return actorAdjustmentCache[formId];
		return nullptr;
	}

	std::shared_ptr<ActorAdjustments> AdjustmentManager::GetActorAdjustments(TESObjectREFR* refr) {
		if (!refr) return nullptr;

		std::shared_ptr<ActorAdjustments> adjustments = GetActorAdjustments(refr->formID);

		if (!adjustments) {
			std::lock_guard<std::shared_mutex> lock(actorMutex);

			TESForm* form = LookupFormByID(refr->formID);

			Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
			if (!actor) return nullptr;


			TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
			if (!npc) return nullptr;

			adjustments = std::make_shared<ActorAdjustments>(actor, npc);

			actorAdjustmentCache[actor->formID] = adjustments;

			UpdateActorAdjustments(adjustments);
		}
		return adjustments;
	}

	NodeSets* AdjustmentManager::GetNodeSets(UInt32 race, bool isFemale) {
		UInt64 key = race;
		if (isFemale)
			race |= 0x100000000;

		if (nodeSets.count(key))
			return &nodeSets[key];
		if (isFemale && nodeSets.count(race))
			return &nodeSets[race];

		//if not found default to human
		race = 0x00013746;
		key = race;
		if (isFemale)
			key |= 0x100000000;

		if (nodeSets.count(key))
			return &nodeSets[key];
		if (isFemale && nodeSets.count(race))
			return &nodeSets[race];

		//if the human node set is not found something went terribly wrong
		_DMESSAGE("Node map could not be found");

		return nullptr;
	}

	std::shared_ptr<ActorAdjustments> AdjustmentManager::CreateActorAdjustment(UInt32 formId)
	{
		TESForm* form = LookupFormByID(formId);

		Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
		if (!actor) return nullptr;

		TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if (!npc) return nullptr;

		std::shared_ptr<ActorAdjustments> adjustments = std::make_shared<ActorAdjustments>(actor, npc);

		actorAdjustmentCache[actor->formID] = adjustments;

		UpdateActorAdjustments(adjustments);

		return adjustments;
	}

	std::vector<std::string>* AdjustmentManager::GetDefaultAdjustments(UInt32 race, bool isFemale)
	{
		UInt64 key = race;
		if (isFemale)
			key |= 0x100000000;

		if (defaultAdjustments.count(key))
			return &defaultAdjustments[key];
		if (isFemale && defaultAdjustments.count(race))
			return &defaultAdjustments[race];

		return nullptr;
	}

	void AdjustmentManager::Load()
	{
		if (!loaded) {
			loaded = true;
			LoadFiles();
		}
	}

	void AdjustmentManager::RemoveMod(BSFixedString modName)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		for (auto& adjustments : actorAdjustmentCache) {
			bool removed = adjustments.second->RemoveMod(modName);
			if (removed) adjustments.second->UpdateAllAdjustments();
		}
	}

	NodeMap AdjustmentManager::CreateNodeMap(NiNode* root, NodeSet* set) {
		NodeMap boneMap;

		if (!root || !set) return boneMap;

		int counter = set->size();
		root->Visit([&](NiAVObject* object) {
			std::string nodeName;
			if (ContainsBSFixed(set, &object->m_name, &nodeName)) {
				boneMap[nodeName] = object;
				--counter;
			}
			return !counter;
		});

		//shouldn't need to do this to find missing bones, need to fix the method above.
		for (auto& bone : *set) {
			if (!boneMap.count(bone)) {
				BSFixedString name(bone.c_str());
				NiPointer<NiAVObject> findBone = root->GetObjectByName(&name);
				if (findBone) {
					boneMap[bone] = findBone;
				}
				else {
					_LogCat("Could not find ", bone);
				}
			}
		}
		return boneMap;
	}

	NodeMap* AdjustmentManager::GetActorBaseMap(std::shared_ptr<ActorAdjustments> adjustments, NodeSet* set)
	{
		NiNode* currentRoot = (*getBaseModelRootNode)(adjustments->npc, adjustments->actor);

		if (currentRoot == adjustments->root) {
			if (baseNodeMapCache.count(currentRoot))
				return &baseNodeMapCache[currentRoot];
		}

		adjustments->root = currentRoot;

		baseNodeMapCache[currentRoot] = CreateNodeMap(currentRoot, set);
		return &baseNodeMapCache[currentRoot];
	}

	TransformMap* AdjustmentManager::GetAdjustmentFile(std::string filename)
	{
		std::shared_lock<std::shared_mutex> lock(fileMutex);

		if (adjustmentFileCache.count(filename))
			return &adjustmentFileCache[filename];
		return nullptr;
	}

	void AdjustmentManager::SetAdjustmentFile(std::string filename, TransformMap map)
	{
		std::lock_guard<std::shared_mutex> lock(fileMutex);

		adjustmentFileCache[filename] = map;
	}

	void AdjustmentManager::UpdateActor(Actor* actor, TESNPC* npc, bool loaded) {

		std::lock_guard<std::shared_mutex> lock(actorMutex);

		std::shared_ptr<ActorAdjustments> adjustments = nullptr;

		if (actorAdjustmentCache.count(actor->formID))
			adjustments = actorAdjustmentCache[actor->formID];

		if (!loaded) {
			if (adjustments) {
				StorePersistentAdjustments(adjustments);
				actorAdjustmentCache.erase(actor->formID);
			}
			return;
		}

		if (!adjustments) {
			adjustments = std::make_shared<ActorAdjustments>(actor, npc);
			actorAdjustmentCache[actor->formID] = adjustments;
		}

		UpdateActorAdjustments(adjustments);
	}

	void AdjustmentManager::CreateNewAdjustment(UInt32 formId, const char* name, const char* mod, bool persistent, bool hidden) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		adjustments->CreateAdjustment(name, mod, persistent, hidden);
	}

	void AdjustmentManager::LoadAdjustment(UInt32 formId, const char* filename, const char* mod, bool persistent, bool hidden) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		adjustments->LoadAdjustment(filename, mod, persistent, hidden);
	}

	void AdjustmentManager::RemoveAdjustment(UInt32 formId, UInt32 handle) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return;

		adjustments->RemoveAdjustment(handle);
	}

	void AdjustmentManager::ResetAdjustment(UInt32 formId, UInt32 handle) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return;

		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		adjustment->Clear();
	}

	void AdjustmentManager::SetTransform(AdjustmentTransformMessage* message) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(message->formId)) return;
		
		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[message->formId];
		if (!adjustments) return;

		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(message->handle);
		if (!adjustment) return;

		switch (message->type) {
		case kAdjustmentTransformPosition:
			adjustment->SetTransformPos(message->key, message->a, message->b, message->c);
			break;
		case kAdjustmentTransformRotation:
			adjustment->SetTransformRot(message->key, message->a, message->b, message->c);
			break;
		case kAdjustmentTransformScale:
			adjustment->SetTransformSca(message->key, message->a);
			break;
		case kAdjustmentTransformReset:
			adjustment->ResetTransform(message->key);
			break;
		case kAdjustmentTransformNegate:
			adjustments->NegateTransform(adjustment, message->key);
			break;
		}
	}

	void AdjustmentManager::NegateAdjustments(UInt32 formId, UInt32 handle, const char* groupName) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return;

		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		adjustments->NegateTransformGroup(adjustment, groupName);
	}

	void AdjustmentManager::LoadPose(UInt32 formId, const char* filename) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return;

		adjustments->LoadPose(filename);
	}

	void AdjustmentManager::ResetPose(UInt32 formId) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return;

		adjustments->ResetPose();
	}

	void AdjustmentManager::UpdateActorAdjustments(std::shared_ptr<ActorAdjustments> adjustments) {
		if (!adjustments || !adjustments->npc || !adjustments->actor) return;

		if (adjustments->ShouldUpdate()) {
			if (adjustments->root) {
				UpdateAdjustments(adjustments);
			}
			else {
				//couldn't find root node so queue for later
				actorUpdates.insert(adjustments->actor->formID);
			}
		}
	}

	void AdjustmentManager::UpdateQueue() {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		for (auto& id : actorUpdates) {
			if (actorAdjustmentCache.count(id)) {
				auto adjustments = actorAdjustmentCache[id];

				//force update
				adjustments->race = adjustments->npc->race.race->formID;
				adjustments->isFemale = (CALL_MEMBER_FN(adjustments->npc, GetSex)() == 1 ? true : false);
				adjustments->root = adjustments->actor->GetActorRootNode(false);

				UpdateAdjustments(adjustments);
			}
		}

		actorUpdates.clear();
	}

	void AdjustmentManager::UpdateAdjustments(std::shared_ptr<ActorAdjustments> adjustments) {
		adjustments->map.clear();
		adjustments->list.clear();
		adjustments->removedAdjustments.clear();
		adjustments->poseHandle = 0;

		if (!adjustments->root) return;

		adjustments->nodeSets = GetNodeSets(adjustments->race, adjustments->isFemale);
		if (!adjustments->nodeSets) return;

		adjustments->nodeMap = CreateNodeMap(adjustments->root, &adjustments->nodeSets->allOrBase);
		adjustments->nodeMapBase = GetActorBaseMap(adjustments, &adjustments->nodeSets->allOrBase);

		adjustments->CreateAdjustment("NewAdjustment");

		std::vector<std::string>* defaults = GetDefaultAdjustments(adjustments->race, adjustments->isFemale);
		std::vector<std::pair<std::string, std::string>>* uniques = nullptr;
		std::vector<PersistentAdjustment>* persistents = nullptr;

		if (uniqueAdjustments.count(adjustments->npc->formID))
			uniques = &uniqueAdjustments[adjustments->npc->formID];

		if (persistentAdjustments.count(adjustments->actor->formID))
			persistents = &persistentAdjustments[adjustments->actor->formID];

		adjustments->UpdatePersistentAdjustments(defaults, uniques, persistents);

		adjustments->UpdateAllAdjustments();
	}

	/*
	* ADJU
	* Actors length
	*   FormId
	*	Adjustments length
	*		Name
	*		Mod
	*		Type
	*		Transforms length (Only if add type)
	*			key
	*			x
	*			y
	*			z
	*			yaw
	*			pitch
	*			roll
	*			scale
	*/

	void AdjustmentManager::SerializeSave(const F4SESerializationInterface* ifc) {

		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock);

		std::unordered_map<UInt32, std::vector<PersistentAdjustment>> savedAdjustments;

		//collect currently loaded actors
		for (auto& actorAdjustments : actorAdjustmentCache) {
			actorAdjustments.second->GetPersistentAdjustments(&savedAdjustments);
		}

		//the persistence store is only updated when an actor is unloaded so only add the missing ones
		for (auto& persistent : persistentAdjustments) {
			if (!savedAdjustments.count(persistent.first)) {
				savedAdjustments[persistent.first] = persistent.second;
			}
		}

		ifc->OpenRecord('ADJU', 0); //adjustments

		UInt32 size = savedAdjustments.size();
		WriteData<UInt32>(ifc, &size);
		for (auto& adjustmentKvp : savedAdjustments) {

			WriteData<UInt32>(ifc, &adjustmentKvp.first);
			UInt32 adjustmentsSize = adjustmentKvp.second.size();

			WriteData<UInt32>(ifc, &adjustmentsSize);
			for (auto& adjustment : adjustmentKvp.second) {

				WriteData<std::string>(ifc, &adjustment.name);
				WriteData<std::string>(ifc, &adjustment.mod);
				WriteData<UInt8>(ifc, &adjustment.type);

				if (adjustment.type == kAdjustmentSerializeAdd) {

					UInt32 mapSize = adjustment.map.size();
					WriteData<UInt32>(ifc, &mapSize);

					for (auto& kvp : adjustment.map) {
						WriteData<std::string>(ifc, &kvp.first);
						WriteData<float>(ifc, &kvp.second.pos.x);
						WriteData<float>(ifc, &kvp.second.pos.y);
						WriteData<float>(ifc, &kvp.second.pos.z);
						float yaw, pitch, roll;
						NiToEuler(kvp.second.rot, yaw, pitch, roll);
						WriteData<float>(ifc, &yaw);
						WriteData<float>(ifc, &pitch);
						WriteData<float>(ifc, &roll);
						WriteData<float>(ifc, &kvp.second.scale);
					}
				}
			}
		}
	}

	void AdjustmentManager::SerializeLoad(const F4SESerializationInterface* ifc) {
		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock);

		UInt32 type, length, version;

		if (ifc->GetNextRecordInfo(&type, &version, &length))
		{
			switch (type)
			{
			case 'ADJU': //adjustment
			{
				UInt32 actorSize;
				ReadData<UInt32>(ifc, &actorSize);

				for (UInt32 i = 0; i < actorSize; ++i) {

					UInt32 oldId, formId;
					ReadData<UInt32>(ifc, &oldId);
					ifc->ResolveFormId(oldId, &formId);

					UInt32 adjustmentsSize;
					ReadData<UInt32>(ifc, &adjustmentsSize);

					for (UInt32 j = 0; j < adjustmentsSize; ++j) {
						std::string name;
						ReadData<std::string>(ifc, &name);
						std::string mod;
						ReadData<std::string>(ifc, &mod);
						UInt8 adjustmentType;
						ReadData<UInt8>(ifc, &adjustmentType);

						//Accept loaded mods and adjustments that don't have a mod specified
						bool modLoaded = mod.empty();
						if (!modLoaded) {
							const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(mod.c_str());
							modLoaded = modInfo && modInfo->modIndex != 0xFF;
						}
						
						if (adjustmentType == kAdjustmentSerializeAdd) {

							UInt32 transformSize;
							ReadData<UInt32>(ifc, &transformSize);
							if (transformSize > 0) {

								TransformMap map;
								for (UInt32 k = 0; k < transformSize; ++k) {

									std::string key;
									ReadData<std::string>(ifc, &key);

									NiTransform transform;
									ReadData<float>(ifc, &transform.pos.x);
									ReadData<float>(ifc, &transform.pos.y);
									ReadData<float>(ifc, &transform.pos.z);

									float yaw, pitch, roll;
									ReadData<float>(ifc, &yaw);
									ReadData<float>(ifc, &pitch);
									ReadData<float>(ifc, &roll);
									NiFromEuler(transform.rot, yaw, pitch, roll);

									ReadData<float>(ifc, &transform.scale);

									map[key] = transform;
								}
								if (modLoaded) {
									persistentAdjustments[formId].push_back(PersistentAdjustment(name, mod, adjustmentType, map));
									if (actorAdjustmentCache.count(formId)) {
										actorUpdates.insert(formId);
									}
								}
							}
						}
						else
						{
							if (modLoaded) {
								persistentAdjustments[formId].push_back(PersistentAdjustment(name, mod, adjustmentType));
								if (actorAdjustmentCache.count(formId)) {
									actorUpdates.insert(formId);
								}
							}
						}
					}
				}
				break;
			}
			}
		}
	}

	void AdjustmentManager::SerializeRevert(const F4SESerializationInterface* ifc) {
		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock);

		persistentAdjustments.clear();
		actorUpdates.clear();
	}

	void ActorAdjustments::GetPersistentAdjustments(std::unordered_map<UInt32, std::vector<PersistentAdjustment>>* persistentAdjustments) {
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : map) {
			PersistentAdjustment persistentAdjustment = adjustment.second->GetPersistence(&removedAdjustments);
			if (persistentAdjustment.type != kAdjustmentSerializeDisabled)
				(*persistentAdjustments)[actor->formID].push_back(persistentAdjustment);
		}
	}

	void AdjustmentManager::StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments) {
		std::lock_guard<std::shared_mutex> persistenceLock(persistenceMutex);

		if (persistentAdjustments.count(adjustments->actor->formID))
			persistentAdjustments[adjustments->actor->formID].clear();
		adjustments->GetPersistentAdjustments(&persistentAdjustments);
	}

	PersistentAdjustment Adjustment::GetPersistence(std::unordered_set<std::string>* removedAdjustments) {
		std::shared_lock<std::shared_mutex> lock(mutex);

		UInt8 type = kAdjustmentSerializeDisabled;
		if (isDefault) {
			if (removedAdjustments->count(name)) {
				type = kAdjustmentSerializeRemove;
			}
		}
		else if (persistent) {
			if (saved) {
				type = kAdjustmentSerializeLoad;
			}
			else {
				type = kAdjustmentSerializeAdd;
			}
		}

		switch (type) {
		case kAdjustmentSerializeAdd:
			return PersistentAdjustment(name, mod, type, map);
		case kAdjustmentSerializeLoad:
		case kAdjustmentSerializeRemove:
			return PersistentAdjustment(name, mod, type);
		default:
			return PersistentAdjustment();
		}
	}
}