#include "adjustments.h"

#include "f4se/GameData.h"
#include "f4se/GameRTTI.h"
#include "f4se/GameTypes.h"
#include "f4se/NiExtraData.h"

#include "util.h"
#include "io.h"
#include "settings.h"

#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <filesystem>

namespace SAF {
	AdjustmentManager g_adjustmentManager;

	RelocAddr<GetBaseModelRootNode> getBaseModelRootNode(0x323BB0);
	RelocAddr<GetRefrModelFilename> getRefrModelFilename(0x408360);

#define NodeKeyBufferSize 128
#define NodeOffsetPostfix "_Offset"
#define NodePosePostfix "_Pose"
#define NodeOverridePostfix "_Override"

	const NodeKey GetNodeKeyFromString(const char* str)
	{
		if (!str)
			return NodeKey();

		UInt32 length = strlen(str);

		if (!length)
			return NodeKey();

		UInt32 offsetLength = ComparePostfix(str, length, NodeOffsetPostfix, 7);
		if (offsetLength)
		{
			if (offsetLength > NodeKeyBufferSize) {
				_Logs("Node name too long", str);
				return NodeKey();
			}
			char buf[NodeKeyBufferSize];
			memcpy(buf, str, offsetLength);
			buf[offsetLength] = '\0';
			return NodeKey(BSFixedString(buf), true);
		}

		//Check for both pose/override for legacy support
		UInt32 poseLength = ComparePostfix(str, length, NodePosePostfix, 5);
		if (!poseLength) {
			poseLength = ComparePostfix(str, length, NodeOverridePostfix, 9);
		}
		if (poseLength)
		{
			if (poseLength > NodeKeyBufferSize) {
				_Logs("Node name too long", str);
				return NodeKey();
			}
			char buf[NodeKeyBufferSize];
			memcpy(buf, str, poseLength);
			buf[poseLength] = '\0';
			return NodeKey(BSFixedString(buf), false);
		}

		return NodeKey(BSFixedString(str), false);
	}

	std::unordered_map<BSFixedString, const char*, BSFixedStringHash, BSFixedStringKeyEqual> toUpperMap;

	std::string GetNodeKeyName(const NodeKey& nodeKey)
	{
		//Force certain strings to upper to prevent case sensitivity errors of external programs
		//TODO: should make this an external file and force lower as well
		if (toUpperMap.empty()) {
			toUpperMap.emplace(BSFixedString("COM"), "COM");
			toUpperMap.emplace(BSFixedString("HEAD"), "HEAD");
			toUpperMap.emplace(BSFixedString("SPINE1"), "SPINE1");
			toUpperMap.emplace(BSFixedString("SPINE2"), "SPINE2");
			toUpperMap.emplace(BSFixedString("WEAPON"), "WEAPON");
		}

		auto it = toUpperMap.find(nodeKey.name);
		std::string str((it != toUpperMap.end()) ? it->second : nodeKey.name);

		if (nodeKey.offset) {
			str += g_settings.offset;
		}

		return str;
	}

	SamTransform* GetFromTransformMap(TransformMap& map, const NodeKey& key) {
		auto found = map.find(key);

		if (found == map.end())
			return nullptr;

		return &found->second;
	}

	BSFixedString* GetFromBSFixedStringMap(BSFixedStringMap& map, const BSFixedString& key) {
		auto found = map.find(key);

		if (found == map.end())
			return nullptr;

		return &found->second;
	}

	NiAVObject* GetFromNodeMap(NodeMap& map, const BSFixedString& key) {
		auto found = map.find(key);

		if (found == map.end())
			return nullptr;

		return found->second;
	}

	bool TransformMapIsDefault(TransformMap& map)
	{
		for (auto& kvp : map) {
			if (!kvp.second.IsDefault()) {
				return false;
			}
		}
		return true;
	}

	PersistentAdjustment::PersistentAdjustment(std::shared_ptr<Adjustment> adjustment, UInt32 _updateType)
	{
		type = adjustment->type;
		name = adjustment->name;
		file = adjustment->file;
		mod = adjustment->mod;
		scale = adjustment->scale;
		updated = adjustment->updated;
		updateType = _updateType;

		if (StoreMap()) {
			map = adjustment->map;
		}
	}

