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

			bool isFemale = (value["sex"].asString() == "female");

			Json::Value offsets = value["offsets"];
			Json::Value overrides = value["overrides"];

			NodeSets nodeSet;

			for (auto& node : offsets) {
				nodeSet.offsets.insert(node.asString());
				nodeSet.all.insert(node.asString());
			}

			for (auto& node : overrides) {
				std::string base = node.asString();
				std::string overrider = base + "_Override";
				nodeSet.base[overrider] = base;
				nodeSet.overrides.insert(overrider);
				nodeSet.all.insert(overrider);
			}

			UInt64 key = formId;
			if (isFemale)
				formId |= 0x100000000;

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

			bool isFemale = (value["sex"].asString() == "female");

			UInt64 key = formId;
			if (isFemale)
				formId |= 0x100000000;

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
				member["p"] = Json::Value(heading * 180 / MATH_PI);
				member["r"] = Json::Value(attitude * 180 / MATH_PI);
				member["y"] = Json::Value(bank * 180 / MATH_PI);
				member["s"] = Json::Value(transform->scale);
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

	bool LoadAdjustmentFile(std::string filename, NodeSet* set, std::unordered_map<std::string, NiTransform>* map) {
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
				if (set->count(member)) {
					NiTransform transform;
					transform.pos.x = value[member]["x"].asFloat();
					transform.pos.y = value[member]["y"].asFloat();
					transform.pos.z = value[member]["z"].asFloat();
					heading = value[member]["p"].asFloat() * MATH_PI / 180;
					attitude = value[member]["r"].asFloat() * MATH_PI / 180;
					bank = value[member]["y"].asFloat() * MATH_PI / 180;
					transform.rot.SetEulerAngles(heading, attitude, bank);
					transform.scale = value[member]["s"].asFloat();
					(*map)[member] = transform;
				}
				else {
					_LogCat("Could not find ", member);
				}
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