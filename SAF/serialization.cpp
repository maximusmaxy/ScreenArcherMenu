#include "serialization.h"

#include "f4se/GameData.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "conversions.h"
#include "util.h"
#include "io.h"

#define ADJU_VERSION 4
#define SKEL_VERSION 1

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
	*		Transforms length
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
	* 	Version 4 (1.0.3) //Need to store adjustments seperately for each race/gender per actor
	*                 
	* 	Actors length
	*	FormId
	*   RaceGender length
	*		RaceGender Id
	*		Adjustments length
	*			Name
	*			File
	*			Mod
	*			Scale
	*			Type
	*			Updated
	*			Transforms length
	*				NodeKey name
	*			    NodeKey offset
	*				x
	*				y
	*				z
	*				yaw
	*				pitch
	*				roll
	*				scale
	* 
	* 
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
			//remove from persistents
			auto it = persistentAdjustments.find(adjustments->formId);
			if (it != persistentAdjustments.end()) {
				it->second.erase(adjustments->GetRaceGender());
			}
		});

		//the persistence store is only updated when an actor is unloaded so only add the missing ones
		for (auto& raceGender : persistentAdjustments) {
			for (auto& adjustment : raceGender.second) {
				savedAdjustments[raceGender.first][adjustment.first] = adjustment.second;
			}
		}

		ifc->OpenRecord('ADJU', ADJU_VERSION); //adjustments

		UInt32 size = savedAdjustments.size();
		WriteData<UInt32>(ifc, &size);
		for (auto& actorKvp : savedAdjustments) {

			WriteData<UInt32>(ifc, &actorKvp.first);

			UInt32 raceGenderSize = actorKvp.second.size();
			WriteData<UInt32>(ifc, &raceGenderSize);

			for (auto& raceGenderKvp : actorKvp.second) {

				WriteData<UInt64>(ifc, &raceGenderKvp.first);

				UInt32 adjustmentSize = raceGenderKvp.second.size();
				WriteData<UInt32>(ifc, &adjustmentSize);

				for (auto& adjustment : raceGenderKvp.second) {
					WriteData<std::string>(ifc, &adjustment.name);
					WriteData<std::string>(ifc, &adjustment.file);
					WriteData<std::string>(ifc, &adjustment.mod);
					WriteData<float>(ifc, &adjustment.scale);
					WriteData<UInt8>(ifc, &adjustment.type);
					WriteData<bool>(ifc, &adjustment.updated);

					//V1.0 always store size, check if needed, zero size if not necessary
					UInt32 mapSize;
					if (adjustment.StoreMap()) {
						mapSize = adjustment.map.size();
					}
					else {
						mapSize = 0;
					}

					WriteData<UInt32>(ifc, &mapSize);
					for (auto& kvp : adjustment.map) {
						WriteData<BSFixedString>(ifc, &kvp.first.name);
						WriteData<bool>(ifc, &kvp.first.offset);
						WriteData<float>(ifc, &kvp.second.pos.x);
						WriteData<float>(ifc, &kvp.second.pos.y);
						WriteData<float>(ifc, &kvp.second.pos.z);
						float yaw, pitch, roll;
						MatrixToEulerYPR(kvp.second.rot, yaw, pitch, roll);
						WriteData<float>(ifc, &yaw);
						WriteData<float>(ifc, &pitch);
						WriteData<float>(ifc, &roll);
						WriteData<float>(ifc, &kvp.second.scale);
					}
				}
			}
		}

		ifc->OpenRecord('SKEL', SKEL_VERSION); //skeleton adjustments

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
				if (version <= ADJU_VERSION) {
					std::unordered_map<std::string, bool> loadedMods;

					UInt32 actorSize;
					ReadData<UInt32>(ifc, &actorSize);

					for (UInt32 i = 0; i < actorSize; ++i) {

						UInt32 oldActorId, actorId;
						ReadData<UInt32>(ifc, &oldActorId);
						bool actorResolved = ifc->ResolveFormId(oldActorId, &actorId);

						//in version 4 onward, need to store a raceGender key as well
						UInt32 raceGenderSize = 1;
						if (version >= 4) {
							ReadData<UInt32>(ifc, &raceGenderSize);
						}

						for (UInt32 j = 0; j < raceGenderSize; ++j) {

							//If version is not 4+, Use a key of 0 and it will be handled later
							UInt64 raceGenderId = 0;
							bool raceGenderResolved = true;

							if (version >= 4) {
								UInt64 oldRaceGender;
								ReadData<UInt64>(ifc, &oldRaceGender);

								UInt64 isFemale = oldRaceGender & 0x100000000;
								UInt32 oldRacegenderId = oldRaceGender & 0xFFFFFFFF;
								UInt32 outRaceGenderId;
								raceGenderResolved = ifc->ResolveFormId(oldRacegenderId, &outRaceGenderId);
								raceGenderId = outRaceGenderId | isFemale;
							}

							UInt32 adjustmentSize;
							ReadData<UInt32>(ifc, &adjustmentSize);

							for (UInt32 k = 0; k < adjustmentSize; ++k) {
								PersistentAdjustment persistent;

								//need to update serialization of types prior to version 3
								persistent.updateType = version < 3 ? kAdjustmentUpdateSerialization : kAdjustmentUpdateNone;

								//Pre version 3, only adds map on default type
								bool loadMap = true;

								switch (version) {
								case 0:
									ReadData<std::string>(ifc, &persistent.name);
									ReadData<std::string>(ifc, &persistent.mod);
									persistent.scale = 1.0f;
									ReadData<UInt8>(ifc, &persistent.type);
									persistent.file = persistent.type == kAdjustmentTypeDefault ? "" : persistent.name;
									persistent.updated = false;
									loadMap = persistent.type == kAdjustmentTypeDefault;
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
										loadMap = persistent.type == kAdjustmentTypeDefault;
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
									auto found = loadedMods.find(persistent.mod);
									if (found != loadedMods.end()) {
										modLoaded = found->second;
									}
									else {
										const ModInfo* modInfo = (*g_dataHandler)->LookupModByName(persistent.mod.c_str());
										modLoaded = modInfo && modInfo->modIndex != 0xFF;
										loadedMods.emplace(persistent.mod, modLoaded);
									}
								}

								if (loadMap) {
									UInt32 transformSize;
									ReadData<UInt32>(ifc, &transformSize);
									if (transformSize > 0) {
										for (UInt32 l = 0; l < transformSize; ++l) {

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
												MatrixFromEulerYPRTransposed(transform.rot, yaw, pitch, roll);
												NodeKey nodeKey = GetNodeKeyFromString(keyName.c_str());
												if (nodeKey.key) {
													persistent.map.emplace(nodeKey, transform);
												}
												break;
											}
											default:
											{
												MatrixFromEulerYPR(transform.rot, yaw, pitch, roll);
												persistent.map.emplace(NodeKey(nodeName, nodeOffset), transform);
												break;
											}
											}
										}
									}
								}
								if (actorResolved && raceGenderResolved && modLoaded)
									LoadPersistentIfValid(actorId, raceGenderId, persistent);
							}
							ValidatePersistents(persistentAdjustments, actorId, raceGenderId);
						}
					}
				}
				
				break;
			}
			case 'SKEL':
			{
				if (version <= SKEL_VERSION) {
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

							UInt64 isFemale = adjustmentKey & 0x100000000;
							UInt32 oldId = adjustmentKey & 0xFFFFFFFF;
							UInt32 formId;
							bool resolved = ifc->ResolveFormId(oldId, &formId);
							adjustmentKey = formId | isFemale;

							UInt32 namesSize;
							ReadData<UInt32>(ifc, &namesSize);

							for (UInt32 k = 0; k < namesSize; ++k) {

								std::string adjustmentName;
								ReadData<std::string>(ifc, &adjustmentName);

								if (resolved)
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
	}

	void AdjustmentManager::LoadPersistentIfValid(UInt32 formId, UInt64 raceGenderId, PersistentAdjustment& persistent) {
		//if default or tongue type, check if transform is unedited
		if (!persistent.IsValid())
			return;

		persistentAdjustments[formId][raceGenderId].push_back(persistent);
	}

	void AdjustmentManager::SerializeRevert(const F4SESerializationInterface* ifc) {
		std::unique_lock<std::shared_mutex> persistenceLock(persistenceMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> actorLock(actorMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> fileLock(fileMutex, std::defer_lock);
		std::unique_lock<std::shared_mutex> nodeMapLock(nodeMapMutex, std::defer_lock);

		std::lock(actorLock, persistenceLock, fileLock, nodeMapLock);

		adjustmentFileCache.clear();
		raceAdjustments.clear();
		actorAdjustmentCache.clear();
		persistentAdjustments.clear();
		nodeMapCache.clear();

		gameLoaded = false;
	}

	void AdjustmentManager::StorePersistentAdjustments(std::shared_ptr<ActorAdjustments> adjustments) {
		std::lock_guard<std::shared_mutex> persistenceLock(persistenceMutex);

		auto raceGender = persistentAdjustments.find(adjustments->formId);
		if (raceGender != persistentAdjustments.end()) {
			raceGender->second.erase(adjustments->GetRaceGender());
		}

		StorePersistentIfValid(persistentAdjustments, adjustments);
	}

	bool AdjustmentManager::StorePersistentIfValid(PersistentMap& map, std::shared_ptr<ActorAdjustments> adjustments) {
		std::vector<PersistentAdjustment> persistents;
		adjustments->GetPersistentAdjustments(persistents);

		for (auto& persistent : persistents) {
			if (IsPersistentValid(persistent)) {
				map[adjustments->formId][adjustments->GetRaceGender()] = persistents;
				return true;
			}
		}
		
		return false;
	}

	//This is to clear out any npcs with unedited adjustments, must have a non race adjustment, or an edited race adjustment
	bool AdjustmentManager::IsPersistentValid(PersistentAdjustment& adjustment)
	{
		switch (adjustment.type)
		{
		case kAdjustmentTypeNone: return false;
		case kAdjustmentTypeDefault:
		case kAdjustmentTypeTongue: 
		case kAdjustmentTypePose:
			return !TransformMapIsDefault(adjustment.map);
		case kAdjustmentTypeRace: 
			return adjustment.updated;
		default: return true;
		}
	}

	void ActorAdjustments::GetPersistentAdjustments(std::vector<PersistentAdjustment>& persistents) {
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : map) {
			PersistentAdjustment persistentAdjustment(adjustment.second, kAdjustmentUpdateNone);
			if (persistentAdjustment.IsValid())
				persistents.push_back(persistentAdjustment);
		}
	}

	
	void AdjustmentManager::ValidatePersistents(PersistentMap& map, UInt32 formId, UInt64 raceGenderId) {
		auto raceGender = map.find(formId);
		if (raceGender == map.end())
			return;

		auto persistents = raceGender->second.find(raceGenderId);
		if (persistents == raceGender->second.end())
			return;

		//Need to enforce file name uniqueness
		InsensitiveStringSet filenames;

		auto it = persistents->second.begin();
		while (it != persistents->second.end()) {
			//skip empty
			if (it->file.empty()) {
				++it;
			}
			else {
				//if already found, erase
				if (filenames.count(it->file)) {
					it = persistents->second.erase(it);
				}
				//else add to set
				else {
					filenames.insert(it->file);
					++it;
				}
			}
		}
	}

	bool PersistentAdjustment::StoreMap()
	{
		switch (type) {
		case kAdjustmentTypeNone:
			return false;
		case kAdjustmentTypeDefault:
		case kAdjustmentTypeTongue:
		case kAdjustmentTypePose:
			return true;
		}

		return updated;
	}

	bool PersistentAdjustment::IsValid()
	{
		switch (type) {
		case kAdjustmentTypeNone:
			return false;
		case kAdjustmentTypeDefault:
		case kAdjustmentTypeTongue:
		case kAdjustmentTypePose:
		{
			if (TransformMapIsDefault(map))
				return false;
		}
		}

		return true;
	}
}