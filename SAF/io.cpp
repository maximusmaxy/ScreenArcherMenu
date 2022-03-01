#include "io.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include "f4se/GameData.h"
#include "f4se/NiTypes.h"

#include "json/json.h"

#include "adjustments.h"
#include "util.h"
#include "conversions.h"

namespace SAF {

	void ReadAll(IFileStream* file, std::string* str)
	{
		char ret = file->Read8();
		while (ret > 0) {
			str->push_back(ret);
			ret = file->Read8();
		}
	}

	float ReadFloat(Json::Value& value) {
		if (value.isDouble()) {
			return value.asFloat();
		}
		else if (value.isString()) {
			return std::stof(value.asString());
		}
		return 0.0f;
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
			Json::Value groups = value["groups"];
			Json::Value::Members groupMembers = groups.getMemberNames();

			NodeSets nodeSet;

			for (auto& node : offsets) {
				std::string offset = node.asString();
				std::string offsetFixed = BSFixedString(offset.c_str()).c_str();
				nodeSet.offsets.insert(offset);
				nodeSet.all.insert(offset);
				nodeSet.allOrBase.insert(offsetFixed);
				nodeSet.fixedConversion[offsetFixed] = offset;
			}

			for (auto& node : overrides) {
				std::string base = node.asString();
				std::string overrider = base + g_adjustmentManager.overridePostfix;
				std::string baseFixed = BSFixedString(base.c_str()).c_str();
				std::string overriderFixed = BSFixedString(overrider.c_str()).c_str();
				nodeSet.base.insert(base);
				nodeSet.overrides.insert(overrider);
				nodeSet.all.insert(overrider);
				nodeSet.allOrBase.insert(baseFixed);
				nodeSet.allOrBase.insert(overriderFixed);
				nodeSet.fixedConversion[baseFixed] = base;
				nodeSet.fixedConversion[overriderFixed] = overrider;
				nodeSet.baseMap[overrider] = base;
			}

			for (auto& name : groupMembers) {
				for (auto& node : groups[name]) {
					std::string base = node.asString();
					if (nodeSet.base.count(base)) {
						nodeSet.groups[name].push_back(base);
					}
					else {
						_LogCat("Could not find node for group: ", base);
					}
				}
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

		char buffer[32];

		float yaw, pitch, roll;
		adjustment->ForEachTransform([&](std::string name, NiTransform* transform) {
			if (!TransormIsDefault(*transform)) {
				Json::Value member;
				
				sprintf_s(buffer, "%.06f", transform->pos.x);
				member["x"] = Json::Value(buffer);
				sprintf_s(buffer, "%.06f", transform->pos.y);
				member["y"] = Json::Value(buffer);
				sprintf_s(buffer, "%.06f", transform->pos.z);
				member["z"] = Json::Value(buffer);
				NiToDegree(transform->rot, yaw, pitch, roll);
				sprintf_s(buffer, "%.02f", yaw);
				member["yaw"] = Json::Value(buffer);
				sprintf_s(buffer, "%.02f", pitch);
				member["pitch"] = Json::Value(buffer);
				sprintf_s(buffer, "%.02f", roll);
				member["roll"] = Json::Value(buffer);
				sprintf_s(buffer, "%.06f", transform->scale);
				member["scale"] = Json::Value(buffer);
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

			float yaw, pitch, roll;
			for (auto& member : members) {
				NiTransform transform;
				
				transform.pos.x = ReadFloat(value[member]["x"]);
				transform.pos.y = ReadFloat(value[member]["y"]);
				transform.pos.z = ReadFloat(value[member]["z"]);
				yaw = ReadFloat(value[member]["yaw"]);
				pitch = ReadFloat(value[member]["pitch"]);
				roll = ReadFloat(value[member]["roll"]);
				NiFromDegree(transform.rot, yaw, pitch, roll);
				transform.scale = ReadFloat(value[member]["scale"]);
				(*map)[member] = transform;
			}
		}
		catch (...) {
			_LogCat("Failed to read ", filename);
			return false;
		}

		return true;
	}

	bool SavePoseFile(std::string filename, TransformMap* poseMap) {
		Json::StyledWriter writer;
		Json::Value value;

		char buffer[32];

		float yaw, pitch, roll;
		for (auto& kvp : *poseMap) {
			Json::Value member;
			sprintf_s(buffer, "%.06f", kvp.second.pos.x);
			member["x"] = Json::Value(buffer);
			sprintf_s(buffer, "%.06f", kvp.second.pos.y);
			member["y"] = Json::Value(buffer);
			sprintf_s(buffer, "%.06f", kvp.second.pos.z);
			member["z"] = Json::Value(buffer);
			NiToDegree(kvp.second.rot, yaw, pitch, roll);
			sprintf_s(buffer, "%.02f", yaw);
			member["yaw"] = Json::Value(buffer);
			sprintf_s(buffer, "%.02f", pitch);
			member["pitch"] = Json::Value(buffer);
			sprintf_s(buffer, "%.02f", roll);
			member["roll"] = Json::Value(buffer);
			sprintf_s(buffer, "%.06f", kvp.second.scale);
			member["scale"] = Json::Value(buffer);
			value[kvp.first] = member;
		}

		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Poses\\");
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

	bool LoadPoseFile(std::string filename, TransformMap* poseMap) {
		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Poses\\");
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

			float yaw, pitch, roll;
			for (auto& member : members) {
				NiTransform transform;
				transform.pos.x = ReadFloat(value[member]["x"]);
				transform.pos.y = ReadFloat(value[member]["y"]);
				transform.pos.z = ReadFloat(value[member]["z"]);
				yaw = ReadFloat(value[member]["yaw"]);
				pitch = ReadFloat(value[member]["pitch"]);
				roll = ReadFloat(value[member]["roll"]);
				NiFromDegree(transform.rot, yaw, pitch, roll);
				transform.scale = ReadFloat(value[member]["scale"]);
				(*poseMap)[member] = transform;
			}
		}
		catch (...) {
			_LogCat("Failed to read ", filename);
			return false;
		}

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