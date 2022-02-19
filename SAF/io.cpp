#include "io.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "f4se/GameData.h"
#include "f4se/NiTypes.h"

#include "json/json.h"

#include "adjustments.h"
#include "util.h"

namespace SAF {

	void ReadAll(IFileStream* file, std::string* str)
	{
		char ret = file->Read8();
		while (ret > 0) {
			str->push_back(ret);
			ret = file->Read8();
		}
	}

	bool LoadNodeSetsFile(std::string path)
	{
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_LogCat("Could not open ", path);
			return false;
		}

		std::string nodeMap;
		ReadAll(&file, &nodeMap);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(nodeMap, value))
		{
			_LogCat("Failed to parse ", path);
			return false;
		}

		try {
			std::string mod = value["mod"].asString();
			std::string race = value["race"].asString();

			UInt32 formId = GetFormID(mod, race);
			if (!formId) return false;

			std::string sex = value["sex"].asString();
			bool isFemale = (sex == "female" || sex == "Female");

			Json::Value offsets = value["offsets"];
			Json::Value overrides = value["overrides"];

			NodeSets nodeSet;

			for (auto& node : offsets) {
				std::string offset = node.asString();
				nodeSet.offsets.insert(offset);
				nodeSet.all.insert(offset);
				nodeSet.allOrBase.insert(offset);
			}

			for (auto& node : overrides) {
				std::string base = node.asString();
				std::string overrider = base + g_adjustmentManager.overridePostfix;
				nodeSet.base.insert(base);
				nodeSet.overrides.insert(overrider);
				nodeSet.all.insert(overrider);
				nodeSet.allOrBase.insert(base);
				nodeSet.allOrBase.insert(overrider);
				nodeSet.baseMap[overrider] = base;
			}

			UInt64 key = formId;
			if (isFemale)
				key |= 0x100000000;

			g_adjustmentManager.nodeSets[key] = nodeSet;
		}
		catch (...) {
			_LogCat("Failed to read ", path);
			return false;
		}

		return true;
	}

	bool LoadDefaultAdjustmentFile(std::string path)
	{
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_LogCat("Could not open ", path);
			return false;
		}

		std::string defaultAdjustment;
		ReadAll(&file, &defaultAdjustment);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(defaultAdjustment, value))
		{
			_LogCat("Failed to parse ", path);
			return false;
		}

		std::vector<std::string> adjustmentList;

		try {
			std::string mod = value["mod"].asString();
			std::string race = value["race"].asString();

			UInt32 formId = GetFormID(mod, race);
			if (!formId) return false;

			std::string sex = value["sex"].asString();
			bool isFemale = (sex == "female" || sex == "Female");

			UInt64 key = formId;
			if (isFemale)
				key |= 0x100000000;

			Json::Value adjustments = value["adjustments"];

			for (auto& adjustment : adjustments) {
				adjustmentList.push_back(adjustment.asString());
			}

			g_adjustmentManager.defaultAdjustments[key] =  adjustmentList;
		}
		catch (...) {
			_LogCat("Failed to read ", path);
			return false;
		}

		return true;
	}


	bool LoadUniqueAdjustmentFile(std::string path) {
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_LogCat(path, " not found");
			return false;
		}

		std::string actorString;
		ReadAll(&file, &actorString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(actorString, value)) {
			_LogCat("Failed to parse ", path);
			return false;
		}

		try {
			std::string mod = value["mod"].asString();
			std::string actor = value["actor"].asString();

			UInt32 formId = GetFormID(mod, actor);
			if (!formId) return false;

			Json::Value members = value["adjustments"];
			std::vector<std::pair<std::string, std::string>> adjustments;

			for (auto& member : members) {
				adjustments.push_back(std::make_pair(member.asString(), mod));
			}

			g_adjustmentManager.uniqueAdjustments[formId] = adjustments;
		}
		catch (...) {
			_LogCat("Failed to read ", path);
			return false;
		}

		return true;
	}

	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment) {
		
		Json::StyledWriter writer;
		Json::Value value;

		float heading, attitude, bank;
		adjustment->ForEachTransform([&](std::string name, NiTransform* transform) {
			if (!TransormIsDefault(transform)) {
				Json::Value member;
				member["x"] = Json::Value(transform->pos.x);
				member["y"] = Json::Value(transform->pos.y);
				member["z"] = Json::Value(transform->pos.z);
				//heading, attitude, bank
				//pitch, roll, yaw
				transform->rot.GetEulerAngles(&heading, &attitude, &bank);
				member["pitch"] = Json::Value(heading * RADIAN_TO_DEGREE);
				member["roll"] = Json::Value(attitude * RADIAN_TO_DEGREE);
				member["yaw"] = Json::Value(bank * RADIAN_TO_DEGREE);
				member["scale"] = Json::Value(transform->scale);
				value[name] = member;
			}
		});

		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Adjustments\\");
		path += filename;
		path += ".json";

		IFileStream::MakeAllDirs(path.c_str());
		if (!file.Create(path.c_str())) {
			_LogCat("Failed to create file ", filename);
			return false;
		}

		std::string jsonString = writer.write(value);
		file.WriteString(jsonString.c_str());
		file.Close();

		return true;
	}

	bool LoadAdjustmentFile(std::string filename, TransformMap* map) {
		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Adjustments\\");
		path += filename;
		path += ".json";

		if (!file.Open(path.c_str())) {
			_LogCat(filename, " not found");
			return false;
		}

		std::string adjustmentString;
		ReadAll(&file, &adjustmentString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(adjustmentString, value)) {
			_LogCat("Failed to parse ", filename);
			return false;
		}

		try {
			Json::Value::Members members = value.getMemberNames();

			float heading, attitude, bank;
			for (auto& member : members) {
				NiTransform transform;
				transform.pos.x = value[member]["x"].asFloat();
				transform.pos.y = value[member]["y"].asFloat();
				transform.pos.z = value[member]["z"].asFloat();
				heading = value[member]["pitch"].asFloat() * DEGREE_TO_RADIAN;
				attitude = value[member]["roll"].asFloat() * DEGREE_TO_RADIAN;
				bank = value[member]["yaw"].asFloat() * DEGREE_TO_RADIAN;
				transform.rot.SetEulerAngles(heading, attitude, bank);
				transform.scale = value[member]["scale"].asFloat();
				(*map)[member] = transform;
			}
		}
		catch (...) {
			_LogCat("Failed to read ", filename);
			return false;
		}

		return true;
	}

	bool SavePoseFile(std::string filename, std::shared_ptr<ActorAdjustments> adjustments) {
		//todo
		return true;
	}

	bool LoadPoseFile(std::string filename, std::shared_ptr<ActorAdjustments> adjustments) {
		//todo
		return true;
	}

	void LoadFiles() {
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\NodeMaps", "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadNodeSetsFile(path);
		}
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\DefaultAdjustments", "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadDefaultAdjustmentFile(path);
		}
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\Actors", "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadUniqueAdjustmentFile(path);
		}
	}
}