	SamTransform* Adjustment::GetTransform(const NodeKey& key)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return GetFromTransformMap(map, key);
	}

	SamTransform Adjustment::GetTransformOrDefault(const NodeKey& key)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		auto found = map.find(key);
		return found != map.end() ? found->second : SamTransform();
	}

	void Adjustment::SetTransform(const NodeKey& key, SamTransform& transform)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		map[key] = transform;
		UpdateScale(key, transform);
	}

	bool Adjustment::HasTransform(const NodeKey& key)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return map.count(key);
	}

	void Adjustment::ResetTransform(const NodeKey& key)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		
		SamTransform* transform = GetFromTransformMap(map, key);

		if (transform) 
		{
			*transform = SamTransform();
			scaled[key] = *transform;
			updated = true;
		}
	}

	void Adjustment::SetTransformPos(const NodeKey& key, float x, float y, float z)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto found = map.find(key);
		SamTransform transform = found != map.end() ? found->second: SamTransform();

		transform.pos.x = x;
		transform.pos.y = y;
		transform.pos.z = z;

		map[key] = transform;
		UpdateScale(key, transform);

		updated = true;
	}

	void Adjustment::SetTransformRot(const NodeKey& key, float yaw, float pitch, float roll)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto found = map.find(key);
		SamTransform transform = found != map.end() ? found->second : SamTransform();

		MatrixFromEulerYPR2(transform.rot, yaw * DEGREE_TO_RADIAN, pitch * DEGREE_TO_RADIAN, roll * DEGREE_TO_RADIAN);

		map[key] = transform;
		UpdateScale(key, transform);

		updated = true;
	}

	void Adjustment::SetTransformSca(const NodeKey& key, float scale)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto found = map.find(key);
		SamTransform transform = found != map.end() ? found->second : SamTransform();

		transform.scale = scale;
		map[key] = transform;
		UpdateScale(key, transform);

		updated = true;
	}

	void Adjustment::ForEachTransform(const std::function<void(const NodeKey*, SamTransform*)>& functor)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		for (auto& transform : map) {
			functor(&transform.first, &transform.second);
		}
	}
	
	void Adjustment::ForEachTransformOrDefault(const std::function<void(const NodeKey*, SamTransform*)>& functor, NodeSet* set)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		for (auto& nodeName : *set) {
			auto found = map.find(nodeName);
			SamTransform transform = found != map.end() ? found->second : SamTransform();
			functor(&nodeName, &transform);
		}
	}

	TransformMap* Adjustment::GetMap()
	{
		std::shared_lock<std::shared_mutex> lock(mutex);
		return &map;
	}

	void Adjustment::SetMap(TransformMap newMap)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		map = newMap;

		for (auto& transform : map) {
			UpdateScale(transform.first, transform.second);
		}
	}

	void Adjustment::CopyMap(TransformMap* newMap, NodeSet* set)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		
		map.clear();

		for (auto& kvp : *newMap) {
			if (set->count(kvp.first)) {
				map.emplace(kvp);
				UpdateScale(kvp.first, kvp.second);
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

		for (auto& transform : map) {
			UpdateScale(transform.first, transform.second);
		}

		updated = true;
	}

	void Adjustment::UpdateScale(const NodeKey& name, SamTransform& transform)
	{
		scaled[name] = SlerpNiTransform(transform, scale);
	}

	SamTransform* Adjustment::GetScaledTransform(const NodeKey& key)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		return GetFromTransformMap(scaled, key);
	}

	SamTransform Adjustment::GetScaledTransformOrDefault(const NodeKey& key)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		auto found = scaled.find(key);
		return found != scaled.end() ? found->second : SamTransform();
	}

	//Should it appear in the pose adjust menu or be a part of the node update
	bool Adjustment::IsVisible()
	{
		return type != kAdjustmentTypeRemovedFile;
	}

	void Adjustment::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		
		map.clear();
		scaled.clear();
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

	UInt64 ActorAdjustments::GetRaceGender()
	{
		if (!race)
			return 0;

		UInt64 key = race;
		if (isFemale)
			key |= 0x100000000;

		return key;
	}

	void ActorAdjustments::Clear()
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		map.clear();
		list.clear();
	}

	void ActorAdjustments::GetOffsetTransform(BSFixedString name, SamTransform* offsetTransform)
	{
		for (auto& adjustment : list)
		{
			if (adjustment->IsVisible()) {
				SamTransform* offset = adjustment->GetScaledTransform(NodeKey(name, true));
				if (offset) {
					*offsetTransform *= *offset;
				}
			}
		}
	}

	void ActorAdjustments::GetPoseTransforms(BSFixedString name, SamTransform* offsetTransform, SamTransform* poseTransform)
	{
		GetPoseTransforms(name, offsetTransform, poseTransform, list);
	}

	void ActorAdjustments::GetPoseTransforms(BSFixedString name, SamTransform* offsetTransform, SamTransform* poseTransform, std::vector<std::shared_ptr<Adjustment>>& adjustmentList)
	{
		for (auto& adjustment : adjustmentList) {
			if (adjustment->IsVisible()) {
				SamTransform* offset = adjustment->GetScaledTransform(NodeKey(name, true));
				if (offset) {
					*offsetTransform *= *offset;
				}

				SamTransform* pose = adjustment->GetScaledTransform(NodeKey(name, false));
				if (pose) {
					*poseTransform = *pose;
				}
			}
		}
	}

	void ActorAdjustments::UpdateNode(BSFixedString name)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		NiAVObject* offsetNode = GetFromNodeMap(offsetMap, name);
		if (!offsetNode)
			return;

		//if base node is found, this is a pose/offset node
		SamTransform* baseNode = GetFromTransformMap(*baseMap, NodeKey(name, false));
		if (baseNode) {

			SamTransform offsets = SamTransform();
			SamTransform poses = *baseNode;

			GetPoseTransforms(name, &offsets, &poses);

			SamTransform result = offsets * poses;
			result *= baseNode->Invert();

			offsetNode->m_localTransform = result;
		}

		//If the base node isn't found, this is an offset only node
		else {
			SamTransform offsets = SamTransform();
			GetOffsetTransform(name, &offsets);
			offsetNode->m_localTransform = offsets;
		}
	}

	void ActorAdjustments::UpdateAllAdjustments()
	{
		if (!baseMap || !nodeSets) return;

		for (auto& name : nodeSets->baseStrings) {
			UpdateNode(name);
		}
	}

	void ActorAdjustments::UpdateAllAdjustments(std::shared_ptr<Adjustment> adjustment)
	{
		auto map = adjustment->GetMap();
		for (auto& kvp : *map) {
			//TODO: this performs the update twice if both an offset/pose adjustment exist on the same node, should fix
			UpdateNode(kvp.first.name);
		}
	}

	void ActorAdjustments::UpdatePersistentAdjustments(AdjustmentUpdateData& data) {
		if (data.persistents) {
			for (auto& persistent : *data.persistents) {
				switch (persistent.type) {
				case kAdjustmentTypeDefault:
				{
					std::shared_ptr<Adjustment> adjustment = CreateAdjustment(persistent.name);
					adjustment->file = persistent.file;
					adjustment->mod = persistent.mod;
					adjustment->scale = persistent.scale;
					adjustment->map = persistent.map;
					adjustment->updated = persistent.updated;
					
					//if version < 2 treat as 0
					UpdateAdjustmentVersion(&adjustment->map, persistent.updateType);
					break;
				}
				case kAdjustmentTypeSkeleton:
				case kAdjustmentTypeRace:
				case kAdjustmentTypePose:
				case kAdjustmentTypeAutoRace:
				case kAdjustmentTypeAutoSkeleton:
				{
					std::shared_ptr<Adjustment> adjustment;
					if (persistent.updated) {
						adjustment = CreateAdjustment(persistent.name);
						adjustment->map = persistent.map;

						UpdateAdjustmentVersion(&adjustment->map, persistent.updateType);
					}
					else {
						adjustment = LoadAdjustment(persistent.file, true);
					}
					if (adjustment != nullptr) {
						adjustment->name = persistent.name;
						adjustment->file = persistent.file;
						adjustment->mod = persistent.mod;
						adjustment->scale = persistent.scale;
						adjustment->type = persistent.type;
						adjustment->updated = persistent.updated;
					}
					break;
				}
				case kAdjustmentTypeRemovedFile:
				{
					std::shared_ptr<Adjustment> adjustment = CreateAdjustment(persistent.file);
					adjustment->file = persistent.file;
					adjustment->name = persistent.file;
					adjustment->mod = persistent.mod;
					adjustment->type = kAdjustmentTypeRemovedFile;
					break;
				}
				case kAdjustmentTypeTongue:
				{
					std::shared_ptr<Adjustment> adjustment = CreateAdjustment(persistent.name);
					adjustment->file = persistent.file;
					adjustment->mod = persistent.mod;
					adjustment->scale = persistent.scale;
					adjustment->map = persistent.map;
					adjustment->updated = persistent.updated;
					adjustment->type = persistent.type;

					UpdateAdjustmentVersion(&adjustment->map, persistent.updateType);
					break;
				}
				}
			}
		}
		
		if (data.race) {
			for (auto& raceAdjustment : *data.race) {
				//Find the filename if exists and add if necessary, this should prevent removed race adjustments from being loaded
				std::shared_ptr<Adjustment> adjustment = GetFile(raceAdjustment);
				if (!adjustment) {
					adjustment = LoadAdjustment(raceAdjustment, true);
					if (adjustment != nullptr) {
						adjustment->type = kAdjustmentTypeRace;
					}
				}
			}
		}

		if (data.autoRace) {
			for (auto& raceAdjustment : *data.autoRace) {
				//Find the filename if exists and add if necessary, this should prevent removed race adjustments from being loaded
				std::shared_ptr<Adjustment> adjustment = GetFile(raceAdjustment);
				if (!adjustment) {
					adjustment = LoadAdjustment(raceAdjustment, true);
					if (adjustment != nullptr) {
						adjustment->type = kAdjustmentTypeAutoRace;
					}
				}
			}
		}

		if (data.autoActor) {
			for (auto& actorAdjustment : *data.autoActor) {
				//Find the filename if exists and add if necessary, this should prevent removed race adjustments from being loaded
				std::shared_ptr<Adjustment> adjustment = GetFile(actorAdjustment);
				if (!adjustment) {
					adjustment = LoadAdjustment(actorAdjustment, true);
					if (adjustment != nullptr) {
						adjustment->type = kAdjustmentTypeAutoSkeleton;
					}
				}
			}
		}

		auto it = list.begin();
		while (it != list.end()) {
			//If there is a removed file and it doesn't exist in any of the update data, remove it
			if ((*it)->type == kAdjustmentTypeRemovedFile && !(
				(data.race && data.race->count((*it)->file)) ||
				(data.autoActor && data.autoActor->count((*it)->file)) ||
				(data.autoRace && data.autoRace->count((*it)->file))
				)) {
				map.erase((*it)->handle);
				it = list.erase(it);
			}
			else {
				++it;
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
		adjustment->type = kAdjustmentTypeDefault;
		adjustment->updated = false;
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

		if (!handle)
			return nullptr;
		
		auto found = map.find(handle);
		return (found != map.end() ? found->second : nullptr);
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
			return;
		}

		//If removing a race adjustment, set to removed race type instead 
		if (ShouldRemoveFile(*it)) {
			(*it)->type = kAdjustmentTypeRemovedFile;
			(*it)->updated = false;
		}
		else {
			list.erase(it);
			map.erase(handle);
		}
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

		//If removing a race adjustment, set to removed race type instead 
		if (ShouldRemoveFile(*it))
		{
			(*it)->type = kAdjustmentTypeRemovedFile;
			(*it)->updated = false;
		}
		else {
			list.erase(it);
			map.erase(handle);
		}
	}

	bool ActorAdjustments::HasAdjustment(std::string name)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& it : list) {
			if (it->name == name) return true;
		}

		return false;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::GetFile(std::string filename)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& it : list) {
			if (!_stricmp(it->file.c_str(), filename.c_str())) return it;
		}

		return nullptr;
	}

	void ActorAdjustments::RemoveFile(std::string filename, UInt32 keepHandle)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		auto it = list.begin();
		while (it != list.end()) {
			//if filenames match but adjustment does not have the keep handle
			if (!_stricmp((*it)->file.c_str(), filename.c_str()) && (*it)->handle != keepHandle) {
				//if no keep handle and should remove race
				if (!keepHandle && ShouldRemoveFile(*it)) {
					(*it)->type = kAdjustmentTypeRemovedFile;
					(*it)->updated = false;
					++it;
				}
				else {
					map.erase((*it)->handle);
					it = list.erase(it);
				}
			}
			else {
				++it;
			}
		}
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

	UInt32 ActorAdjustments::MoveAdjustment(UInt32 fromIndex, UInt32 toIndex)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		if (fromIndex == toIndex || fromIndex < 0 || toIndex < 0 || fromIndex >= list.size() || toIndex >= list.size()) return 0;

		std::shared_ptr<Adjustment> adjustment = list[fromIndex];

		if (fromIndex < toIndex) {

			//search for first visible
			while (!list[toIndex]->IsVisible()) {
				++toIndex;
				//No visible above
				if (toIndex >= list.size()) { 
					return 0;
				}
			}

			//shift downward
			for (int i = fromIndex; i < toIndex; ++i) {
				list[i] = list[i + 1];
			}
		}
		else {

			//search for first visible
			while (!list[toIndex]->IsVisible()) {
				--toIndex;
				//No visible below
				if (toIndex < 0) { 
					return 0;
				}
			}

			//shift upward
			for (int i = fromIndex; i > toIndex; --i) {
				list[i] = list[i - 1];
			}
		}

		list[toIndex] = adjustment;

		return 1;
	}

	void ActorAdjustments::UpdateAdjustmentVersion(TransformMap* map, UInt32 updateType)
	{
		switch (updateType) {
		case kAdjustmentUpdateSerialization:
		case kAdjustmentUpdateFile:
		{
			for (auto& transform : *map) {
				if (!transform.first.offset) {
					auto baseNode = GetFromTransformMap(*baseMap, transform.first);
					if (baseNode) {
						SamTransform dst = *baseNode * transform.second;
						transform.second = *baseNode / dst;
					}
				}
			}
			break;
		}
		}
	}

	std::shared_ptr<Adjustment> ActorAdjustments::GetFileOrCreate(std::string filename)
	{
		//If an adjustment with the same file name already exists, load over that instead
		std::shared_ptr<Adjustment> adjustment = GetFile(filename);

		if (!adjustment) {
			adjustment = CreateAdjustment(filename);
		}

		return adjustment;
	}

	bool ActorAdjustments::LoadUpdatedAdjustment(std::string filename, TransformMap* map)
	{
		LoadedAdjustment loadedAdjustment(map);

		if (LoadAdjustmentFile(filename, &loadedAdjustment)) {

			UpdateAdjustmentVersion(loadedAdjustment.map, loadedAdjustment.updateType);

			return true;
		}

		return false;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::LoadAdjustment(std::string filename, bool cached)
	{
		if (cached) {
			//get from cache
			auto adjustmentFile = g_adjustmentManager.GetAdjustmentFile(filename);
			if (adjustmentFile) {

				std::shared_ptr<Adjustment> adjustment = GetFileOrCreate(filename);

				if (nodeSets) {
					adjustment->CopyMap(adjustmentFile, &nodeSets->all);
				}
				
				adjustment->name = filename;
				adjustment->file = filename;
				adjustment->updated = false;
				adjustment->scale = 1.0f;
				adjustment->type = kAdjustmentTypeSkeleton;
				return adjustment;
			}
		}

		TransformMap map;

		if (LoadUpdatedAdjustment(filename, &map)) {

			//add to cache
			if (cached) {
				g_adjustmentManager.SetAdjustmentFile(filename, map);
			}

			std::shared_ptr<Adjustment> adjustment = GetFileOrCreate(filename);

			if (nodeSets) {
				adjustment->CopyMap(&map, &nodeSets->all);
			}
			
			adjustment->name = filename;
			adjustment->file = filename;
			adjustment->updated = false;
			adjustment->scale = 1.0f;
			adjustment->type = kAdjustmentTypeSkeleton;
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
		if (filename.empty())
			return;

		std::shared_ptr<Adjustment> adjustment = GetAdjustment(handle);
		if (!adjustment) 
			return;

		if (SaveAdjustmentFile(filename, adjustment)) {

			//Need to delete all other adjustments with the same file name to enforce name uniqueness
			RemoveFile(filename, handle);

			adjustment->name = filename;
			adjustment->file = filename;
			adjustment->updated = false;

			//Only update if not a tongue adjustment
			if (adjustment->type != kAdjustmentTypeTongue)
				adjustment->type = kAdjustmentTypeSkeleton;
		}
	}

	void ActorAdjustments::ForEachAdjustment(const std::function<void(std::shared_ptr<Adjustment>)>& functor)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : list) {
			if (adjustment->IsVisible()) {
				functor(adjustment);
			}
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

	bool ActorAdjustments::HasNode(BSFixedString name)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		if (offsetMap.count(name)) return true;
		if (poseMap.count(name)) return true;

		return false;
	}

	//Checks the base map for node, if not found it's an offset only node
	bool ActorAdjustments::IsNodeOffset(NodeKey& nodeKey)
	{
		std::shared_lock<std::shared_mutex> lock(mutex);

		if (!baseMap)
			return true;

		auto found = baseMap->find(nodeKey);
		return found == baseMap->end();
	}

	//Sets the override node such that it negates the base node back to the a-pose position
	void ActorAdjustments::NegateTransform(std::shared_ptr<Adjustment> adjustment, NodeKey& key)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);
		
		SamTransform result;

		//if offset set to identity
		if (key.offset) {
			result = SamTransform();
		}
		//else calculate the difference we currently are from base
		else {
			NiAVObject* poseNode = GetFromNodeMap(poseMap, key.name);
			if (!poseNode)
				return;

			SamTransform* baseNode = GetFromTransformMap(*baseMap, key);
			if (!baseNode)
				return;

			result = SamTransform(poseNode->m_localTransform) / *baseNode;
		}

		adjustment->SetTransform(key, result);
		adjustment->updated = true;
	}

	//Sets the override node such that it negates the base node to the specified transform position
	void ActorAdjustments::OverrideTransform(std::shared_ptr<Adjustment> adjustment, const NodeKey& key, SamTransform& transform)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		SamTransform result;

		//if offset just set as is
		if (key.offset) {
			result = transform;
		}
		//if override get the difference
		else {
			NiAVObject* poseNode = GetFromNodeMap(poseMap, key.name);
			if (!poseNode)
				return;

			SamTransform* baseNode = GetFromTransformMap(*baseMap, key);
			if (!baseNode)
				return;

			result = transform * SamTransform(poseNode->m_localTransform).Invert();
			result *= *baseNode;
			result = *baseNode / result;
		}

		adjustment->SetTransform(key, result);
		adjustment->updated = true;
	}

	void ActorAdjustments::RotateTransformXYZ(std::shared_ptr<Adjustment> adjustment, NodeKey& key, UInt32 type, float scalar)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		SamTransform result;

		if (key.offset) {
			result = adjustment->GetTransformOrDefault(key);
			result.rot = MultiplyNiMatrix(result.rot, GetXYZRotation(type, scalar));
		}
		else {
			NiAVObject* poseNode = GetFromNodeMap(poseMap, key.name);
			if (!poseNode)
				return;

			SamTransform* baseNode = GetFromTransformMap(*baseMap, key);
			if (!baseNode)
				return;

			//Get Current position
			result = *baseNode * adjustment->GetTransformOrDefault(key);
			result = result * baseNode->Invert();
			result = result * poseNode->m_localTransform;

			//Apply rotation to current position
			result.rot = MultiplyNiMatrix(result.rot, GetXYZRotation(type, scalar));

			//Trace back the pose/negation
			result = result * SamTransform(poseNode->m_localTransform).Invert();
			result = result * *baseNode;

			//Get the difference
			result = *baseNode / result;
		}

		adjustment->SetTransform(key, result);
		adjustment->updated = true;
	}

	void ActorAdjustments::SavePose(std::string filename, ExportSkeleton* exports)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		//Order matters for applying translations so get them from the list in the correct order
		std::vector<std::shared_ptr<Adjustment>> adjustments;
		for (auto& adjustment : list) {
			if (exports->handles.count(adjustment->handle))
				adjustments.push_back(adjustment);
		}

		//If no nodeset specified, grab everything and assume vanilla skeleton
		if (!exports->nodes) {
			exports->nodes = &nodeSets->all;
		}

		TransformMap transformMap;

		for (auto& node : *exports->nodes) {
			//Include offsets only if they are edited
			if (node.offset) {

				NiTransform offsetTransform = SamTransform();
				bool updated = false;

				for (auto& adjustment : adjustments) {
					if (adjustment->IsVisible()) {
						SamTransform* transform = adjustment->GetTransform(node);
						if (transform) {
							offsetTransform = offsetTransform * *transform;
							updated = true;
						}
					}
				}

				if (updated) {
					transformMap.emplace(node, offsetTransform);
				}
			}

			//Must include all pose nodes
			else {

				NiAVObject* poseNode = GetFromNodeMap(poseMap, node.name);
				if (poseNode) {
					SamTransform* baseNode = GetFromTransformMap(*baseMap, node);
					if (baseNode) {

						SamTransform result = *baseNode;

						for (auto& adjustment : adjustments) {
							if (adjustment->IsVisible()) {
								SamTransform* transform = adjustment->GetTransform(node);
								if (transform) {
									result = result * *transform;
								}
							}
						}

						result *= *baseNode;
						result *= poseNode->m_localTransform;
						transformMap.emplace(node, result);
					}
				}
			}
		}

		SavePoseFile(filename, &transformMap, exports->skeleton);
	}

	void ActorAdjustments::SaveOutfitStudioPose(std::string filename, ExportSkeleton* exports)
	{
		std::lock_guard<std::shared_mutex> lock(mutex);

		//Order matters for applying translations so get them from the list in the correct order
		std::vector<std::shared_ptr<Adjustment>> adjustments;
		for (auto& adjustment : list) {
			if (exports->handles.count(adjustment->handle))
				adjustments.push_back(adjustment);
		}

		TransformMap transformMap;

		for (auto& name : nodeSets->baseStrings) {

			SamTransform* baseNode = GetFromTransformMap(*baseMap, NodeKey(name, false));

			//if the base node isn't found it's an offset only node
			if (baseNode) {
				NiAVObject* poseNode = GetFromNodeMap(poseMap, name);
				if (poseNode) {
					SamTransform offsets;
					SamTransform poses = *baseNode;

					GetPoseTransforms(name, &offsets, &poses, adjustments);

					SamTransform result = offsets * poses; //get offset + pose
					result *= baseNode->Invert(); //negate base node
					result *= poseNode->m_localTransform; //apply base node
					result = *baseNode / result; 
					
					//insert if non zero
					if (!result.IsDefault()) {
						transformMap.emplace(NodeKey(name, false), result);
					}
				}
			}

			//There's no pose node to override so just output as is
			else {
				SamTransform offset = SamTransform();
				GetOffsetTransform(name, &offset);
				if (!offset.IsDefault()) {
					transformMap.emplace(NodeKey(name, true), offset);
				}
			}
		}

		SaveOutfitStudioXml(filename, &transformMap);
	}

	UInt32 ActorAdjustments::LoadPose(std::string path) {
		std::lock_guard<std::shared_mutex> lock(mutex);

		TransformMap transformMap;
		
		if (LoadPosePath(path, &transformMap)) {

			std::shared_ptr<Adjustment> adjustment;

			std::string filename = getFilename(path);

			//clear default types
			for (auto& kvp : map) {
				if (kvp.second->type == kAdjustmentTypeDefault) {
					kvp.second->Clear();
				}
			}

			adjustment = GetAdjustmentByType(kAdjustmentTypePose);

			if (adjustment != nullptr) {
				adjustment->Clear();
				adjustment->name = filename;
				adjustment->scale = 1.0f;
			}
			else {
				adjustment = std::make_shared<Adjustment>(nextHandle, filename);
				map[nextHandle] = adjustment;
				list.push_back(adjustment);
				nextHandle++;
				
				adjustment->type = kAdjustmentTypePose;
			}

			for (auto& kvp : transformMap) {
				if (kvp.first.offset) {
					//Just apply offset as is
					adjustment->SetTransform(kvp.first, kvp.second);
				}
				else {
					//Negate the pose node into the correct position
					NiAVObject* poseNode = GetFromNodeMap(poseMap, kvp.first.name);
					if (poseNode) {
						SamTransform* baseNode = GetFromTransformMap(*baseMap, kvp.first);
						if (baseNode) {

							//base -> adjust -> ibase -> pose = transform
							//base -> adjust = transform -> ipose -> base
							//adjust = (transform -> ipose -> base) / base

							SamTransform dst = kvp.second * SamTransform(poseNode->m_localTransform).Invert();
							dst *= *baseNode;
							dst = *baseNode / dst;

							adjustment->SetTransform(kvp.first, dst);
						}
					}
				}
			}
			
			return 1;
		}

		return 0;
	}

	void ActorAdjustments::ResetPose() {
		UInt32 poseHandle = GetHandleByType(kAdjustmentTypePose);
		if (!poseHandle)
			return;

		RemoveAdjustment(poseHandle);
	}

	void ActorAdjustments::LoadRaceAdjustment(std::string filename, bool clear, bool enable) {
		if (clear) {
			RemoveAdjustmentsByType(kAdjustmentTypeRace, false);
			RemoveAdjustmentsByType(kAdjustmentTypeAutoRace, false);
			//RemoveAdjustmentsByType(kAdjustmentTypeRemovedFile, false); //TODO Can't do this, need to remove specifically removed races
		}
			
		if (filename.empty())
			return;

		if (enable) {
			std::shared_ptr<Adjustment> adjustment = LoadAdjustment(filename, true);
			if (!adjustment) 
				return;

			adjustment->type = kAdjustmentTypeRace;
		} 
		else {
			RemoveFile(filename, 0);
		}
	}

	void ActorAdjustments::RemoveAdjustmentsByType(UInt32 type, bool checkRace) {
		std::lock_guard<std::shared_mutex> lock(mutex);

		auto it = list.begin();

		while (it != list.end()) 
		{
			if ((*it)->type == type) 
			{
				//If removing a non removed race adjustment, check if it should resolve to a removed race adjustment
				if (checkRace && ShouldRemoveFile(*it)) 
				{
					(*it)->type = kAdjustmentTypeRemovedFile;
					(*it)->updated = false;
					++it;
				}
				else 
				{
					map.erase((*it)->handle);
					it = list.erase(it);
				}
			}
			else 
			{
				++it;
			}
		}
	}

	UInt32 ActorAdjustments::GetHandleByType(UInt32 type) {
		for (auto& adjustment : list) {
			if (adjustment->type == type) {
				return adjustment->handle;
			}
		}

		return 0;
	}

	std::shared_ptr<Adjustment> ActorAdjustments::GetAdjustmentByType(UInt32 type) {
		for (auto& adjustment : list) {
			if (adjustment->type == type) {
				return adjustment;
			}
		}

		return nullptr;
	}

	bool ActorAdjustments::ShouldRemoveFile(std::shared_ptr<Adjustment> adjustment) {
		if (adjustment->file.empty())
			return false;

		return g_adjustmentManager.HasFile(race, isFemale, npc->formID, adjustment->file);
	}

	std::shared_ptr<ActorAdjustments> AdjustmentManager::GetActorAdjustments(UInt32 formId) {
		std::shared_lock<std::shared_mutex> lock(actorMutex);

		if (actorAdjustmentCache.count(formId)) {

			std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];
			
			auto valid = adjustments->IsValid();

			if (valid == kActorValid) {
				return adjustments;
			} 
			else if (valid == kActorUpdated) {
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
			key |= 0x100000000;

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

	InsensitiveStringSet* AdjustmentManager::GetRaceAdjustments(UInt32 race, bool isFemale)
	{
		UInt64 key = race;
		if (isFemale)
			key |= 0x100000000;

		if (raceAdjustments.count(key))
			return &raceAdjustments[key];

		return nullptr;
	}

	InsensitiveStringSet* AdjustmentManager::GetAutoRaceAdjustments(UInt32 race, bool isFemale)
	{
		UInt64 key = race;
		if (isFemale)
			key |= 0x100000000;

		if (autoRaceCache.count(key))
			return &autoRaceCache[key];
		if (isFemale && autoRaceCache.count(race))
			return &autoRaceCache[race];

		return nullptr;
	}

	InsensitiveStringSet* AdjustmentManager::GetAutoActorAdjustments(UInt32 formId)
	{
		if (autoActorCache.count(formId))
			return &autoActorCache[formId];

		return nullptr;
	}

	bool AdjustmentManager::HasFile(UInt32 race, bool isFemale, UInt32 formId, std::string filename)
	{
		UInt64 key = race;
		if (isFemale)
			key |= 0x100000000;

		auto raceIt = raceAdjustments.find(key);
		if (raceIt != raceAdjustments.end())
		{
			if (raceIt->second.count(filename))
				return true;
		}

		raceIt = autoRaceCache.find(key);
		if (raceIt != autoRaceCache.end())
		{
			if (raceIt->second.count(filename))
				return true;
		}

		auto actorIt = autoActorCache.find(formId);
		if (actorIt != autoActorCache.end())
		{
			if (actorIt->second.count(filename))
				return true;
		}

		return false;
	}

	std::vector<PersistentAdjustment>* AdjustmentManager::GetPersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments)
	{
		auto raceGender = persistentAdjustments.find(adjustments->formId);
		if (raceGender == persistentAdjustments.end())
			return nullptr;

		//For compatibility with versions prior to 1.0.3, need to check 0 and if it exists move it to the current race/gender
		auto persistents = raceGender->second.find(0);
		if (persistents != raceGender->second.end()) {
			UInt64 key = adjustments->GetRaceGender();
			auto it = raceGender->second.emplace(key, persistents->second);
			raceGender->second.erase(persistents);
			return &it.first->second;
		}

		persistents = raceGender->second.find(adjustments->GetRaceGender());
		if (persistents != raceGender->second.end())
			return &persistents->second;

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

		auto found = actorAdjustmentCache.find(actor->formID);
		if (found != actorAdjustmentCache.end())
			adjustments = found->second;
		
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
				_Logi("base form not found for actor id", actor->formID);
				return;
			}

			adjustments = std::make_shared<ActorAdjustments>(actor, npc);
		}

		if (!CopyActorCache(adjustments)) {
			UpdateActorCache(adjustments);
		}
	}

	bool AdjustmentManager::UpdateActorCache(std::shared_ptr<ActorAdjustments> adjustments) {
		if (UpdateActor(adjustments)) {
			actorAdjustmentCache.emplace(adjustments->formId, adjustments);
		}
		else {
			actorAdjustmentCache.erase(adjustments->formId);
			return false;
		}
		
		return true;
	}

	bool AdjustmentManager::CopyActorCache(std::shared_ptr<ActorAdjustments> adjustments) 
	{
		//If the actor is the AAF Doppelganger, copy the adjustments from the player
		if (adjustments->npc->formID == g_settings.fullDoppelgangerID) {

			auto it = actorAdjustmentCache.find(0x14); //player ref
			if (it != actorAdjustmentCache.end()) {

				return CopyActor(it->second, adjustments);
			}
		}

		return false;
	}

	bool IsRootValid(NiNode* root, NodeSets* set) {
		if (!root)
			return false;

		NiAVObject* rootNode = root->GetObjectByName(&set->rootName);

		if (!rootNode)
			return false;

		static BSFixedString safVersion("SAF_Version");

		NiIntegerExtraData* extraData = (NiIntegerExtraData*)rootNode->GetExtraData(safVersion);
		if (!extraData)
			return false;

		//There has been no breaking changes yet so just accept any version
		return extraData->value;
	}

	bool AdjustmentManager::CopyActor(std::shared_ptr<ActorAdjustments> srcActor, std::shared_ptr<ActorAdjustments> dstActor)
	{
		if (!UpdateActorNodes(dstActor))
			return false;

		for (auto& src : srcActor->list) {
			auto dst = dstActor->CreateAdjustment(std::string(src->name));
			dst->file = std::string(src->file);
			dst->mod = std::string(src->mod);
			dst->type = src->type;
			dst->scale = src->scale;
			dst->updated = src->updated;
			dst->CopyMap(src->GetMap(), &dstActor->nodeSets->all);
		}

		dstActor->UpdateAllAdjustments();

		return true;
	}

	bool AdjustmentManager::UpdateActorNodes(std::shared_ptr<ActorAdjustments> adjustments) {
		if (adjustments->baseRoot)
			RemoveNodeMap(adjustments->baseRoot);

		auto valid = adjustments->IsValid();

		switch (valid) {
		case kActorInvalid:
		{
			adjustments->Clear();
			return false;
		}
		case kActorValid:
		{
			adjustments->Clear();
			break;
		}
		case kActorUpdated:
		{
			TESForm* actorForm = LookupFormByID(adjustments->formId);
			if (!actorForm)
				return false;

			Actor* actor = adjustments->actor = DYNAMIC_CAST(actorForm, TESForm, Actor);
			if (!actor)
				return false;

			TESNPC* npc = adjustments->npc = DYNAMIC_CAST(adjustments->actor->baseForm, TESForm, TESNPC);
			if (!npc)
				return false;

			bool isFemale = (CALL_MEMBER_FN(adjustments->npc, GetSex)() == 1 ? true : false);

			TESRace* actorRace = adjustments->actor->GetActorRace();
			if (!actorRace)
				return false;

			UInt32 raceId = actorRace->formID;

			//if there was a previous race/gender, the actor is persistent and it is now updated. Persist the previous adjustments
			if (adjustments->race && adjustments->actor->flags & TESForm::kFlag_Persistent &&
				(raceId != adjustments->race || isFemale != adjustments->isFemale)) {
				StorePersistentAdjustments(adjustments);
			}

			adjustments->actor = actor;
			adjustments->npc = npc;
			adjustments->isFemale = isFemale;
			adjustments->race = raceId;

			adjustments->Clear();
			break;
		}
		}

		adjustments->root = adjustments->actor->GetActorRootNode(false);
		if (!adjustments->root)
			return false;

		adjustments->nodeSets = GetNodeSets(adjustments->race, adjustments->isFemale);
		if (!adjustments->nodeSets)
			return false;

		if (!IsRootValid(adjustments->root, adjustments->nodeSets))
			return false;

		adjustments->poseMap.clear();
		adjustments->offsetMap.clear();
		CreateNodeMap(adjustments->root, &adjustments->nodeSets->nodeKeys, &adjustments->nodeSets->allStrings, &adjustments->poseMap, &adjustments->offsetMap);

		adjustments->baseRoot = (*getBaseModelRootNode)(adjustments->npc, adjustments->actor);

		if (!IsRootValid(adjustments->baseRoot, adjustments->nodeSets))
			return false;

		adjustments->baseMap = GetCachedTransformMap(adjustments->baseRoot, &adjustments->nodeSets->nodeKeys, &adjustments->nodeSets->baseNodeStrings);
		if (!adjustments->baseMap)
			return false;

		return true;
	}

	bool AdjustmentManager::UpdateActor(std::shared_ptr<ActorAdjustments> adjustments) {
		if (!UpdateActorNodes(adjustments))
			return false;

		adjustments->CreateAdjustment("New Adjustment");

		AdjustmentUpdateData data;
		data.race = GetRaceAdjustments(adjustments->race, adjustments->isFemale);
		data.persistents = GetPersistentAdjustments(adjustments);
		data.autoRace = GetAutoRaceAdjustments(adjustments->race, adjustments->isFemale);
		data.autoActor = GetAutoActorAdjustments(adjustments->npc->formID);

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

	void AdjustmentManager::CreateNodeMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings, NodeMap* poseMap, NodeMap* offsetMap)
	{
		std::vector<NiAVObject*> boneTrees;
		BSFixedStringSet remainingNodes(*strings);
		UInt64 flattenedBoneTreeVFTableAddr = bsFlattenedBoneTreeVFTable.GetUIntPtr();

		root->Visit([&](NiAVObject* object) {
			//check for flattend bone tree VFtable and add it to be searched for missing nodes after DFS is complete
			if (*(UInt64*)object == flattenedBoneTreeVFTableAddr) {
				boneTrees.push_back(object);
			} 
			else if (object->m_name) {
				auto nodeKey = nodeKeys->find(object->m_name);
				if (nodeKey != nodeKeys->end()) {
					if (nodeKey->second.offset) {
						if (offsetMap) {
							offsetMap->emplace(nodeKey->second.name, object);
							remainingNodes.erase(object->m_name);
							return (remainingNodes.size() <= 0);
						}
					}
					else {
						poseMap->emplace(nodeKey->second.name, object);
						remainingNodes.erase(object->m_name);
						return (remainingNodes.size() <= 0);
					}
				}
			}
			return false;
		});

		for (auto& nodeName : remainingNodes) {
			//Search flattened bone trees for remaining nodes
			NiPointer<NiAVObject> flattenedBone = GetNodeFromFlattenedTrees(boneTrees, nodeName);
			if (flattenedBone) {
				auto nodeKey = nodeKeys->find(nodeName);
				if (nodeKey != nodeKeys->end()) {
					if (nodeKey->second.offset) {
						if (offsetMap) {
							offsetMap->emplace(nodeKey->second.name, flattenedBone);
						}
					}
					else {
						poseMap->emplace(nodeKey->second.name, flattenedBone);
					}
				}
			}
		}

		return;
	}

	TransformMap* AdjustmentManager::GetCachedTransformMap(NiNode* root, NodeKeyMap* nodeKeys, BSFixedStringSet* strings)
	{
		std::lock_guard<std::shared_mutex> lock(nodeMapMutex);

		if (nodeMapCache.count(root)) {
			//update ref count
			nodeMapCache[root].refs++;

			return &nodeMapCache[root].transforms;
		}

		NodeMapRef ref;

		//only get the pose nodes
		CreateNodeMap(root, nodeKeys, strings, &ref.map, nullptr);

		for (auto& key : ref.map) 
		{
			ref.transforms.emplace(NodeKey(key.first, false), key.second->m_localTransform);
		}

		nodeMapCache.emplace(root, ref);

		return &nodeMapCache[root].transforms;
	}

	void AdjustmentManager::RemoveNodeMap(NiNode* root) {
		std::lock_guard<std::shared_mutex> lock(nodeMapMutex);

		if (!root)
			return;

		auto it = nodeMapCache.find(root);
		if (it != nodeMapCache.end()) {
			if (--it->second.refs <= 0)
				nodeMapCache.erase(root);
		}
	}

	TransformMap* AdjustmentManager::GetAdjustmentFile(std::string filename)
	{
		std::shared_lock<std::shared_mutex> lock(fileMutex);

		auto found = adjustmentFileCache.find(filename);
		return found != adjustmentFileCache.end() ? &found->second : nullptr;
	}

	TransformMap* AdjustmentManager::SetAdjustmentFile(std::string filename, TransformMap& map)
	{
		std::lock_guard<std::shared_mutex> lock(fileMutex);

		return &(adjustmentFileCache[filename] = map);
	}

	UInt32 AdjustmentManager::CreateNewAdjustment(UInt32 formId, const char* name, const char* mod) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return 0;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		return adjustments->CreateAdjustment(name, mod);
	}

	void  AdjustmentManager::SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;

		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		adjustments->SaveAdjustment(filename, handle);
	}

	UInt32 AdjustmentManager::LoadAdjustment(UInt32 formId, const char* filename, const char* mod) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return 0;

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

		if (!actorAdjustmentCache.count(message->formId)) 
			return;
		
		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[message->formId];
		if (!adjustments)
			return;

		std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(message->handle);
		if (!adjustment)
			return;

		//make sure node exists
		if (!adjustments->HasNode(message->nodeKey.name))
			return;

		switch (message->type) {
		case kAdjustmentTransformPosition:
			adjustment->SetTransformPos(message->nodeKey, message->a, message->b, message->c);
			break;
		case kAdjustmentTransformRotation:
			adjustment->SetTransformRot(message->nodeKey, message->a, message->b, message->c);
			break;
		case kAdjustmentTransformScale:
			adjustment->SetTransformSca(message->nodeKey, message->a);
			break;
		case kAdjustmentTransformReset:
			adjustment->ResetTransform(message->nodeKey);
			break;
		case kAdjustmentTransformNegate:
			adjustments->NegateTransform(adjustment, message->nodeKey);
			break;
		case kAdjustmentTransformRotate:
			adjustments->RotateTransformXYZ(adjustment, message->nodeKey, message->a, message->b);
			break;
		}
	}

	UInt32 AdjustmentManager::LoadPose(UInt32 formId, const char* filename) {
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return 0;

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

	void AdjustmentManager::LoadRaceAdjustment(UInt32 formId, bool isFemale, const char* filename, bool race, bool clear, bool enable)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		std::string name(filename ? filename : "");

		//target all race/gender
		if (race) { 
			UInt64 key = formId;
			if (isFemale)
				key |= 0x100000000;

			if (clear) {
				raceAdjustments.erase(key);
			}

			TransformMap map;
			LoadedAdjustment loadedAdjustment(&map);

			if (!name.empty()) {
				if (enable) {
					raceAdjustments[key].insert(name);

					if (LoadAdjustmentFile(filename, &loadedAdjustment)) {
						//get the ref to the current map
						loadedAdjustment.map = SetAdjustmentFile(filename, *loadedAdjustment.map);
					}
				}
				else {
					raceAdjustments[key].erase(name);
				}
			}

			//load default adjustments
			ForEachActorAdjustments([&](std::shared_ptr<ActorAdjustments> adjustments) {
				if (adjustments->race == formId && adjustments->isFemale == isFemale)
				{
					//we need a reference actor to update the version so use the first actor found as a reference to update the file
					if (loadedAdjustment.updateType == kAdjustmentUpdateFile) {
						adjustments->UpdateAdjustmentVersion(loadedAdjustment.map, loadedAdjustment.updateType);

						//clear the update type to only update once
						loadedAdjustment.updateType = kAdjustmentUpdateNone;
					}

					adjustments->LoadRaceAdjustment(name, clear, enable);
					adjustments->UpdateAllAdjustments();
				}
			});
		}

		//if single npc target just use the regular adjustment functions
		else { 
			auto found = actorAdjustmentCache.find(formId);
			if (found == actorAdjustmentCache.end())
				return;
			std::shared_ptr<ActorAdjustments> adjustments = found->second;

			if (clear) {
				adjustments->RemoveAdjustmentsByType(kAdjustmentTypeSkeleton, true);
				adjustments->RemoveAdjustmentsByType(kAdjustmentTypeAutoSkeleton, true);
			}
			
			if (filename) {
				if (enable) {
					std::shared_ptr<Adjustment> adjustment = adjustments->LoadAdjustment(filename, false);
					if (adjustment != nullptr) {
						adjustment->type = kAdjustmentTypeSkeleton;
					}
				}
				else {
					adjustments->RemoveFile(filename, 0);
				}
			}
			adjustments->UpdateAllAdjustments();
		}
	}

	UInt32 AdjustmentManager::MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex) 
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);
		
		if (!actorAdjustmentCache.count(formId)) return 0;

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

	void AdjustmentManager::LoadTongueAdjustment(UInt32 formId, TransformMap* transforms)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		if (!actorAdjustmentCache.count(formId)) return;
		std::shared_ptr<ActorAdjustments> adjustments = actorAdjustmentCache[formId];

		UInt32 tongueHandle = adjustments->GetHandleByType(kAdjustmentTypeTongue);

		//clear if no transforms
		if (!transforms) {
			if (tongueHandle) {
				adjustments->RemoveAdjustment(tongueHandle);
				adjustments->UpdateAllAdjustments();
			}
			return;
		}

		//if all transforms are identity then delete the adjustment
		bool updated = false;
		for (auto& transform : *transforms) {
			if (!transform.second.IsDefault())
				updated = true;
		}

		if (updated) {

			std::shared_ptr<Adjustment> adjustment;

			if (tongueHandle) {
				adjustment = adjustments->GetAdjustment(tongueHandle); 
				adjustment->Clear();
				adjustment->name = "Face Morphs Tongue";
				adjustment->scale = 1.0f;
			}
			else {
				adjustment = adjustments->CreateAdjustment("Face Morphs Tongue");
				adjustment->type = kAdjustmentTypeTongue;
			}

			for (auto& transform : *transforms) {
				if (!transform.second.IsDefault()) {
					adjustments->OverrideTransform(adjustment, transform.first, transform.second);
				}
			}
		}
		else {
			adjustments->RemoveAdjustment(tongueHandle);
		}

		adjustments->UpdateAllAdjustments();
	}

	void AdjustmentManager::RemoveMod(BSFixedString modName)
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		ForEachActorAdjustments([&](std::shared_ptr<ActorAdjustments> adjustments) {
			bool removed = adjustments->RemoveMod(modName);
			if (removed) adjustments->UpdateAllAdjustments();
		});
	}

	void AdjustmentManager::RemoveAllAdjustments()
	{
		std::lock_guard<std::shared_mutex> lock(actorMutex);

		raceAdjustments.clear();
		persistentAdjustments.clear();

		ForEachActorAdjustments([&](std::shared_ptr<ActorAdjustments> adjustments) {
			adjustments->Clear();
			adjustments->UpdateAllAdjustments();
		});
	}

	SAFMessaging safMessaging;

	void SAFDispatcher::CreateAdjustment(UInt32 formId, const char* name) {
		SAF::AdjustmentCreateMessage message{ formId, name, modName };
		messaging->Dispatch(handle, kSafAdjustmentCreate, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::SaveAdjustment(UInt32 formId, const char* filename, UInt32 handle)
	{
		SAF::AdjustmentSaveMessage message{ formId, filename, handle };
		messaging->Dispatch(handle, kSafAdjustmentSave, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::LoadAdjustment(UInt32 formId, const char* filename) {
		SAF::AdjustmentCreateMessage message{ formId, filename, modName };
		messaging->Dispatch(handle, kSafAdjustmentLoad, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::RemoveAdjustment(UInt32 formId, UInt32 handle) {
		SAF::AdjustmentMessage message{ formId, handle };
		messaging->Dispatch(handle, kSafAdjustmentErase, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::ResetAdjustment(UInt32 formId, UInt32 handle) {
		SAF::AdjustmentMessage message{ formId, handle };
		messaging->Dispatch(handle, kSafAdjustmentReset, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::TransformAdjustment(UInt32 formId, UInt32 handle, const SAF::NodeKey nodeKey, UInt32 type, float a, float b, float c) {
		SAF::AdjustmentTransformMessage message{ formId, handle, nodeKey, type, a, b, c };
		messaging->Dispatch(handle, kSafAdjustmentTransform, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::CreateActorAdjustments(UInt32 formId) {
		SAF::AdjustmentActorMessage message{ formId };
		messaging->Dispatch(handle, kSafAdjustmentActor, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::NegateAdjustmentGroup(UInt32 formId, UInt32 handle, const char* group) {
		SAF::AdjustmentNegateMessage message{ formId, handle, group };
		messaging->Dispatch(handle, kSafAdjustmentNegate, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::LoadPose(UInt32 formId, const char* filename) {
		SAF::PoseMessage message{ formId, filename };
		messaging->Dispatch(handle, kSafPoseLoad, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::ResetPose(UInt32 formId) {
		SAF::PoseMessage message{ formId, nullptr };
		messaging->Dispatch(handle, kSafPoseReset, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::LoadDefaultAdjustment(UInt32 raceId, bool isFemale, const char* filename, bool npc, bool clear, bool enable) {
		SAF::SkeletonMessage message{ raceId, isFemale, filename, npc, clear, enable };
		messaging->Dispatch(handle, kSafDefaultAdjustmentLoad, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::MoveAdjustment(UInt32 formId, UInt32 fromIndex, UInt32 toIndex) {
		SAF::MoveMessage message{ formId, fromIndex, toIndex };
		messaging->Dispatch(handle, kSafAdjustmentMove, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::RenameAdjustment(UInt32 formId, UInt32 handle, const char* name) {
		SAF::AdjustmentSaveMessage message{ formId, name, handle };
		messaging->Dispatch(handle, kSafAdjustmentRename, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFDispatcher::LoadTongueAdjustment(UInt32 formId, SAF::TransformMap* transforms) {
		SAF::TransformMapMessage message{ formId, transforms };
		messaging->Dispatch(handle, kSafAdjustmentTongue, &message, sizeof(uintptr_t), "SAF");
	}

	void SAFMessageHandler(F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case kSafAdjustmentCreate:
		{
			auto data = (AdjustmentCreateMessage*)msg->data;
			UInt32 result = g_adjustmentManager.CreateNewAdjustment(data->formId, data->name, data->esp);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafAdjustmentSave:
		{
			auto data = (AdjustmentSaveMessage*)msg->data;
			g_adjustmentManager.SaveAdjustment(data->formId, data->filename, data->handle);
			break;
		}
		case kSafAdjustmentLoad:
		{
			auto data = (AdjustmentCreateMessage*)msg->data;
			UInt32 result = g_adjustmentManager.LoadAdjustment(data->formId, data->name, data->esp);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafAdjustmentErase:
		{
			auto data = (AdjustmentMessage*)msg->data;
			g_adjustmentManager.RemoveAdjustment(data->formId, data->handle);
			break;
		}
		case kSafAdjustmentReset:
		{
			auto data = (AdjustmentMessage*)msg->data;
			g_adjustmentManager.ResetAdjustment(data->formId, data->handle);
			break;
		}
		case kSafAdjustmentTransform:
		{
			auto data = (AdjustmentTransformMessage*)msg->data;
			g_adjustmentManager.SetTransform(data);
			break;
		}
		case kSafAdjustmentActor:
		{
			auto data = (AdjustmentActorMessage*)msg->data;
			std::shared_ptr<ActorAdjustments> adjustments = g_adjustmentManager.CreateActorAdjustment(data->formId);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafAdjustmentActor, &adjustments, sizeof(uintptr_t), msg->sender);
			break;
		}
		//case SAF::kSafAdjustmentNegate:
		//{
		//	auto data = (SAF::AdjustmentNegateMessage*)msg->data;
		//	SAF::g_adjustmentManager.NegateAdjustments(data->formId, data->handle, data->group);
		//	break;
		//}
		case kSafPoseLoad:
		{
			auto data = (PoseMessage*)msg->data;
			UInt32 result = g_adjustmentManager.LoadPose(data->formId, data->filename);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafPoseReset:
		{
			auto data = (PoseMessage*)msg->data;
			g_adjustmentManager.ResetPose(data->formId);
			break;
		}
		case kSafDefaultAdjustmentLoad:
		{
			auto data = (SkeletonMessage*)msg->data;
			g_adjustmentManager.LoadRaceAdjustment(data->raceId, data->isFemale, data->filename, data->npc, data->clear, data->enable);
			break;
		}
		case kSafAdjustmentMove:
		{
			auto data = (MoveMessage*)msg->data;
			UInt32 result = g_adjustmentManager.MoveAdjustment(data->formId, data->from, data->to);
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafResult, &result, sizeof(uintptr_t), msg->sender);
			break;
		}
		case kSafAdjustmentRename:
		{
			auto data = (AdjustmentSaveMessage*)msg->data;
			g_adjustmentManager.RenameAdjustment(data->formId, data->handle, data->filename);
			break;
		}
		case kSafAdjustmentTongue:
		{
			auto data = (TransformMapMessage*)msg->data;
			g_adjustmentManager.LoadTongueAdjustment(data->formId, data->transforms);
			break;
		}
		}
	}

	class SAFEventReciever :
		public BSTEventSink<TESObjectLoadedEvent>,
		public BSTEventSink<TESLoadGameEvent>,
		public BSTEventSink<TESInitScriptEvent>
	{
	public:
		EventResult	ReceiveEvent(TESObjectLoadedEvent* evn, void* dispatcher)
		{
			if (!g_adjustmentManager.gameLoaded) return kEvent_Continue;

			TESForm* form = LookupFormByID(evn->formId);
			Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
			if (!actor)
				return kEvent_Continue;

			g_adjustmentManager.ActorLoaded(actor, evn->loaded);

			return kEvent_Continue;
		}

		EventResult	ReceiveEvent(TESInitScriptEvent* evn, void* dispatcher)
		{
			Actor* actor = DYNAMIC_CAST(evn->reference, TESForm, Actor);
			if (!actor)
				return kEvent_Continue;

			g_adjustmentManager.ActorLoaded(actor, true);

			return kEvent_Continue;
		}

		EventResult	ReceiveEvent(TESLoadGameEvent* evn, void* dispatcher)
		{
			g_adjustmentManager.GameLoaded();

			return kEvent_Continue;
		}
	};

	SAFEventReciever safEventReciever;

	void F4SEMessageHandler(F4SEMessagingInterface::Message* msg)
	{
		switch (msg->type)
		{
		case F4SEMessagingInterface::kMessage_PostLoad:
			if (safMessaging.messaging)
				safMessaging.messaging->RegisterListener(safMessaging.pluginHandle, nullptr, SAFMessageHandler);
			break;
		case F4SEMessagingInterface::kMessage_GameLoaded:
			GetEventDispatcher<TESObjectLoadedEvent>()->AddEventSink(&safEventReciever);
			GetEventDispatcher<TESLoadGameEvent>()->AddEventSink(&safEventReciever);
			//GetEventDispatcher<TESInitScriptEvent>()->AddEventSink(&safEventReciever);
			break;
		case F4SEMessagingInterface::kMessage_GameDataReady:
			g_adjustmentManager.LoadFiles();
			safMessaging.messaging->Dispatch(safMessaging.pluginHandle, kSafAdjustmentManager, &g_adjustmentManager, sizeof(uintptr_t), nullptr);
			break;
		}
	}

	void SAFDispatcher::Recieve(F4SEMessagingInterface::Message* msg) {
		switch (msg->type)
		{
		case kSafAdjustmentManager:
			manager = static_cast<SAF::AdjustmentManager*>(msg->data);
			break;
		case kSafAdjustmentActor:
			actorAdjustments = *(std::shared_ptr<SAF::ActorAdjustments>*)msg->data;
			break;
		case kSafResult:
			result = *(UInt32*)msg->data;
			break;
		}
	}

	std::shared_ptr<ActorAdjustments> SAFDispatcher::GetActorAdjustments(UInt32 formId) {
		std::lock_guard<std::mutex> lock(mutex);

		if (!manager)
			return nullptr;

		std::shared_ptr<ActorAdjustments> adjustments = manager->GetActorAdjustments(formId);

		if (!adjustments) {
			CreateActorAdjustments(formId);
			adjustments = actorAdjustments;
			actorAdjustments = nullptr;
		}

		return adjustments;
	}

	UInt32 SAFDispatcher::GetResult() {
		UInt32 _result = result;
		result = 0;
		return _result;
	}

	void SaveCallback(const F4SESerializationInterface* ifc) {
		g_adjustmentManager.SerializeSave(ifc);
	}

	void LoadCallback(const F4SESerializationInterface* ifc) {
		g_adjustmentManager.SerializeLoad(ifc);
	}

	void RevertCallback(const F4SESerializationInterface* ifc) {
		g_adjustmentManager.SerializeRevert(ifc);
	}
}