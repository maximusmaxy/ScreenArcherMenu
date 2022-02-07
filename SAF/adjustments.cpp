#include "Adjustments.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameTypes.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "io.h"
#include "util.h"

#include <algorithm>

namespace SAF {

	AdjustmentManager g_adjustmentManager;

	RelocAddr<GetBaseModelRootNode> getBaseModelRootNode(0x323BB0);
	RelocAddr<GetRefrModelFilename> getRefrModelFilename(0x408360);

	NiTransform Adjustment::GetTransform(std::string name)
	{
		SimpleLocker locker(&lock);
		return map[name];
	}

	NiTransform Adjustment::GetTransformOrDefault(std::string name)
	{
		SimpleLocker locker(&lock);
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
		lock.Lock();
		map[name] = transform;
		lock.Release();
	}

	void Adjustment::ResetTransform(std::string name)
	{
		lock.Lock();
		NiTransform transform;
		transform.pos.x = 0.0f;
		transform.pos.y = 0.0f;
		transform.pos.z = 0.0f;
		transform.rot.SetEulerAngles(0.0f, 0.0f, 0.0f);
		transform.scale = 1.0f;
		map[name] = transform;
		lock.Release();
	}

	void Adjustment::SetTransformPos(std::string name, float x, float y, float z)
	{
		NiTransform transform = GetTransformOrDefault(name);
		transform.pos.x = x;
		transform.pos.y = y;
		transform.pos.z = z;
		map[name] = transform;
	}

	void Adjustment::SetTransformRot(std::string name, float heading, float attitude, float bank)
	{
		NiTransform transform = GetTransformOrDefault(name);
		transform.rot.SetEulerAngles(heading, attitude, bank);
		map[name] = transform;
	}

	void Adjustment::SetTransformSca(std::string name, float scale)
	{
		NiTransform transform = GetTransformOrDefault(name);
		transform.scale = scale;
		map[name] = transform;
	}

	void Adjustment::ForEachTransform(const std::function<void(std::string, NiTransform*)>& functor)
	{
		lock.Lock();
		for (auto& transform : map) {
			functor(transform.first, &transform.second);
		}
		lock.Release();
	}

	void Adjustment::Clear()
	{
		lock.Lock();
		map.clear();
		lock.Release();
	}

	bool ActorAdjustments::UpdateKey(UInt32 _race, bool _isFemale)
	{
		if (race == _race && isFemale == _isFemale)
			return false;
		race = _race;
		isFemale = _isFemale;
		return true;
	}

