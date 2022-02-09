#include "adjustments.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameTypes.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "io.h"
#include "util.h"

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
		NiTransform transform;
		transform.pos.x = 0.0f;
		transform.pos.y = 0.0f;
		transform.pos.z = 0.0f;
		transform.rot.SetEulerAngles(0.0f, 0.0f, 0.0f);
		transform.scale = 1.0f;
		return transform;
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
		NiTransform transform;
		transform.pos.x = 0.0f;
		transform.pos.y = 0.0f;
		transform.pos.z = 0.0f;
		transform.rot.SetEulerAngles(0.0f, 0.0f, 0.0f);
		transform.scale = 1.0f;
		map[name] = transform;
	}

	void Adjustment::SetTransformPos(std::string name, float x, float y, float z)
	{
		NiTransform transform = GetTransformOrDefault(name);
		std::lock_guard<std::shared_mutex> lock(mutex);
		transform.pos.x = x;
		transform.pos.y = y;
		transform.pos.z = z;
		map[name] = transform;
	}

	void Adjustment::SetTransformRot(std::string name, float heading, float attitude, float bank)
	{
		NiTransform transform = GetTransformOrDefault(name);
		std::lock_guard<std::shared_mutex> lock(mutex);
		transform.rot.SetEulerAngles(heading, attitude, bank);
		map[name] = transform;
	}

	void Adjustment::SetTransformSca(std::string name, float scale)
	{
		NiTransform transform = GetTransformOrDefault(name);
		std::lock_guard<std::shared_mutex> lock(mutex);
		transform.scale = scale;
		map[name] = transform;
	}

	void Adjustment::ForEachTransform(const std::function<void(std::string, NiTransform*)>& functor)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		for (auto& transform : map) {
			functor(transform.first, &transform.second);
		}
	}

	void Adjustment::SetMap(std::unordered_map<std::string, NiTransform> newMap)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		map = newMap;
	}

	void Adjustment::CopyMap(std::unordered_map<std::string, NiTransform>* newMap)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		map = std::unordered_map<std::string, NiTransform>(*newMap);
	}

	void Adjustment::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		map.clear();
	}

	bool ActorAdjustments::UpdateKey(UInt32 _race, bool _isFemale)
	{
		if (race == _race && isFemale == _isFemale)
			return false;
		race = _race;
		isFemale = _isFemale;
		return true;
	}

	void ActorAdjustments::UpdateAdjustments(std::string name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		
		NiAVObject* node = (*nodeMap)[name];
		if (!node) return;
		NiAVObject* baseNode = (*nodeMapBase)[name];
		if (!baseNode) return;

		NiTransform transform = baseNode->m_localTransform;

		for (auto& kvp : map) {
			if (kvp.second->HasTransform(name)) {
				NiTransform adjust = kvp.second->GetTransform(name);
				transform.pos += adjust.pos;
				transform.rot = transform.rot * adjust.rot;
				transform.scale *= adjust.scale;
			}
		}

		node->m_localTransform = transform;
	}

	void ActorAdjustments::UpdateAllAdjustments()
	{
		NodeSet set = nodeSets->all;
		for (auto& name : set) {
			UpdateAdjustments(name);
		}
	}

	void ActorAdjustments::UpdateDefault(std::vector<std::string>* adjustments)
	{
		for (auto& filename : *adjustments) {
			std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, true);
			adjustment->isDefault = true;
		}
	}

	void ActorAdjustments::UpdateUnique(std::vector<std::pair<std::string, std::string>>* uniques)
	{
		for (auto& kvp : *uniques) {
			std::shared_ptr<Adjustment> adjustment = LoadAdjustment(kvp.first, true);
			adjustment->esp = kvp.second;
			adjustment->isDefault = true;
		}
	}

	void ActorAdjustments::UpdateSaved(std::vector<std::pair<std::string, std::string>>* adjustments)
	{
		for (auto& kvp : *adjustments) {
			std::shared_ptr<Adjustment> adjustment = LoadAdjustment(kvp.first, true);
			adjustment->esp = kvp.second;
			adjustment->persistent = true;
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

	UInt32 ActorAdjustments::CreateAdjustment(std::string name, std::string espName, bool persistent, bool hidden)
	{
		std::shared_ptr<Adjustment> adjustment = CreateAdjustment(name);
		adjustment->esp = espName;
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

		auto it = list.begin();
		while (it != list.end()) {
			if ((*it)->handle == handle)
				break;
			it++;
		}

		list.erase(it);
		map.erase(handle);
	}

	void ActorAdjustments::RemoveListAdjustment(UInt32 index)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto it = list.begin() + index;
		map.erase((*it)->handle);
		list.erase(it);
	}

	void ActorAdjustments::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		map.clear();
		list.clear();
	}

	std::shared_ptr<Adjustment> ActorAdjustments::LoadAdjustment(std::string filename, bool cached)
	{
		if (cached) {
			auto adjustmentFile = g_adjustmentManager.GetAdjustmentFile(filename);
			if (adjustmentFile) {
				std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);
				adjustment->CopyMap(adjustmentFile);
				return adjustment;
			}
		}

		std::unordered_map<std::string, NiTransform> adjustmentMap;

		if (LoadAdjustmentFile(filename, &nodeSets->all, &adjustmentMap)) {
			std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);
			if (cached) {
				g_adjustmentManager.SetAdjustmentFile(filename, adjustmentMap);
				adjustment->CopyMap(&adjustmentMap);
			} else {
				adjustment->SetMap(adjustmentMap);
			}
			return adjustment;
		}

		return nullptr;
	}

	UInt32 ActorAdjustments::LoadAdjustment(std::string filename, std::string espName, bool persistent, bool hidden, bool cached)
	{
		std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, cached);
		if (!adjustment) return 0;

		adjustment->esp = espName;
		adjustment->persistent = persistent;
		adjustment->hidden = hidden;
		return adjustment->handle;
	}

	void ActorAdjustments::SaveAdjustment(std::string filename, UInt32 handle)
	{
		std::shared_ptr<Adjustment> adjustment = GetAdjustment(handle);
		if (!adjustment) return;
		SaveAdjustmentFile(filename, adjustment);
	}

	void ActorAdjustments::SaveListAdjustment(std::string filename, UInt32 index)
	{
		std::shared_ptr<Adjustment> adjustment = GetListAdjustment(index);
		if (!adjustment) return;
		SaveAdjustmentFile(filename, adjustment);
	}

	void ActorAdjustments::ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : list) {
			functor(adjustment);
		}
	}

	bool ActorAdjustments::RemoveMod(std::string espName)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		bool updated = false;

		auto it = list.begin();
		while (it != list.end()) {
			if ((*it)->esp == espName) {
				updated = true;
				map.erase((*it)->handle);
				it = list.erase(it);
			}
		}

		return updated;
	}

	void ActorAdjustments::OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		std::string baseName = nodeSets->base[name];
		NiAVObject* baseNode = (*nodeMap)[baseName];
		if (!baseNode) return;
		NiTransform baseTransform = baseNode->m_localTransform;
		NiTransform newTransform;
		newTransform.pos = transform.pos - baseTransform.pos;
		newTransform.rot = transform.rot * baseTransform.rot.Transpose();
		if (baseTransform.scale == 0)
			newTransform.scale = 1.0f;
		else
			newTransform.scale = transform.scale / baseTransform.scale;
		adjustment->SetTransform(name, newTransform);
	}

	bool ActorAdjustments::IsPersistent()
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : map) {
			if (adjustment.second->persistent && !adjustment.second->isDefault)
				return true;
		}
		return false;
	}

	std::shared_ptr<ActorAdjustments> AdjustmentManager::GetActorAdjustments(UInt32 formId) {
		if (actorAdjustmentCache.count(formId))
			return actorAdjustmentCache[formId];

		TESForm* form = LookupFormByID(formId);

		Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
		if (!actor)
			return nullptr;

		TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if (!npc)
			return nullptr;

		UInt32 race = npc->race.race->formID;
		bool isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);
		actorAdjustmentCache[actor->formID] = std::make_shared<ActorAdjustments>(race, isFemale, actor, npc);
		actorAdjustmentCache[actor->formID]->CreateAdjustment("NewAdjustment");
		return actorAdjustmentCache[actor->formID];
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

		return nullptr;
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

	void AdjustmentManager::RemoveMod(std::string espName)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);
		for (auto& adjustments : actorAdjustmentCache) {
			if (adjustments.second->RemoveMod(espName))
				adjustments.second->UpdateAllAdjustments();
		}
	}

	NodeMap AdjustmentManager::CreateNodeMap(NiNode* root, NodeSet* set) {
		NodeMap boneMap;
		int counter = set->size();
		root->Visit([&](NiAVObject* object) {
			std::string nodeName(object->m_name.c_str());
			if (set->count(nodeName)) {
				boneMap.emplace(nodeName, object);
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
					boneMap.emplace(bone, findBone);
				}
				else {
					_LogCat("Could not find ", bone);
				}
			}
		}
		return boneMap;
	}

	NodeMap* AdjustmentManager::GetActorNodeMap(Actor* actor, TESNPC* npc, NodeSet* nodeSet) {
		if (actorNodeMapCache.count(npc->formID))
			return &actorNodeMapCache[npc->formID];
		NiNode* root = actor->GetActorRootNode(false);
		if (!root) return nullptr;
		actorNodeMapCache[npc->formID] = CreateNodeMap(root, nodeSet);
		return &actorNodeMapCache[npc->formID];
	}

	NodeMap* AdjustmentManager::GetActorBaseMap(Actor* actor, TESNPC* npc, NodeSet* set)
	{
		std::string filename = (*getRefrModelFilename)(actor);
		if (baseNodeMapCache.count(filename))
			return &baseNodeMapCache[filename];
		NiNode* root = (*getBaseModelRootNode)(npc, actor);
		if (!root) return nullptr;
		baseNodeMapCache[filename] = CreateNodeMap(root, set);
		return &baseNodeMapCache[filename];
	}

	std::unordered_map<std::string, NiTransform>* AdjustmentManager::GetAdjustmentFile(std::string filename)
	{
		std::shared_lock<std::shared_mutex> lock(fileMutex);
		
		if (adjustmentFileCache.count(filename))
			return &adjustmentFileCache[filename];
		return nullptr;
	}

	void AdjustmentManager::SetAdjustmentFile(std::string filename, std::unordered_map<std::string, NiTransform> map)
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
				actorAdjustmentCache.erase(actor->formID);
			}
			return;
		}

		if (!adjustments) {
			adjustments = std::make_shared<ActorAdjustments>();
			adjustments->actor = actor;
			adjustments->npc = npc;
			actorAdjustmentCache[actor->formID] = adjustments;
		}

		UInt32 race = npc->race.race->formID;
		bool isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);

		if (adjustments->UpdateKey(race, isFemale)) {
			adjustments->nodeSets = GetNodeSets(race, isFemale);
			if (adjustments->nodeSets) {
				adjustments->Clear();
				adjustments->CreateAdjustment("NewAdjustment");
				auto defaultAdjustments = GetDefaultAdjustments(race, isFemale);
				if (defaultAdjustments)
					adjustments->UpdateDefault(defaultAdjustments);
				if (savedAdjustments.count(actor->formID))
					adjustments->UpdateSaved(&savedAdjustments[actor->formID]);
				if (uniqueAdjustments.count(npc->formID)) 
					adjustments->UpdateUnique(&uniqueAdjustments[npc->formID]);
				adjustments->nodeMap = GetActorNodeMap(actor, npc, &adjustments->nodeSets->all);
				adjustments->nodeMapBase = GetActorBaseMap(actor, npc, &adjustments->nodeSets->all);
				if (adjustments->nodeMap && adjustments->nodeMapBase)
					adjustments->UpdateAllAdjustments();
				else
					actorUpdateQueue.push_back(adjustments);
			}
		}
	}

	void AdjustmentManager::UpdateQueue() {
		for (auto& adjustments : actorUpdateQueue) {
			adjustments->nodeMap = GetActorNodeMap(adjustments->actor, adjustments->npc, &adjustments->nodeSets->all);
			adjustments->nodeMapBase = GetActorBaseMap(adjustments->actor, adjustments->npc, &adjustments->nodeSets->all);
			if (adjustments->nodeMap && adjustments->nodeMapBase)
				adjustments->UpdateAllAdjustments();
		}

		actorUpdateQueue.clear();
	}

	/*
	* ADJA / ADJD (Adjustment add, adjustment delete)
	* Actors length
	*	FormId
	*   Adjustments length
	*		Name
	*       Mod
	*/

	void AdjustmentManager::SerializeSave(const F4SESerializationInterface* ifc) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		std::vector<std::shared_ptr<ActorAdjustments>> persistentAdjustments;
		for (auto& kvp : actorAdjustmentCache)
		{
			if (kvp.second->IsPersistent())
				persistentAdjustments.push_back(kvp.second);
		}

		ifc->OpenRecord('ADJA', 0); //adjustment add
		UInt32 actorSize = persistentAdjustments.size();
		ifc->WriteRecordData(&actorSize, sizeof(actorSize));

		for (auto& adjustments : persistentAdjustments) {
			UInt32 formId = adjustments->actor->formID;
			ifc->WriteRecordData(&formId, sizeof(formId));

			std::vector<std::shared_ptr<Adjustment>> adjustmentNames;
			for (auto& kvp : adjustments->map) {
				if (kvp.second->persistent && !kvp.second->isDefault) {
					adjustmentNames.push_back(kvp.second);
				}
			}

			UInt32 namesSize = adjustmentNames.size();
			ifc->WriteRecordData(&namesSize, sizeof(namesSize));

			for (auto& adjustment : adjustmentNames) {
				WriteData<std::string>(ifc, &adjustment->name);
				WriteData<std::string>(ifc, &adjustment->esp);
			}
		}
	}

	void AdjustmentManager::SerializeLoad(const F4SESerializationInterface* ifc) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		UInt32 type, length, version;

		if (ifc->GetNextRecordInfo(&type, &version, &length))
		{
			switch (type)
			{
				case 'ADJA': //adjustment add
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
							
							const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(mod.c_str());
							if (modInfo && modInfo->modIndex != 0xFF)
								savedAdjustments[formId].push_back(std::make_pair(name, mod));
						}
					}
					break;
				}
			}
		}
	}

	void AdjustmentManager::SerializeRevert(const F4SESerializationInterface* ifc) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		actorNodeMapCache.clear();
		actorAdjustmentCache.clear();
	}
}