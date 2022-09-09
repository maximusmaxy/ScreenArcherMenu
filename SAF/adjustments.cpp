#include "adjustments.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameTypes.h"

#include "io.h"
#include "util.h"
#include "conversions.h"

#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <filesystem>

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

		updated = true;
	}

	void Adjustment::SetTransformPos(std::string name, float x, float y, float z)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform transform = map.count(name) ? map[name] : TransformIdentity();

		transform.pos.x = x;
		transform.pos.y = y;
		transform.pos.z = z;

		map[name] = transform;

		updated = true;
	}

	void Adjustment::SetTransformRot(std::string name, float yaw, float pitch, float roll)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform transform = map.count(name) ? map[name] : TransformIdentity();

		//MatrixFromEulerRPY(transform.rot, roll * DEGREE_TO_RADIAN, pitch * DEGREE_TO_RADIAN, yaw * DEGREE_TO_RADIAN);
		MatrixFromDegree(transform.rot, yaw, pitch, roll);

		map[name] = transform;

		updated = true;
	}

	void Adjustment::SetTransformSca(std::string name, float scale)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiTransform transform = map.count(name) ? map[name] : TransformIdentity();

		transform.scale = scale;
		map[name] = transform;

		updated = true;
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

	void Adjustment::Rename(std::string newName) 
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		name = newName;

		updated = true;
	}

	void Adjustment::SetScale(float newScale)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		scale = newScale;

		updated = true;
	}

	void Adjustment::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		map.clear();
	}

	ActorAdjustmentState ActorAdjustments::IsValid()
	{
		TESForm* form = LookupFormByID(formId);
		if (!form) 
			return kActorInvalid;

		Actor* formActor = DYNAMIC_CAST(form, TESForm, Actor);
		if (!formActor)
			return kActorInvalid;

		//loaded data
		if (!formActor->unkF0)
			return kActorInvalid;

		//deleted
		if (formActor->flags & TESForm::kFlag_IsDeleted)
			return kActorInvalid;

		auto result = kActorValid;

		//updated actor
		if (formActor != actor) {
			result = kActorUpdated;
		}

		TESNPC* formNpc = DYNAMIC_CAST(formActor->baseForm, TESForm, TESNPC);
		if (!formNpc)
			return kActorInvalid;

		//updated npc
		if (formNpc != npc) {
			result = kActorUpdated;
		}

		//updated gender
		bool newFemale = (CALL_MEMBER_FN(formNpc, GetSex)() == 1 ? true : false);
		if (isFemale != newFemale) {
			result = kActorUpdated;
		}

		//updated race
		TESRace* actorRace = formActor->GetActorRace();
		if (!actorRace)
			return kActorInvalid;	

		UInt32 newRace = actorRace->formID;
		if (race != newRace) {
			result = kActorUpdated;
		}

		return result;
	}

	void ActorAdjustments::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		map.clear();
		list.clear();
		removedAdjustments.clear();
		poseHandle = 0;
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
		if (!nodeMapBase || !nodeSets) return;

		for (auto& name : nodeSets->all) {
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

	void ActorAdjustments::UpdatePersistentAdjustments(AdjustmentUpdateData& data) {
		std::unordered_set<std::string> removeSet;
		
		if (data.persistents) {
			for (auto& persistent : *data.persistents) {
				if (!persistent.file.empty()) {
					removeSet.insert(persistent.file);
				}
				switch (persistent.type) {
				case kAdjustmentSerializeAdd:
				{
					std::shared_ptr<Adjustment> adjustment = CreateAdjustment(persistent.name);
					adjustment->file = persistent.file;
					adjustment->mod = persistent.mod;
					adjustment->map = persistent.map;
					adjustment->scale = persistent.scale;
					break;
				}
				case kAdjustmentSerializeLoad:
				{
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(persistent.file, true);
					if (adjustment != nullptr) {
						adjustment->name = persistent.name;
						adjustment->mod = persistent.mod;
						adjustment->scale = persistent.scale;
					}
					break;
				}
				case kAdjustmentSerializeRemove:
				{
					removedAdjustments.insert(persistent.file);
					break;
				}
				case kAdjustmentSerializeDefault:
				{
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(persistent.file, true);
					if (adjustment != nullptr) {
						adjustment->name = persistent.file;
						adjustment->isDefault = true;
						adjustment->mod = persistent.mod;
					}
					break;
				}
				}
			}
		}
		
		if (data.defaults) {
			for (auto& def : *data.defaults) {
				if (!removeSet.count(def)) {
					removeSet.insert(def);
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(def, true);
					if (adjustment != nullptr) {
						adjustment->isDefault = true;
					}
				}
			}
		}
		
		//Only load uniques if there are no persistents, add the unique into the persistents then ignore it
		if (data.uniques && !data.persistents) {
			for (auto& kvp : *data.uniques) {
				if (!removeSet.count(kvp.first)) {
					std::shared_ptr<Adjustment> adjustment = LoadAdjustment(kvp.first, true);
					if (adjustment != nullptr) {
						adjustment->file.clear();
					}
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

	UInt32 ActorAdjustments::CreateAdjustment(std::string name, std::string modName)
	{
		std::shared_ptr<Adjustment> adjustment = CreateAdjustment(name);
		adjustment->mod = modName;
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

	void ActorAdjustments::RemoveAdjustment(std::string name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto it = list.begin();
		while (it != list.end()) {
			if ((*it)->name == name)
				break;
			++it;
		}

		if (it == list.end()) {
			return;
		}

		UInt32 handle = (*it)->handle;
		if (handle == poseHandle)
			poseHandle = 0;

		list.erase(it);
		map.erase(handle);
	}

	bool ActorAdjustments::HasAdjustment(std::string name)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& it : list) {
			if (it->name == name) return true;
		}

		return false;
	}

	std::unordered_set<std::string> ActorAdjustments::GetAdjustmentNames()
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		std::unordered_set<std::string> set;

		for (auto& it : list) {
			set.insert(std::string(it->name));
		}

		return set;
	}

	UInt32 ActorAdjustments::GetAdjustmentIndex(UInt32 handle)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (int i = 0; i < list.size(); ++i) {
			if (list[i]->handle == handle) {
				return i;
			}
		}

		return -1;
	}

	bool ActorAdjustments::MoveAdjustment(UInt32 fromIndex, UInt32 toIndex)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (fromIndex == toIndex || fromIndex < 0 || toIndex < 0 || fromIndex >= list.size() || toIndex >= list.size()) return false;

		std::shared_ptr<Adjustment> adjustment = list[fromIndex];

		if (fromIndex < toIndex) {
			for (int i = fromIndex; i < toIndex; ++i) {
				list[i] = list[i + 1];
			}
		}
		else {
			for (int i = fromIndex; i > toIndex; --i) {
				list[i] = list[i - 1];
			}
		}

		list[toIndex] = adjustment;

		return true;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::LoadAdjustment(std::string filename, bool cached)
	{
		if (cached) {
			auto adjustmentFile = g_adjustmentManager.GetAdjustmentFile(filename);
			if (adjustmentFile) {

				std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);

				if (removedAdjustments.count(filename)) {
					removedAdjustments.erase(filename);
					adjustment->isDefault = true;
				}

				if (nodeSets) {
					adjustment->CopyMap(adjustmentFile, &nodeSets->all);
				}
				
				adjustment->file = filename;
				return adjustment;
			}
		}

		std::unordered_map<std::string, NiTransform> adjustmentMap;

		if (LoadAdjustmentFile(filename, &adjustmentMap)) {
			if (cached) {
				g_adjustmentManager.SetAdjustmentFile(filename, adjustmentMap);
			}
			std::shared_ptr<Adjustment> adjustment = CreateAdjustment(filename);

			if (removedAdjustments.count(filename)) {
				removedAdjustments.erase(filename);
				adjustment->isDefault = true;
			}
			
			if (nodeSets) {
				adjustment->CopyMap(&adjustmentMap, &nodeSets->all);
			}
			
			adjustment->file = filename;
			return adjustment;
		}

		return nullptr;
	}

	UInt32 ActorAdjustments::LoadAdjustmentHandle(std::string filename, std::string modName, bool cached)
	{
		std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, cached);
		if (!adjustment) return 0;

		adjustment->mod = modName;
		return adjustment->handle;
	}

	void ActorAdjustments::SaveAdjustment(std::string filename, UInt32 handle)
	{
		std::shared_ptr<Adjustment> adjustment = GetAdjustment(handle);
		if (!adjustment) return;

		if (SaveAdjustmentFile(filename, adjustment)) {
			//if saving to a new file from the default it needs to be removed 
			if (adjustment->isDefault && adjustment->name != filename) {
				removedAdjustments.insert(adjustment->name);
				adjustment->isDefault = false;
			}
			adjustment->name = filename;
			adjustment->file = filename;
			adjustment->updated = false;
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

	bool ActorAdjustments::HasNode(std::string name)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		if (!nodeMap.count(name)) return false;
		if (!nodeMapBase->count(name)) return false;

		return true;
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
		adjustment->updated = true;
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
		adjustment->updated = true;
	}

	void ActorAdjustments::RotateTransformXYZ(std::shared_ptr<Adjustment> adjustment, std::string name, UInt32 type, float scalar)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (!nodeSets->baseMap.count(name)) return;
		std::string baseName = nodeSets->baseMap[name];

		if (!nodeMap.count(baseName)) return;
		NiAVObject* baseNode = nodeMap[baseName];

		NiTransform transform = baseNode->m_localTransform * adjustment->GetTransformOrDefault(name);
		RotateMatrixXYZ(transform.rot, type, scalar);
		//RotateMatrixAxis(transform.rot, type, scalar);

		adjustment->SetTransform(name, NegateNiTransform(baseNode->m_localTransform, transform));
		adjustment->updated = true;
	}

	void ActorAdjustments::SavePose(std::string filename, std::unordered_set<UInt32> handles) {
		std::lock_guard<std::shared_mutex> lock(mutex);

		//if (handles.size() == 0) return;

		//Order matters for applying translations so get them from the list in the correct order
		std::vector<std::shared_ptr<Adjustment>> adjustments;
		for (auto& adjustment : list) {
			if (handles.count(adjustment->handle))
				adjustments.push_back(adjustment);
		}

		//if (adjustments.size() == 0) return;

		TransformMap poseMap;
		for (auto& overrider : nodeSets->pose) {
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
			else {
				_DMESSAGE("could not find overrider node", overrider.c_str());
			}
		}

		SavePoseFile(filename, &poseMap);
	}

	bool ActorAdjustments::LoadPose(std::string path) {
		std::lock_guard<std::shared_mutex> lock(mutex);

		TransformMap poseMap;
		
		if (LoadPosePath(path, &poseMap)) {

			std::shared_ptr<Adjustment> adjustment;

			std::string filename = getFilename(path);

			for (auto& kvp : map) {
				if (kvp.second->file.empty() && kvp.second->handle != poseHandle) {
					kvp.second->Clear();
				}
			}

			if (poseHandle && map.count(poseHandle)) {
				adjustment = map[poseHandle];
				adjustment->Clear();
				adjustment->name = filename;
				adjustment->scale = 1.0f;
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
			
			return true;
		}

		return false;
	}

	void ActorAdjustments::ResetPose() {
		if (!poseHandle) 
			return;

		RemoveAdjustment(poseHandle);
	}

	void ActorAdjustments::LoadDefaultAdjustment(std::string filename, bool clear, bool enable) {
		if (clear) 
			RemoveDefaultAdjustments();

		if (filename.empty())
			return;

		if (enable) {
			std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, true);
			if (!adjustment) 
				return;

			adjustment->isDefault = true;
		} 
		else {
			RemoveAdjustment(filename);
		}
	}

	void ActorAdjustments::RemoveDefaultAdjustments() {
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto it = list.begin();

		while (it != list.end()) {
			if ((*it)->isDefault) {
				map.erase((*it)->handle);
				it = list.erase(it);
			}
			else {
				++it;
			}
		}

		removedAdjustments.clear();
	}

	std::shared_ptr<ActorAdjustments> AdjustmentManager::GetActorAdjustments(UInt32 formId) {
		std::shared_lock<std::shared_mutex> lock(actorMutex);

		if (actorAdjustmentCache.count(formId)) {

			std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
			
			auto valid = adjustments->IsValid();

			if (valid == kActorValid) {
				return adjustments;
			} else if (valid == kActorUpdated) {
				//attempt update
				if (!UpdateActorCache(adjustments)) {
					return nullptr;
				}
			}
			else if (valid == kActorInvalid) {
				RemoveNodeMap(adjustments->baseRoot);
				actorAdjustmentCache.erase(adjustments->formId);
			}
		}
		
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

			if (!UpdateActorCache(adjustments))
				return nullptr;
		}

		return adjustments;
	}

	void AdjustmentManager::ForEachActorAdjustments(const std::function<void(std::shared_ptr<ActorAdjustments> adjustments)>& functor) 
	{
		for (auto& kvp : actorAdjustmentCache)
		{
			if (kvp.second->IsValid() == kActorValid) {
				functor(kvp.second);
			}
		}
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
		if (!actor) 
			return nullptr;

		TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);
		if (!npc) 
			return nullptr;

		std::shared_ptr<ActorAdjustments> adjustments = std::make_shared<ActorAdjustments>(actor, npc);
		
		if (!UpdateActorCache(adjustments))
			return nullptr;

		return adjustments;
	}

	std::unordered_set<std::string>* AdjustmentManager::GetDefaultAdjustments(UInt32 race, bool isFemale)
	{
		UInt64 key = race;
		if (isFemale)
			key |= 0x100000000;

		if (defaultAdjustments.count(key))
			return &defaultAdjustments[key];

		return nullptr;
	}

	void AdjustmentManager::LoadFiles()
	{
		if (!filesLoaded) {
			filesLoaded = true;
			LoadAllFiles();
		}
	}

	RelocPtr<ProcessListsActor*> processListsActors(0x58CEE98);

	typedef bool(*_GetActorFromHandle)(const UInt32& handle, NiPointer<Actor>& actor);
	RelocAddr<_GetActorFromHandle> GetActorFromHandle(0x23870);

	void ForEachProcessActor(const std::function<void(Actor*)>& functor)
	{
		UInt32* handles = (*processListsActors)->handles;
		UInt32 count = (*processListsActors)->count;
		for (UInt32* handle = handles; handle < handles + count; ++handle)
		{
			NiPointer<Actor> actor;
			GetActorFromHandle(*handle, actor);
			if (actor) {
				functor(actor);
			}
		}
	}

	void AdjustmentManager::GameLoaded()
	{
		PlayerCharacter* player = *g_player;
		if (player) {
			ActorLoaded(player, true);
		}
		
		ForEachProcessActor([&](Actor* actor) {
			ActorLoaded(actor, true);
		});

		gameLoaded = true;
	}

	void AdjustmentManager::ActorLoaded(Actor* actor, bool loaded)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		std::shared_ptr<ActorAdjustments> adjustments = nullptr;

		if (actorAdjustmentCache.count(actor->formID))
			adjustments = actorAdjustmentCache[actor->formID];
		
		if (!loaded) {
			if (adjustments) {
				if (adjustments->actor && adjustments->actor->flags & TESForm::kFlag_Persistent) {
					StorePersistentAdjustments(adjustments);
				}
				RemoveNodeMap(adjustments->baseRoot);
				actorAdjustmentCache.erase(adjustments->formId);
			}
			return;
		}

		if (!adjustments) {
			TESNPC* npc = DYNAMIC_CAST(actor->baseForm, TESForm, TESNPC);

			if (!npc) {
				_Log("base form not found for actor id", actor->formID);
				return;
			}

			adjustments = std::make_shared<ActorAdjustments>(actor, npc);
		}

		UpdateActorCache(adjustments);
	}

	bool AdjustmentManager::UpdateActorCache(std::shared_ptr<ActorAdjustments> adjustments) {
		if (UpdateActor(adjustments)) {
			if (!actorAdjustmentCache.count(adjustments->formId)) {
				actorAdjustmentCache.emplace(adjustments->formId, adjustments);
			}
		}
		else {
			if (actorAdjustmentCache.count(adjustments->formId)) {
				actorAdjustmentCache.erase(adjustments->formId);
			}
			return false;
		}
		
		return true;
	}

	bool IsRootValid(NiNode* root, NodeSets* set) {
		if (!root)
			return false;

		NiAVObject* rootNode = root->GetObjectByName(&set->rootName);

		if (!rootNode)
			return false;

		static BSFixedString safVersion("SAF_Version");

		if (!rootNode->HasExtraData(safVersion))
			return false;

		return true;
	}

	bool AdjustmentManager::UpdateActor(std::shared_ptr<ActorAdjustments> adjustments) {
		adjustments->Clear();

		if (adjustments->baseRoot)
			RemoveNodeMap(adjustments->baseRoot);

		auto valid = adjustments->IsValid();

		if (valid == kActorInvalid)
			return false;
		else if (valid == kActorUpdated) {
			TESForm* actorForm = LookupFormByID(adjustments->formId);
			if (!actorForm)
				return false;

			adjustments->actor = DYNAMIC_CAST(actorForm, TESForm, Actor);
			if (!adjustments->actor)
				return false;

			adjustments->npc = DYNAMIC_CAST(adjustments->actor->baseForm, TESForm, TESNPC);
			if (!adjustments->npc)
				return false;

			adjustments->isFemale = (CALL_MEMBER_FN(adjustments->npc, GetSex)() == 1 ? true : false);

			TESRace* actorRace = adjustments->actor->GetActorRace();
			if (!actorRace)
				return false;

			adjustments->race = actorRace->formID;
		}

		adjustments->root = adjustments->actor->GetActorRootNode(false);
		if (!adjustments->root)
			return false;

		adjustments->nodeSets = GetNodeSets(adjustments->race, adjustments->isFemale);
		if (!adjustments->nodeSets) 
			return false;

		if (!IsRootValid(adjustments->root, adjustments->nodeSets)) 
			return false;

		adjustments->nodeMap = CreateNodeMap(adjustments->root, adjustments->nodeSets);

		adjustments->baseRoot = (*getBaseModelRootNode)(adjustments->npc, adjustments->actor);

		if (!IsRootValid(adjustments->baseRoot, adjustments->nodeSets))
			return false;

		adjustments->nodeMapBase = GetCachedNodeMap(adjustments->baseRoot, adjustments->nodeSets);
		if (!adjustments->nodeMapBase)
			return false;

		adjustments->CreateAdjustment("New Adjustment");

		AdjustmentUpdateData data;
		data.defaults = GetDefaultAdjustments(adjustments->race, adjustments->isFemale);

		if (uniqueAdjustments.count(adjustments->npc->formID))
			data.uniques = &uniqueAdjustments[adjustments->npc->formID];

		if (persistentAdjustments.count(adjustments->formId))
			data.persistents = &persistentAdjustments[adjustments->formId];

		adjustments->UpdatePersistentAdjustments(data);

		adjustments->UpdateAllAdjustments();

		return true;
	}

	RelocAddr<UInt64> bsFlattenedBoneTreeVFTable(0x2E14E38);

	typedef UInt32 (*_GetBoneTreeKey)(NiAVObject* node, BSFixedString* name);
	RelocAddr<_GetBoneTreeKey> GetBoneTreeKey(0x1BA1860);

	typedef NiAVObject* (*_GetBoneTreeNode)(NiAVObject* node, SInt32 key, char* unk);
	RelocAddr<_GetBoneTreeNode> GetBoneTreeNode(0x1BA1910);

	NiAVObject* GetNodeFromFlattenedTrees(std::vector<NiAVObject*>& trees, BSFixedString name) {
		for (auto& tree : trees) {
			SInt32 key = GetBoneTreeKey(tree, &name);
			if (key >= 0) {
				char unk = 1;
				NiAVObject* node = GetBoneTreeNode(tree, key, &unk);
				if (node) 
					return node;
			}
		}

		return nullptr;
	}

	NodeMap AdjustmentManager::CreateNodeMap(NiNode* root, NodeSets* set) {
		NodeMap boneMap;

		std::vector<NiAVObject*> boneTrees;
		std::unordered_set<std::string> remainingNodes(set->allOrBase);
		UInt64 flattenedBoneTreeVFTableAddr = bsFlattenedBoneTreeVFTable.GetUIntPtr();

		root->Visit([&](NiAVObject* object) {
			//check for flattend bone tree VFtable and add it to be searched for missing nodes after DFS is complete
			if (*(UInt64*)object == flattenedBoneTreeVFTableAddr) {
				boneTrees.push_back(object);
			}
			if (object->m_name) {
				std::string nodeName(object->m_name.c_str());
				if (set->allOrBase.count(object->m_name.c_str())) {
					if (set->fixedConversion.count(nodeName)) {
						boneMap[set->fixedConversion[nodeName]] = object;
						remainingNodes.erase(nodeName);
					}
				}
				return (remainingNodes.size() <= 0);
			}
		});

		if (remainingNodes.size() <= 0) return boneMap;

		for (auto& nodeName : remainingNodes) {
			if (set->fixedConversion.count(nodeName)) {
				BSFixedString name(nodeName.c_str());
				std::string converted = set->fixedConversion[nodeName];

				//Search flattened bone trees for remaining nodes
				NiPointer<NiAVObject> flattenedBone = GetNodeFromFlattenedTrees(boneTrees, name);
				if (flattenedBone) {
					boneMap[converted] = flattenedBone;
				}
				else {

					//Shouldn't need to do this to find missing bones, if this is triggered something is wrong with the method above.
					NiPointer<NiAVObject> findBone = root->GetObjectByName(&name);
					if (findBone) {
						boneMap[converted] = findBone;
					}
				}
			}
		}

		return boneMap;
	}

	NodeMap* AdjustmentManager::GetCachedNodeMap(NiNode* root, NodeSets* set)
	{
		std::lock_guard<std::shared_mutex> lock(nodeMapMutex);

		if (nodeMapCache.count(root)) {
			//update ref count
			nodeMapCache[root].refs++;

			return &nodeMapCache[root].map;
		}

		nodeMapCache[root] = NodeMapRef(CreateNodeMap(root, set));

		return &nodeMapCache[root].map;
	}

	void AdjustmentManager::RemoveNodeMap(NiNode* root) {
		std::lock_guard<std::shared_mutex> lock(nodeMapMutex);

		if (nodeMapCache.count(root)) {
			if (--nodeMapCache[root].refs <= 0)
				nodeMapCache.erase(root);
		}
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

	void AdjustmentManager::CreateNewAdjustment(UInt32 formId, const char* name, const char* mod) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		adjustments->CreateAdjustment(name, mod);
	}

	void  AdjustmentManager::SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		adjustments->SaveAdjustment(filename, handle);
	}

	bool AdjustmentManager::LoadAdjustment(UInt32 formId, const char* filename, const char* mod) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return false;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		return adjustments->LoadAdjustmentHandle(filename, mod);
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
		case kAdjustmentTransformRotate:
			adjustments->RotateTransformXYZ(adjustment, message->key, message->a, message->b);
			break;
		}
	}

	//void AdjustmentManager::NegateAdjustments(UInt32 formId, UInt32 handle, const char* groupName) {
	//	std::lock_guard<std::shared_mutex> lock(actorMutex);

	//	if (!actorAdjustmentCache.count(formId)) return;

	//	std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
	//	if (!adjustments) return;

	//	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
	//	if (!adjustment) return;

	//	adjustments->NegateTransformGroup(adjustment, groupName);
	//}

	bool AdjustmentManager::LoadPose(UInt32 formId, const char* filename) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return false;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return false;

		return adjustments->LoadPose(filename);
	}

	void AdjustmentManager::ResetPose(UInt32 formId) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
		if (!adjustments) return;

		adjustments->ResetPose();
	}

	void AdjustmentManager::LoadDefaultAdjustment(UInt32 formId, bool isFemale, const char* filename, bool npc, bool clear, bool enable)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		std::string name(filename ? filename : "");

		//if single npc target just use the regular adjustment functions
		if (npc) { 
			//single targeting is currently disabled
			
			//std::shared_ptr<ActorAdjustments> adjustments = GetActorAdjustments(formId);
			//if (!adjustments) return;

			//if (!filename) {
			//	adjustments->RemoveDefaultAdjustments();
			//}
			//else {
			//	
			//}
		}
		//target all race/gender
		else { 
			UInt64 key = formId;
			if (isFemale)
				key |= 0x100000000;

			if (clear) {
				defaultAdjustments.erase(key);
			}

			if (!name.empty()) {
				if (enable) {
					defaultAdjustments[key].insert(name);

					//recache the adjustment file before loading
					std::unordered_map<std::string, NiTransform> adjustmentMap;
					if (LoadAdjustmentFile(filename, &adjustmentMap)) {
						SetAdjustmentFile(filename, adjustmentMap);
					}
				}
				else {
					defaultAdjustments[key].erase(name);
				}
			}

			//load default adjustments
			ForEachActorAdjustments([&](std::shared_ptr<ActorAdjustments> adjustments) {
				if (adjustments->race == formId && adjustments->isFemale == isFemale)
				{
					adjustments->LoadDefaultAdjustment(name, clear, enable);
					adjustments->UpdateAllAdjustments();
				}
			});
		}
	}

	bool AdjustmentManager::MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex) 
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);
		
		if (!actorAdjustmentCache.count(formId)) return false;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		return adjustments->MoveAdjustment(fromIndex, toIndex);
	}

	void AdjustmentManager::RenameAdjustment(UInt32 formId, UInt32 handle, const char* name)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(handle);
		if (!adjustment) return;

		adjustment->Rename(name);
	}

	void AdjustmentManager::RemoveMod(BSFixedString modName)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		ForEachActorAdjustments([&](std::shared_ptr<ActorAdjustments> adjustments) {
			bool removed = adjustments->RemoveMod(modName);
			if (removed) adjustments->UpdateAllAdjustments();
		});
	}
}