	void ActorAdjustments::UpdateDefault(std::vector<std::string>* adjustments)
	{
		for (auto& filename : *adjustments) {
			std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);
			auto defaultAdjustment = g_adjustmentManager.GetDefaultAdjustment(filename);
			if (defaultAdjustment) {
				adjustment->isDefault = true;
				adjustment->map = std::unordered_map<std::string, NiTransform>(*defaultAdjustment);
			}
			else {
				LoadAdjustmentFile(filename, &nodeSets->all, &g_adjustmentManager.defaultAdjustmentCache[filename]);
				adjustment->map = g_adjustmentManager.defaultAdjustmentCache[filename];
			}
		}
	}

	void ActorAdjustments::UpdateAdjustments(std::string name)
	{
		SimpleLocker locker(&lock);
		
		NiAVObject* node = (*nodeMap)[name];
		if (!node) return;
		NiAVObject* baseNode = (*nodeMapBase)[name];
		if (!baseNode) return;

		NiTransform transform = baseNode->m_localTransform;

		for (auto& kvp : map) {
			if (kvp.second->map.count(name)) {
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

	void ActorAdjustments::UpdateSaved(std::vector<std::pair<std::string, std::string>>* adjustments)
	{
		for (auto& kvp : *adjustments) {
			std::shared_ptr<Adjustment> adjustment = CreateAdjustment(kvp.first);
			adjustment->esp = kvp.second;
			adjustment->persistent = true;
			LoadAdjustmentFile(kvp.first, &nodeSets->all, &adjustment->map);
		}
	}

	std::shared_ptr<Adjustment> ActorAdjustments::CreateAdjustment(std::string name)
	{
		lock.Lock();
		std::shared_ptr<Adjustment> adjustment = std::make_shared<Adjustment>(nextHandle, name);
		map[nextHandle] = adjustment;
		list.push_back(adjustment);
		nextHandle++;
		lock.Release();
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
		SimpleLocker locker(&lock);
		if (map.count(handle))
			return map[handle];
		return nullptr;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::GetListAdjustment(UInt32 index)
	{
		SimpleLocker locker(&lock);
		if (index < list.size())
			return list[index];
		return nullptr;
	}

	void ActorAdjustments::RemoveAdjustment(UInt32 handle)
	{
		lock.Lock();
		auto it = list.begin();
		while (it != list.end()) {
			if ((*it)->handle == handle)
				break;
			it++;
		}
		list.erase(it);

		map.erase(handle);

		lock.Release();

		//if (handle == nextHandle - 1)
		//	nextHandle--;
	}

	void ActorAdjustments::RemoveListAdjustment(UInt32 index)
	{
		lock.Lock();

		auto it = list.begin() + index;
		map.erase((*it)->handle);
		list.erase(it);

		lock.Release();
	}

	std::shared_ptr<Adjustment> ActorAdjustments::LoadAdjustment(std::string filename)
	{
		std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);
		if (!adjustment) return nullptr;
		LoadAdjustmentFile(filename, &nodeSets->all, &adjustment->map);
		return adjustment;
	}

	UInt32 ActorAdjustments::LoadAdjustment(std::string filename, std::string espName, bool persistent, bool hidden)
	{
		std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename);
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
		lock.Lock();
		for (auto& adjustment : map) {
			functor(adjustment.second);
		}
		lock.Release();
	}

	void ActorAdjustments::GetModAdjustmentsPapyrus(std::string espName, VMArray<UInt32>* result)
	{
		lock.Lock();
		for (auto& adjustment : map) {
			if (adjustment.second->esp == espName)
				result->Push(&adjustment.second->handle);
		}
		lock.Release();
	}

	bool ActorAdjustments::RemoveMod(std::string espName)
	{
		bool updated = false;
		lock.Lock();

		auto it = list.begin();
		while (it != list.end()) {
			if ((*it)->esp == espName) {
				updated = true;
				map.erase((*it)->handle);
				it = list.erase(it);
			}
		}

		lock.Release();
		return updated;
	}

	void ActorAdjustments::OverrideTransform(std::shared_ptr<Adjustment> adjustment, std::string name, NiTransform transform)
	{
		SimpleLocker locker(&lock);
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
		SimpleLocker locker(&lock);
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

		return actorAdjustmentCache[actor->formID];
	}

	NodeSets* AdjustmentManager::GetNodeSets(UInt64 key) {
		SimpleLocker locker(&lock);

		if (nodeSets.count(key))
			return &nodeSets[key];
		return nullptr;
	}

	void AdjustmentManager::SetNodeSets(UInt64 key, NodeSets sets)
	{
		lock.Lock();
		nodeSets[key] = sets;
		lock.Release();
	}

	std::unordered_map<std::string, NiTransform>* AdjustmentManager::GetDefaultAdjustment(std::string filename)
	{
		SimpleLocker locker(&lock);
		if (defaultAdjustmentCache.count(filename))
			return &defaultAdjustmentCache[filename];
		return nullptr;
	}

	std::vector<std::string>* AdjustmentManager::GetDefaultAdjustments(UInt64 key)
	{
		SimpleLocker locker(&lock);
		if (defaultAdjustments.count(key))
			return &defaultAdjustments[key];
		return nullptr;
	}

	void AdjustmentManager::SetDefaultAdjustments(UInt64 key, std::vector<std::string> list)
	{
		lock.Lock();
		defaultAdjustments[key] = list;
		lock.Release();
	}

	std::vector<std::pair<std::string, std::string>>* AdjustmentManager::GetSavedAdjustments(UInt32 formId)
	{
		SimpleLocker locker(&lock);
		if (savedAdjustmentCache.count(formId))
			return &savedAdjustmentCache[formId];
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
		lock.Lock();
		for (auto& adjustments : actorAdjustmentCache) {
			if (adjustments.second->RemoveMod(espName))
				adjustments.second->UpdateAllAdjustments();
		}
		lock.Release();
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

	NodeMap* AdjustmentManager::GetActorNodeMap(TESObjectREFR* actor, NodeSet* nodeSet) {
		TESNPC* npc = (TESNPC*)actor->baseForm;
		if (actorNodeMapCache.count(npc->formID))
			return &actorNodeMapCache[npc->formID];
		NiNode* root = actor->GetActorRootNode(false);
		actorNodeMapCache[npc->formID] = CreateNodeMap(root, nodeSet);
		return &actorNodeMapCache[npc->formID];
	}

	NodeMap* AdjustmentManager::GetActorBaseMap(TESObjectREFR* refr, NodeSet* set)
	{
		std::string filename = (*getRefrModelFilename)(refr);
		if (baseNodeMapCache.count(filename))
			return &baseNodeMapCache[filename];
		TESNPC* npc = (TESNPC*)refr->baseForm;
		NiNode* root = (*getBaseModelRootNode)(npc, refr);
		baseNodeMapCache[filename] = CreateNodeMap(root, set);
		return &baseNodeMapCache[filename];
	}

	void AdjustmentManager::UpdateActor(Actor* actor, TESNPC* npc, bool loaded) {
		std::shared_ptr<ActorAdjustments> adjustments = GetActorAdjustments(actor->formID);
		if (!loaded) {
			if (adjustments) {
				actorAdjustmentCache.erase(actor->formID);
			}
			return;
		}
		UInt32 race = npc->race.race->formID;
		bool isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);

		if (!adjustments) {
			adjustments = std::make_shared<ActorAdjustments>(race, isFemale, actor, npc);
			actorAdjustmentCache[actor->formID] = adjustments;
		}

		if (adjustments->UpdateKey(race, isFemale)) {
			UInt64 key = race;
			if (isFemale)
				key |= 0x100000000;
			adjustments->nodeSets = GetNodeSets(key);
			if (adjustments->nodeSets) {
				adjustments->nodeMap = GetActorNodeMap(actor, &adjustments->nodeSets->all);
				adjustments->nodeMapBase = GetActorBaseMap(actor, &adjustments->nodeSets->all);
				adjustments->UpdateDefault(GetDefaultAdjustments(key));
				adjustments->UpdateSaved(GetSavedAdjustments(actor->formID));
				adjustments->UpdateAllAdjustments();
			}
		}
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
		lock.Lock();

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

		lock.Release();
	}

	void AdjustmentManager::SerializeLoad(const F4SESerializationInterface* ifc) {
		SimpleLocker locker(&lock);

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
								savedAdjustmentCache[formId].push_back(std::make_pair(name, mod));
						}
					}
					break;
				}
			}
		}
	}

	void AdjustmentManager::SerializeRevert(const F4SESerializationInterface* ifc) {
		lock.Lock();
		actorNodeMapCache.clear();
		actorAdjustmentCache.clear();
		lock.Release();
	}
}