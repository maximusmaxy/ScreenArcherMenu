#include "serialization.h"

#include "f4se/GameData.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "conversions.h"
#include "util.h"

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
	*/

	/*
	* SKEL (Skeleton Adjustments)
	*
	* Version 0
	*
	* Adjustments Length
	*   Key
	*   Name
	*
	* Version 1 (Beta pre 0.5)
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

		ifc->OpenRecord('ADJU', 1); //adjustments

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

				if (adjustment.type == kAdjustmentSerializeAdd) {

					UInt32 mapSize = adjustment.map.size();
					WriteData<UInt32>(ifc, &mapSize);

					for (auto& kvp : adjustment.map) {
						WriteData<std::string>(ifc, &kvp.first);
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

		//ifc->OpenRecord('SKEL', 0); //skeleton adjustments

		//size = defaultAdjustments.size();
		//WriteData<UInt32>(ifc, &size);

		//for (auto& kvp : defaultAdjustments) {
		//	WriteData<UInt64>(ifc, &kvp.first);
		//	WriteData<std::string>(ifc, &kvp.second);
		//}

		ifc->OpenRecord('SKEL', 1); //skeleton adjustments

		size = defaultAdjustments.size();
		WriteData<UInt32>(ifc, &size);

		for (auto& keyKvp : defaultAdjustments) {
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

						std::string name;
						std::string file;
						std::string mod;
						float scale;

						switch (version) {
						case 0:
							ReadData<std::string>(ifc, &name);
							file = name;
							ReadData<std::string>(ifc, &mod);
							scale = 1.0f;
							break;
						case 1:
							ReadData<std::string>(ifc, &name);
							ReadData<std::string>(ifc, &file);
							ReadData<std::string>(ifc, &mod);
							ReadData<float>(ifc, &scale);
							break;
						}

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
									MatrixFromEulerYPR(transform.rot, yaw, pitch, roll);

									ReadData<float>(ifc, &transform.scale);

									map[key] = transform;
								}
								LoadPersistentIfValid(formId, PersistentAdjustment(name, file, mod, scale, adjustmentType, map), modLoaded);
							}
						}
						else
						{
							LoadPersistentIfValid(formId, PersistentAdjustment(name, file, mod, scale, adjustmentType), modLoaded);
						}
					}
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

						defaultAdjustments[adjustmentKey].insert(adjustmentName);
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

							defaultAdjustments[adjustmentKey].insert(adjustmentName);
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
		if (!loaded)
			return;

		if (persistent.type == kAdjustmentSerializeAdd) {
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
		defaultAdjustments.clear();
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

		//Must have 1 add or 1 load
		for (auto& persistent : persistents) {
			if (IsPersistentValid(persistent)) {
				map[adjustments->formId] = persistents;
				return;
			}
		}
	}

	bool AdjustmentManager::IsPersistentValid(PersistentAdjustment& adjustment)
	{
		return (adjustment.type == kAdjustmentSerializeAdd ||
			adjustment.type == kAdjustmentSerializeLoad);
	}

	void ActorAdjustments::GetPersistentAdjustments(std::vector<PersistentAdjustment>& persistents) {
		std::shared_lock<std::shared_mutex> lock(mutex);

		for (auto& adjustment : map) {
			PersistentAdjustment persistentAdjustment = adjustment.second->GetPersistence();
			if (persistentAdjustment.type != kAdjustmentSerializeDisabled)
				persistents.push_back(persistentAdjustment);
		}

		for (auto& removed : removedAdjustments)
		{
			persistents.push_back(PersistentAdjustment(removed, kAdjustmentSerializeRemove));
		}
	}

	PersistentAdjustment Adjustment::GetPersistence() {
		std::shared_lock<std::shared_mutex> lock(mutex);

		UInt8 type = kAdjustmentSerializeDisabled;

		if (!updated) {
			if (isDefault) {
				type = kAdjustmentSerializeDefault;
			}
			else if (!file.empty()) {
				type = kAdjustmentSerializeLoad;
			}
		}

		if (type == kAdjustmentSerializeDisabled) {
			if (!TransformMapIsDefault(map)) {
				type = kAdjustmentSerializeAdd;
			}
		}

		switch (type) {
		case kAdjustmentSerializeAdd:
			return PersistentAdjustment(name, file, mod, scale, type, map);
		case kAdjustmentSerializeLoad:
		case kAdjustmentSerializeDefault:
			return PersistentAdjustment(name, file, mod, scale, type);
		default:
			return PersistentAdjustment();
		}
	}
}