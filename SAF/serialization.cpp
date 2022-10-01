#include "serialization.h"

#include "f4se/GameData.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "conversions.h"
#include "util.h"
#include "io.h"

namespace SAF {

	/*
	* ADJU (Adjustments)
	*
	* Version 0 (Beta pre 0.5)
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
	*
	* Version 1 (0.5.0)
	* Actors length
	*	FormId
	*	Adjustments length
	*		Name
	*		File
	*		Mod
	*		Scale
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
	* 
	* Version 2 (0.6.0 Unreleased) //Replaced transform key with node key name/offset
    *
	* Actors length
	*	FormId
	*	Adjustments length
	*		Name
	*		File
	*		Mod
	*		Scale
	*		Type
	*		Transforms length (Only if add type)
	*			NodeKey name
	*           NodeKey offset
	*			x
	*			y
	*			z
	*			yaw
	*			pitch
	*			roll
	*			scale
	* 
	* Version 3 (1.0.0) //Changed types to be stored on the adjustment and not calculated retroactively 
	*                   //Included the updated flag
	* 	Actors length
	*	FormId
	*	Adjustments length
	*		Name
	*		File
	*		Mod
	*		Scale
	*		Type
	*		Updated
	*		Transforms length (Only if add type)
	*			NodeKey name
	*           NodeKey offset
	*			x
	*			y
	*			z
	*			yaw
	*			pitch
	*			roll
	*			scale
	*/

	/*
	* SKEL (Skeleton Adjustments)
	*
	* Version 0 (Pre 0.5.0)
	*
	* Adjustments Length
	*   Key
	*   Name
	*
	* Version 1 (0.5.0)
	*
	* Keys Length
	*   Key
	*   Adjustments Length
	*	  Name
	*/

	void AdjustmentManager::SerializeSave(const F4SESerializationInterface* ifc) {

		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock);

		PersistentMap savedAdjustments;

		//collect currently loaded actors
		ForEachActorAdjustments([&](std::shared_ptr<ActorAdjustments> adjustments) {
			StorePersistentIfValid(savedAdjustments, adjustments);
		});

		//the persistence store is only updated when an actor is unloaded so only add the missing ones
		for (auto& persistent : persistentAdjustments) {
			if (!savedAdjustments.count(persistent.first)) {
				savedAdjustments[persistent.first] = persistent.second;
			}
		}

		ifc->OpenRecord('ADJU', 3); //adjustments

		UInt32 size = savedAdjustments.size();
		WriteData<UInt32>(ifc, &size);
		for (auto& adjustmentKvp : savedAdjustments) {

			WriteData<UInt32>(ifc, &adjustmentKvp.first);
			UInt32 adjustmentsSize = adjustmentKvp.second.size();

			WriteData<UInt32>(ifc, &adjustmentsSize);
			for (auto& adjustment : adjustmentKvp.second) {

				WriteData<std::string>(ifc, &adjustment.name);
				WriteData<std::string>(ifc, &adjustment.file);
				WriteData<std::string>(ifc, &adjustment.mod);
				WriteData<float>(ifc, &adjustment.scale);
				WriteData<UInt8>(ifc, &adjustment.type);
				WriteData<bool>(ifc, &adjustment.updated);

				if (adjustment.StoreMap()) {

					UInt32 mapSize = adjustment.map.size();
					WriteData<UInt32>(ifc, &mapSize);

					for (auto& kvp : adjustment.map) {
						WriteData<BSFixedString>(ifc, &kvp.first.name);
						WriteData<bool>(ifc, &kvp.first.offset);
						WriteData<float>(ifc, &kvp.second.pos.x);
						WriteData<float>(ifc, &kvp.second.pos.y);
						WriteData<float>(ifc, &kvp.second.pos.z);
						float yaw, pitch, roll;
						MatrixToEulerYPR2(kvp.second.rot, yaw, pitch, roll);
						WriteData<float>(ifc, &yaw);
						WriteData<float>(ifc, &pitch);
						WriteData<float>(ifc, &roll);
						WriteData<float>(ifc, &kvp.second.scale);
					}
				}
			}
		}

