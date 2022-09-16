#include "io.h"

#include "common/IDirectoryIterator.h"

#include "f4se/GameData.h"
#include "f4se/NiTypes.h"

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

	float ReadJsonFloat(Json::Value& value) {
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
			_DMESSAGE("Could not open ", path);
			return false;
		}

		std::string nodeMap;
		ReadAll(&file, &nodeMap);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(nodeMap, value))
		{
			_DMESSAGE("Failed to parse ", path);
			return false;
		}

		try {
			std::string mod = value["mod"].asString();
			std::string race = value["race"].asString();

			UInt32 formId = GetFormId(mod, race);
			if (!formId) return false;

			const char* sex = value["sex"].asCString();
			bool isFemale = !_stricmp(sex, "female");

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
			_DMESSAGE("Failed to read ", path);
			return false;
		}

		return true;
	}

	bool LoadDefaultAdjustmentFile(std::string path)
	{
		//IFileStream file;

		//if (!file.Open(path.c_str())) {
		//	_DMESSAGE("Could not open ", path);
		//	return false;
		//}

		//std::string defaultAdjustment;
		//ReadAll(&file, &defaultAdjustment);
		//file.Close();

		//Json::Reader reader;
		//Json::Value value;

		//if (!reader.parse(defaultAdjustment, value))
		//{
		//	_DMESSAGE("Failed to parse ", path);
		//	return false;
		//}

		//std::vector<std::string> adjustmentList;

		//try {
		//	std::string mod = value["mod"].asString();
		//	std::string race = value["race"].asString();

		//	UInt32 formId = GetFormId(mod, race);
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
		//	_DMESSAGE("Failed to read ", path);
		//	return false;
		//}

		return true;
	}

	bool LoadUniqueAdjustmentFile(std::string path) {
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_DMESSAGE(path.c_str(), " not found");
			return false;
		}

		std::string actorString;
		ReadAll(&file, &actorString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(actorString, value)) {
			_DMESSAGE("Failed to parse ", path);
			return false;
		}

		try {
			std::string mod = value["mod"].asString();
			std::string actor = value["actor"].asString();

			UInt32 formId = GetFormId(mod, actor);
			if (!formId) return false;

			Json::Value members = value["adjustments"];
			std::vector<std::pair<std::string, std::string>> adjustments;

			for (auto& member : members) {
				adjustments.push_back(std::make_pair(member.asString(), mod));
			}

			g_adjustmentManager.uniqueAdjustments[formId] = adjustments;
		}
		catch (...) {
			_DMESSAGE("Failed to read ", path);
			return false;
		}

		return true;
	}

	void ReadTransformJson(NiTransform& transform, Json::Value& value, UInt32 version) {
		transform.pos.x = ReadJsonFloat(value["x"]);
		transform.pos.y = ReadJsonFloat(value["y"]);
		transform.pos.z = ReadJsonFloat(value["z"]);

		float yaw, pitch, roll;
		yaw = ReadJsonFloat(value["yaw"]);
		pitch = ReadJsonFloat(value["pitch"]);
		roll = ReadJsonFloat(value["roll"]);

		switch (version) {
		case 0:
			MatrixFromDegree(transform.rot, yaw, pitch, roll);
			break;
		case 1:
			MatrixFromPose(transform.rot, yaw, pitch, roll);
			break;
		}

		transform.scale = ReadJsonFloat(value["scale"]);
	}

	void WriteTransformJson(NiTransform* transform, Json::Value& value, UInt32 version) {
		char buffer[32];
		WriteJsonFloat(value["x"], transform->pos.x, "%.06f");
		WriteJsonFloat(value["y"], transform->pos.y, "%.06f");
		WriteJsonFloat(value["z"], transform->pos.z, "%.06f");

		float yaw, pitch, roll;
		switch (version) {
		case 0:
			MatrixToDegree(transform->rot, yaw, pitch, roll);
			break;
		case 1:
			MatrixToPose(transform->rot, yaw, pitch, roll);
			break;
		}

		WriteJsonFloat(value["yaw"], yaw, "%.02f");
		WriteJsonFloat(value["pitch"], pitch, "%.02f");
		WriteJsonFloat(value["roll"], roll, "%.02f");
		WriteJsonFloat(value["scale"], transform->scale, "%.06f");
	}

	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment) {
		Json::StyledWriter writer;
		Json::Value value;

		adjustment->ForEachTransform([&](std::string name, NiTransform* transform) {
			if (!TransormIsDefault(*transform)) {
				Json::Value member;
				WriteTransformJson(transform, member, 0);
				value[name] = member;
			}
		});

		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Adjustments\\");
		path += filename;
		path += ".json";

		IFileStream::MakeAllDirs(path.c_str());
		if (!file.Create(path.c_str())) {
			_DMESSAGE("Failed to create file ", filename);
			return false;
		}

		std::string jsonString = writer.write(value);
		file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
		file.Close();

		return true;
	}

	bool LoadAdjustmentFile(std::string filename, TransformMap* map) {
		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Adjustments\\");
		path += filename;
		path += ".json";

		if (!file.Open(path.c_str())) {
			_DMESSAGE(filename.c_str(), " not found");
			return false;
		}

		std::string adjustmentString;
		ReadAll(&file, &adjustmentString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(adjustmentString, value)) {
			_DMESSAGE("Failed to parse ", filename);
			return false;
		}

		try {
			Json::Value::Members members = value.getMemberNames();

			for (auto& member : members) {
				NiTransform transform;
				ReadTransformJson(transform, value[member], 0);
				(*map)[member] = transform;
			}
		}
		catch (...) {
			_DMESSAGE("Failed to read ", filename);
			return false;
		}

		return true;
	}

	/*
	* Pre V0.5 was just a simple key->transform map
	* There was also a bug causing the output rotations to be transposed, this is fixed in future versions
	* 
	* Post 0.5 will now have a header with version numbers in case of updates
	*/

	bool SavePoseFile(std::string filename, TransformMap* poseMap) {
		Json::StyledWriter writer;
		Json::Value value;

		value["version"] = Json::Value(1);
		value["name"] = Json::Value(getFilename(filename));

		Json::Value transforms(Json::ValueType::objectValue);

		for (auto& kvp : *poseMap) {
			Json::Value member;
			WriteTransformJson(&kvp.second, member, 1);
			transforms[kvp.first] = member;
		}

		value["transforms"] = transforms;

		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Poses\\");
		path += filename;
		path += ".json";

		IFileStream::MakeAllDirs(path.c_str());
		if (!file.Create(path.c_str())) {
			_DMESSAGE("Failed to create file ", filename);
			return false;
		}

		std::string jsonString = writer.write(value);
		file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
		file.Close();

		return true;
	}

	bool LoadPosePath(std::string path, TransformMap* poseMap) 
	{
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_DMESSAGE(path.c_str(), " not found");
			return false;
		}

		std::string adjustmentString;
		ReadAll(&file, &adjustmentString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(adjustmentString, value)) {
			_DMESSAGE("Failed to parse ", path);
			return false;
		}

		try {
			//If no version treat as simple key->transform map, else get map from transforms property
			Json::Value versionValue = value["version"];
			UInt32 version = versionValue.isNull() ? 0 : versionValue.asInt();

			Json::Value transforms = value["transforms"];
			if (version == 0 || transforms.isNull())
				transforms = value;

			Json::Value::Members members = transforms.getMemberNames();

			for (auto& member : members) {
				NiTransform transform;
				ReadTransformJson(transform, transforms[member], version);
				(*poseMap)[member] = transform;
			}
		}
		catch (...) {
			_DMESSAGE("Failed to read ", path);
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

	void LoadSettingsFile(const char* path)
	{
		//defaults
		g_adjustmentManager.overridePostfix = "_Pose";
		g_adjustmentManager.offsetPostfix = "_Offset";

		IFileStream file;

		//if settings doesn't exist generate it
		if (!file.Open(path)) {
			try {
				Json::StyledWriter writer;
				Json::Value value;

				value["override"] = Json::Value("_Pose");
				value["offset"] = Json::Value("_Offset");


				std::string jsonString = writer.write(value);

				if (file.Create(path))
				{
					file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
					file.Close();
				}
			}
			catch (...) {
				_DMESSAGE("Failed to create settings json");
				return;
			}

			return;
		}

		std::string settingString;
		ReadAll(&file, &settingString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(settingString, value)) {
			_DMESSAGE("Failed to parse settings json");
			return;
		}

		g_adjustmentManager.overridePostfix = value["override"].isString() ? value["override"].asString() : "_Pose";
		g_adjustmentManager.offsetPostfix = value["offset"].isString() ? value["offset"].asString() : "_Offset";
	}

	void LoadAllFiles() {
		LoadSettingsFile("Data\\F4SE\\Plugins\\SAF\\settings.json");

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