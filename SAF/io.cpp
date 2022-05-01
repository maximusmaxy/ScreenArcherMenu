#include "io.h"

#include "common/IDirectoryIterator.h"

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
	
	void InsertOffsetNode(NodeSets* nodeSet, Json::Value& node) {
		std::string offset = node.asString();
		std::string offsetFixed = BSFixedString(offset.c_str()).c_str();
		nodeSet->offsets.insert(offset);
		nodeSet->all.insert(offset);
		nodeSet->allOrBase.insert(offsetFixed);
		nodeSet->fixedConversion[offsetFixed] = offset;
	}

	void InsertOverrideNode(NodeSets* nodeSet, Json::Value& node, bool pose) {
		std::string base = node.asString();
		std::string overrider = base + g_adjustmentManager.overridePostfix;
		std::string baseFixed = BSFixedString(base.c_str()).c_str();
		std::string overriderFixed = BSFixedString(overrider.c_str()).c_str();
		nodeSet->base.insert(base);
		nodeSet->overrides.insert(overrider);
		nodeSet->all.insert(overrider);
		nodeSet->allOrBase.insert(baseFixed);
		nodeSet->allOrBase.insert(overriderFixed);
		nodeSet->fixedConversion[baseFixed] = base;
		nodeSet->fixedConversion[overriderFixed] = overrider;
		nodeSet->baseMap[overrider] = base;
		if (pose) {
			nodeSet->pose.insert(overrider);
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

			Json::Value pose = value["pose"];
			Json::Value offsets = value["offsets"];
			Json::Value overrides = value["overrides"];

			UInt64 key = formId;
			if (isFemale)
				key |= 0x100000000;

			NodeSets* nodeSet = &g_adjustmentManager.nodeSets[key];

			//TODO This probably shouldn't be hard coded
			if (formId == 0x1D698) { //DogmeatRace
				nodeSet->rootName = BSFixedString("Dogmeat_Root");
			}
			else {
				nodeSet->rootName = BSFixedString("Root");
			}

			if (!offsets.isNull()) {
				for (auto& node : offsets) {
					InsertOffsetNode(nodeSet, node);
				}
			}

			if (!pose.isNull()) {
				for (auto& node : pose) {
					InsertOverrideNode(nodeSet, node, true);
				}
			}

			if (!overrides.isNull()) {
				for (auto& node : overrides) {
					InsertOverrideNode(nodeSet, node, false);
				}
			}
		}
		catch (...) {
			_LogCat("Failed to read ", path);
			return false;
		}

		return true;
	}

	bool LoadDefaultAdjustmentFile(std::string path)
	{
		//IFileStream file;

		//if (!file.Open(path.c_str())) {
		//	_LogCat("Could not open ", path);
		//	return false;
		//}

		//std::string defaultAdjustment;
		//ReadAll(&file, &defaultAdjustment);
		//file.Close();

		//Json::Reader reader;
		//Json::Value value;

		//if (!reader.parse(defaultAdjustment, value))
		//{
		//	_LogCat("Failed to parse ", path);
		//	return false;
		//}

		//std::vector<std::string> adjustmentList;

		//try {
		//	std::string mod = value["mod"].asString();
		//	std::string race = value["race"].asString();

		//	UInt32 formId = GetFormID(mod, race);
		//	if (!formId) return false;

		//	std::string sex = value["sex"].asString();
		//	bool isFemale = (sex == "female" || sex == "Female");

		//	UInt64 key = formId;
		//	if (isFemale)
		//		key |= 0x100000000;

		//	Json::Value adjustments = value["adjustments"];

		//	for (auto& adjustment : adjustments) {
		//		adjustmentList.push_back(adjustment.asString());
		//	}

		//	g_adjustmentManager.defaultAdjustments[key] =  adjustmentList;
		//}
		//catch (...) {
		//	_LogCat("Failed to read ", path);
		//	return false;
		//}

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

	void ReadTransformJson(NiTransform& transform, Json::Value& value) {
		transform.pos.x = ReadFloat(value["x"]);
		transform.pos.y = ReadFloat(value["y"]);
		transform.pos.z = ReadFloat(value["z"]);

		float yaw, pitch, roll;
		yaw = ReadFloat(value["yaw"]);
		pitch = ReadFloat(value["pitch"]);
		roll = ReadFloat(value["roll"]);
		MatrixFromDegree(transform.rot, yaw, pitch, roll);

		transform.scale = ReadFloat(value["scale"]);
	}

	void WriteTransformJson(NiTransform* transform, Json::Value& value) {
		char buffer[32];
		sprintf_s(buffer, "%.06f", transform->pos.x);
		value["x"] = Json::Value(buffer);
		sprintf_s(buffer, "%.06f", transform->pos.y);
		value["y"] = Json::Value(buffer);
		sprintf_s(buffer, "%.06f", transform->pos.z);
		value["z"] = Json::Value(buffer);
		float yaw, pitch, roll;
		MatrixToDegree(transform->rot, yaw, pitch, roll);
		sprintf_s(buffer, "%.02f", yaw);
		value["yaw"] = Json::Value(buffer);
		sprintf_s(buffer, "%.02f", pitch);
		value["pitch"] = Json::Value(buffer);
		sprintf_s(buffer, "%.02f", roll);
		value["roll"] = Json::Value(buffer);
		sprintf_s(buffer, "%.06f", transform->scale);
		value["scale"] = Json::Value(buffer);
	}

	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment) {
		
		Json::StyledWriter writer;
		Json::Value value;

		adjustment->ForEachTransform([&](std::string name, NiTransform* transform) {
			if (!TransormIsDefault(*transform)) {
				Json::Value member;
				WriteTransformJson(transform, member);
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

			for (auto& member : members) {
				NiTransform transform;
				ReadTransformJson(transform, value[member]);
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

		for (auto& kvp : *poseMap) {
			Json::Value member;
			WriteTransformJson(&kvp.second, member);
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

	bool LoadPosePath(std::string path, TransformMap* poseMap) 
	{
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_LogCat(path, " not found");
			return false;
		}

		std::string adjustmentString;
		ReadAll(&file, &adjustmentString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(adjustmentString, value)) {
			_LogCat("Failed to parse ", path);
			return false;
		}

		try {
			Json::Value::Members members = value.getMemberNames();

			for (auto& member : members) {
				NiTransform transform;
				ReadTransformJson(transform, value[member]);
				(*poseMap)[member] = transform;
			}
		}
		catch (...) {
			_LogCat("Failed to read ", path);
			return false;
		}

		return true;
	}

	bool LoadPoseFile(std::string filename, TransformMap* poseMap) 
	{
		std::string path("Data\\F4SE\\Plugins\\SAF\\Poses\\");
		path += filename;
		path += ".json";

		return LoadPosePath(filename, poseMap);
	}

	void LoadAllFiles() {
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\NodeMaps", "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadNodeSetsFile(path);
		}
		//for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\DefaultAdjustments", "*.json"); !iter.Done(); iter.Next())
		//{
		//	std::string	path = iter.GetFullPath();
		//	LoadDefaultAdjustmentFile(path);
		//}
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\Actors", "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadUniqueAdjustmentFile(path);
		}
	}
}