		ifc->OpenRecord('SKEL', 1); //skeleton adjustments

		size = raceAdjustments.size();
		WriteData<UInt32>(ifc, &size);

		for (auto& keyKvp : raceAdjustments) {
			WriteData<UInt64>(ifc, &keyKvp.first);

			UInt32 adjustmentsSize = keyKvp.second.size();
			WriteData<UInt32>(ifc, &adjustmentsSize);

			for (auto& adjustment : keyKvp.second)
			{
				WriteData<std::string>(ifc, &adjustment);
			}
		}
	}

	void AdjustmentManager::SerializeLoad(const F4SESerializationInterface* ifc) {
		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock);

		UInt32 type, length, version;

		while (ifc->GetNextRecordInfo(&type, &version, &length))
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

						PersistentAdjustment persistent;
						persistent.version = 3;

						switch (version) {
						case 0:
							ReadData<std::string>(ifc, &persistent.name);
							persistent.file = persistent.name;
							ReadData<std::string>(ifc, &persistent.mod);
							persistent.scale = 1.0f;
							ReadData<UInt8>(ifc, &persistent.type);
							persistent.updated = false;
							break;
						case 1:
						case 2:
							ReadData<std::string>(ifc, &persistent.name);
							ReadData<std::string>(ifc, &persistent.file);
							ReadData<std::string>(ifc, &persistent.mod);
							ReadData<float>(ifc, &persistent.scale);
							ReadData<UInt8>(ifc, &persistent.type);
							//if file isn't empty and add type, change type to skeleton and set updated to true
							if (!persistent.file.empty() && persistent.type == kAdjustmentTypeDefault) {
								persistent.type = kAdjustmentTypeSkeleton;
								persistent.updated = true;
							}
							else {
								persistent.updated = false;
							}
							break;
						default: //3+
							ReadData<std::string>(ifc, &persistent.name);
							ReadData<std::string>(ifc, &persistent.file);
							ReadData<std::string>(ifc, &persistent.mod);
							ReadData<float>(ifc, &persistent.scale);
							ReadData<UInt8>(ifc, &persistent.type);
							ReadData<bool>(ifc, &persistent.updated);
						}

						//Accept loaded mods and adjustments that don't have a mod specified
						bool modLoaded = persistent.mod.empty();
						if (!modLoaded) {
							const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(persistent.mod.c_str());
							modLoaded = modInfo && modInfo->modIndex != 0xFF;
						}

						if (persistent.StoreMap()) {

							UInt32 transformSize;
							ReadData<UInt32>(ifc, &transformSize);
							if (transformSize > 0) {

								TransformMap map;
								for (UInt32 k = 0; k < transformSize; ++k) {
									
									std::string keyName;
									BSFixedString nodeName;
									bool nodeOffset;

									switch (version) {
									case 0:
									case 1:
									{
										ReadData<std::string>(ifc, &keyName);
										break;
									}
									default:
									{
										ReadData<BSFixedString>(ifc, &nodeName);
										ReadData<bool>(ifc, &nodeOffset);
										break;
									}
									}

									NiTransform transform;
									ReadData<float>(ifc, &transform.pos.x);
									ReadData<float>(ifc, &transform.pos.y);
									ReadData<float>(ifc, &transform.pos.z);

									float yaw, pitch, roll;
									ReadData<float>(ifc, &yaw);
									ReadData<float>(ifc, &pitch);
									ReadData<float>(ifc, &roll);

									ReadData<float>(ifc, &transform.scale);

									switch (version) {
									case 0:
									case 1:
									{
										MatrixFromEulerYPR(transform.rot, yaw, pitch, roll);
										NodeKey nodeKey = GetNodeKeyFromString(keyName.c_str());
										if (nodeKey.key) {
											map.emplace(nodeKey, transform);
										}
										break;
									}
									default:
									{
										MatrixFromEulerYPR2(transform.rot, yaw, pitch, roll);
										map.emplace(NodeKey(nodeName, nodeOffset), transform);
										break;
									}
									}
								}
								LoadPersistentIfValid(formId, persistent, modLoaded);
							}
						}
						else
						{
							LoadPersistentIfValid(formId, persistent, modLoaded);
						}
					}
					//enforce filename uniqueness
					ValidateFilenames(persistentAdjustments[formId]);
				}
				break;
			}
			case 'SKEL':
			{
				switch (version) {
				case 0:
				{
					UInt32 adjustmentSize;
					ReadData<UInt32>(ifc, &adjustmentSize);

					for (UInt32 i = 0; i < adjustmentSize; ++i) {
						UInt64 adjustmentKey;
						ReadData<UInt64>(ifc, &adjustmentKey);

						std::string adjustmentName;
						ReadData<std::string>(ifc, &adjustmentName);

						raceAdjustments[adjustmentKey].insert(adjustmentName);
					}
					break;
				}
				case 1:
				{
					UInt32 adjustmentSize;
					ReadData<UInt32>(ifc, &adjustmentSize);

					for (UInt32 i = 0; i < adjustmentSize; ++i) {
						UInt64 adjustmentKey;
						ReadData<UInt64>(ifc, &adjustmentKey);

						UInt32 namesSize;
						ReadData<UInt32>(ifc, &namesSize);

						for (UInt32 k = 0; k < namesSize; ++k) {

							std::string adjustmentName;
							ReadData<std::string>(ifc, &adjustmentName);

							raceAdjustments[adjustmentKey].insert(adjustmentName);
						}
					}
					break;
				}
				}
			}
			}
		}
	}

	void AdjustmentManager::LoadPersistentIfValid(UInt32 formId, PersistentAdjustment persistent, bool loaded) {
		//if mod not loaded
		if (!loaded)
			return;

		//if default or tongue type, check if transform is unedited
		if (persistent.type == kAdjustmentTypeDefault || persistent.type == kAdjustmentTypeTongue) {
			if (TransformMapIsDefault(persistent.map))
				return;
		}

		persistentAdjustments[formId].push_back(persistent);
	}

	void AdjustmentManager::SerializeRevert(const F4SESerializationInterface* ifc) {
		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock);

		persistentAdjustments.clear();
		raceAdjustments.clear();
		actorAdjustmentCache.clear();
		nodeMapCache.clear();

		gameLoaded = false;
	}

	void AdjustmentManager::StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments) {
		std::lock_guard<std::shared_mutex> persistenceLock(persistenceMutex);

		persistentAdjustments.erase(adjustments->formId);

		StorePersistentIfValid(persistentAdjustments, adjustments);
	}

	void AdjustmentManager::StorePersistentIfValid(PersistentMap& map, std::shared_ptr<ActorAdjustments> adjustments) {
		std::vector<PersistentAdjustment> persistents;
		adjustments->GetPersistentAdjustments(persistents);

		for (auto& persistent : persistents) {
			if (IsPersistentValid(persistent)) {
				map[adjustments->formId] = persistents;
				return;
			}
		}
	}

	//This is to clear out any npcs with unedited adjustments, must have at least one non race adjustment
	bool AdjustmentManager::IsPersistentValid(PersistentAdjustment& adjustment)
	{
		return (adjustment.type != kAdjustmentTypeNone && adjustment.type != kAdjustmentTypeRace);
	}

	void ActorAdjustments::GetPersistentAdjustments(std::vector<PersistentAdjustment>& persistents) {
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : map) {
			PersistentAdjustment persistentAdjustment(adjustment.second, 3);
			if (persistentAdjustment.type != kAdjustmentTypeNone)
				persistents.push_back(persistentAdjustment);
		}
	}

	//Need to enforce file name uniqueness
	void AdjustmentManager::ValidateFilenames(std::vector<PersistentAdjustment>& persistents) {
		std::unordered_set<std::string> filenames;

		auto it = persistents.begin();
		while (it != persistents.end()) {
			//if already found, erase
			if (filenames.count(it->file)) {
				it = persistents.erase(it);
			}
			//else add to set
			else {
				filenames.insert(it->file);
				++it;
			}
		}
	}
}