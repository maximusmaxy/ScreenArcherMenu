#include "io.h"

#include "common/IDirectoryIterator.h"

#include "f4se/GameData.h"
#include "f4se/NiTypes.h"

#include "util.h"
#include "conversions.h"
#include "settings.h"

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
using namespace rapidxml;

namespace SAF {
	std::regex tabSeperatedRegex("([^\\t]+)\\t+([^\\t]+)");		//matches (1)\t(2)
	std::regex tabOptionalRegex("([^\\t]+)(?:\\t+([^\\t]+))?");	//matches (1) or (1)\t(2)

	InsensitiveUInt32Map menuHeaderMap = {
		{"race", kMenuHeaderRace},
		{"mod", kMenuHeaderMod},
		{"sex", kMenuHeaderSex},
		{"type", kMenuHeaderType},
	};

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

	void InsertNode(NodeSets* nodeSet, const char* name, bool offset)
	{
		BSFixedString baseStr(name);
		BSFixedString offsetStr((name + g_settings.offset).c_str());
		
		NodeKey offsetKey(baseStr, true);
		nodeSet->offsets.insert(offsetKey);
		nodeSet->all.insert(offsetKey);
		nodeSet->nodeKeys.emplace(offsetStr, offsetKey);
		nodeSet->baseStrings.insert(baseStr);
		nodeSet->allStrings.insert(offsetStr);
		
		//if offset don't insert the pose nodes
		if (!offset) {
			NodeKey baseKey(baseStr, false);
			nodeSet->pose.insert(baseKey);
			nodeSet->all.insert(baseKey);
			nodeSet->nodeKeys.emplace(baseStr, baseKey);
			nodeSet->allStrings.insert(baseStr);
			nodeSet->baseNodeStrings.insert(baseStr);
		}

		nodeSet->offsetMap.emplace(baseStr, offsetStr);
	}

	bool LoadNodeSetsJson(std::string path)
	{
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_Logs("Could not open ", path);
			return false;
		}

		std::string nodeMap;
		ReadAll(&file, &nodeMap);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(nodeMap, value))
		{
			_Logs("Failed to parse ", path);
			return false;
		}

		try {
			std::string mod = value["mod"].asString();
			std::string race = value["race"].asString();

			UInt32 formId = GetFormId(mod.c_str(), race.c_str());
			if (!formId) return false;

			const char* sex = value["sex"].asCString();
			bool isFemale = !_stricmp(sex, "female");

			UInt64 key = formId;
			if (isFemale)
				key |= 0x100000000;

			NodeSets* nodeSet = &g_adjustmentManager.nodeSets[key];

			Json::Value pose = value["pose"];
			Json::Value offsets = value["offsets"];
			Json::Value overrides = value["overrides"];

			//TODO This probably shouldn't be hard coded
			if (formId == 0x1D698) { //DogmeatRace
				nodeSet->rootName = BSFixedString("Dogmeat_Root");
			}
			else {
				nodeSet->rootName = BSFixedString("Root");
			}

			if (!offsets.isNull()) {
				for (auto& node : offsets) {
					InsertNode(nodeSet, node.asCString(), true);
				}
			}

			//As of Update v1.0 pose/overrides have been combined so just treat them as the same thing for legacy support
			if (!pose.isNull()) {
				for (auto& node : pose) {
					InsertNode(nodeSet, node.asCString(), false);
				}
			}
			if (!overrides.isNull()) {
				for (auto& node : overrides) {
					InsertNode(nodeSet, node.asCString(), false);
				}
			}
		}
		catch (...) {
			_Logs("Failed to read ", path);
			return false;
		}

		return true;
	}

	enum {
		kMenuTypeNodes = 1,
		kMenuTypeRace,
		kMenuTypeActor
	};

	InsensitiveUInt32Map safTypeMap = {
		{ "nodes", kMenuTypeNodes },
		{ "race", kMenuTypeRace },
		{ "actor", kMenuTypeActor }
	};

	enum {
		kNodeTypeOffset = 1,
		kNodeTypePose,
		kNodeTypeRoot
	};

	InsensitiveUInt32Map nodeTypeMap = {
		{ "offset", kNodeTypeOffset },
		{ "pose", kNodeTypePose },
		{ "override", kNodeTypePose },
		{ "root", kNodeTypeRoot }
	};

	enum {
		kAutoTypeAdjustment = 1
	};

	InsensitiveUInt32Map autoTypeMap = {
		{ "adjustment", kAutoTypeAdjustment },
		{ "adjustments", kAutoTypeAdjustment }
	};

	bool TsvReader::Read() {
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_Logs(path.c_str(), " not found");
			return false;
		}

		char buf[512];
		std::cmatch match;
		bool finalized = false;

		try {
			while (!file.HitEOF()) {
				file.ReadString(buf, 512, '\n', '\r');
				if (std::regex_search(buf, match, tabOptionalRegex)) {
					if (!_stricmp(match[1].str().c_str(), "category")) {
						if (!finalized) {
							if (!FinalizeHeader(match[1].str(), match[2].str()))
								return false;
							finalized = true;
						}
						ReadCategory(match[1].str(), match[2].str());
					}
					else if (!finalized) {
						ReadHeader(match[1].str(), match[2].str());
					}
					else {
						ReadLine(match[1].str(), match[2].str());
					}
				}
			}
		}
		catch (...) {
			_Logs("Failed to read ", path);
			return false;
		}

		return true;
	}

	void TsvReader::ReadHeader(std::string m1, std::string m2)
	{
		auto it = menuHeaderMap.find(m1);
		if (m2.size() && it != menuHeaderMap.end())
		{
			switch (it->second)
			{
			case kMenuHeaderRace:
				header.race = m2;
				break;
			case kMenuHeaderMod:
				header.mod = m2;
				break;
			case kMenuHeaderSex:
				header.isFemale = !_stricmp(m2.c_str(), "female");
				break;
			case kMenuHeaderType:
				auto value = typeMap->find(m2);
				if (value != typeMap->end())
				{
					header.type = value->second;
				}
				break;
			}
		}
	}

	class SafTsvReader : public TsvReader {
	public:
		SafTsvReader(std::string path, InsensitiveUInt32Map* typeMap) :
			TsvReader(path, typeMap),
			nodeSet(nullptr)
		{};

		NodeSets* nodeSet;

		bool FinalizeHeader(std::string m1, std::string m2) 
		{
			if (!header.type)
			{
				_Logs("Failed to read header type", path);
				return false;
			}

			key = GetFormId(header.mod.c_str(), header.race.c_str());

			if (!key)
			{
				_Logs("Failed to read header race or mod", path);
				return false;
			}
			if (header.isFemale)
				key |= 0x100000000;

			nodeSet = &g_adjustmentManager.nodeSets[key];

			//These are defaults and will get replaced by Category Root
			if (key == 0x1D698) { //DogmeatRace
				nodeSet->rootName = BSFixedString("Dogmeat_Root");
			}
			else {
				nodeSet->rootName = BSFixedString("Root");
			}

			return true;
		}

		void ReadCategory(std::string m1, std::string m2) 
		{
			switch (header.type) {
			case kMenuTypeNodes:
			{
				auto it = nodeTypeMap.find(m2);
				categoryIndex = it != nodeTypeMap.end() ? it->second : 0;
				break;
			}
			case kMenuTypeRace:
			case kMenuTypeActor:
			{
				auto it = autoTypeMap.find(m2);
				categoryIndex = it != autoTypeMap.end() ? it->second : 0;
				break;
			}
			}
		}

		void ReadLine(std::string m1, std::string m2) 
		{
			switch (header.type) {
			case kMenuTypeNodes: 
			{
				switch (categoryIndex) {
				case kNodeTypeOffset:
					InsertNode(nodeSet, m1.c_str(), true);
					break;
				case kNodeTypePose:
					InsertNode(nodeSet, m1.c_str(), false);
					break;
				case kNodeTypeRoot:
					nodeSet->rootName = BSFixedString(m1.c_str());
					break;
				}
				break;
			}
			case kMenuTypeRace:
				g_adjustmentManager.autoRaceCache[key].emplace(m1);
				break;
			case kMenuTypeActor:
				g_adjustmentManager.autoActorCache[key].emplace(m1);
				break;
			}
		}
	};

	bool LoadNodeSetsTsv(std::string path)
	{
		SafTsvReader reader(path, &safTypeMap);
		return reader.Read();
	}

	bool LoadRaceAdjustmentFile(std::string path)
	{
		//IFileStream file;

		//if (!file.Open(path.c_str())) {
		//	_Logs("Could not open ", path);
		//	return false;
		//}

		//std::string defaultAdjustment;
		//ReadAll(&file, &defaultAdjustment);
		//file.Close();

		//Json::Reader reader;
		//Json::Value value;

		//if (!reader.parse(defaultAdjustment, value))
		//{
		//	_Logs("Failed to parse ", path);
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
		//	_Logs("Failed to read ", path);
		//	return false;
		//}

		return true;
	}

	//bool LoadUniqueAdjustmentFile(std::string path) {
	//	IFileStream file;

	//	if (!file.Open(path.c_str())) {
	//		_Logs(path.c_str(), " not found");
	//		return false;
	//	}

	//	std::string actorString;
	//	ReadAll(&file, &actorString);
	//	file.Close();

	//	Json::Reader reader;
	//	Json::Value value;

	//	if (!reader.parse(actorString, value)) {
	//		_Logs("Failed to parse ", path);
	//		return false;
	//	}

	//	try {
	//		std::string mod = value["mod"].asString();
	//		std::string actor = value["actor"].asString();

	//		UInt32 formId = GetFormId(mod, actor);
	//		if (!formId) return false;

	//		Json::Value members = value["adjustments"];
	//		std::vector<std::pair<std::string, std::string>> adjustments;

	//		for (auto& member : members) {
	//			adjustments.push_back(std::make_pair(member.asString(), mod));
	//		}

	//		g_adjustmentManager.uniqueAdjustments[formId] = adjustments;
	//	}
	//	catch (...) {
	//		_Logs("Failed to read ", path);
	//		return false;
	//	}

	//	return true;
	//}

	enum {
		kTransformJsonTranspose = 0,
		kTransformJsonPose,
		kTransformJsonAdjustment
	};

	void ReadTransformJson(SamTransform& transform, Json::Value& value, UInt32 version) {
		transform.pos.x = ReadJsonFloat(value["x"]);
		transform.pos.y = ReadJsonFloat(value["y"]);
		transform.pos.z = ReadJsonFloat(value["z"]);

		float yaw, pitch, roll;
		yaw = ReadJsonFloat(value["yaw"]);
		pitch = ReadJsonFloat(value["pitch"]);
		roll = ReadJsonFloat(value["roll"]);

		switch (version) {
		case kTransformJsonTranspose:
			MatrixFromDegree(transform.rot, yaw, pitch, roll);
			break;
		case kTransformJsonPose:
			MatrixFromPose(transform.rot, yaw, pitch, roll);
			break;
		case kTransformJsonAdjustment:
			MatrixFromEulerYPR2(transform.rot, yaw * DEGREE_TO_RADIAN, pitch * DEGREE_TO_RADIAN, roll * DEGREE_TO_RADIAN);
			break;
		}

		transform.scale = ReadJsonFloat(value["scale"]);
	}

	void WriteTransformJson(SamTransform* transform, Json::Value& value, UInt32 version) {
		char buffer[32];
		WriteJsonFloat(value["x"], transform->pos.x, "%.06f");
		WriteJsonFloat(value["y"], transform->pos.y, "%.06f");
		WriteJsonFloat(value["z"], transform->pos.z, "%.06f");

		float yaw, pitch, roll;
		switch (version) {
		case kTransformJsonTranspose:
			MatrixToDegree(transform->rot, yaw, pitch, roll);
			break;
		case kTransformJsonPose:
			MatrixToPose(transform->rot, yaw, pitch, roll);
			break;
		case kTransformJsonAdjustment:
			MatrixToEulerYPR2(transform->rot, yaw, pitch, roll);
			yaw *= RADIAN_TO_DEGREE;
			pitch *= RADIAN_TO_DEGREE;
			roll *= RADIAN_TO_DEGREE;
			break;
		}

		WriteJsonFloat(value["yaw"], yaw, "%.02f");
		WriteJsonFloat(value["pitch"], pitch, "%.02f");
		WriteJsonFloat(value["roll"], roll, "%.02f");
		WriteJsonFloat(value["scale"], transform->scale, "%.06f");
	}

	/*
	* v0 was just a simple key->transform map
	* The wrong conversion from euler to matrix was being used so we need to add versioning for legacy support
	*
	* v1 header/version/name added
	*/

	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment) {
		Json::StyledWriter writer;
		Json::Value value;
		value["version"] = 1;
		value["name"] = getFilename(filename);

		Json::Value transforms(Json::ValueType::objectValue);

		adjustment->ForEachTransform([&](const NodeKey* nodeKey, SamTransform* transform) {
			if (!transform->IsDefault()) {
				Json::Value member;
				WriteTransformJson(transform, member, kTransformJsonAdjustment);
				transforms[GetNodeKeyName(*nodeKey)] = member;
			}
		});

		value["transforms"] = transforms;

		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Adjustments\\");
		path += filename;
		path += ".json";

		IFileStream::MakeAllDirs(path.c_str());
		if (!file.Create(path.c_str())) {
			_Logs("Failed to create file ", filename);
			return false;
		}

		std::string jsonString = writer.write(value);
		file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
		file.Close();

		return true;
	}

	bool LoadAdjustmentFile(std::string filename, LoadedAdjustment* adjustment) {
		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Adjustments\\");
		path += filename;
		path += ".json";

		if (!file.Open(path.c_str())) {
			_Logs(filename.c_str(), " not found");
			return false;
		}

		std::string adjustmentString;
		ReadAll(&file, &adjustmentString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(adjustmentString, value)) {
			_Logs("Failed to parse ", filename);
			return false;
		}

		try {
			//If no version treat as simple key->transform map, else get map from transforms property
			UInt32 version = value.get("version", 0).asUInt();

			//if prior to version 1 it needs to be updated
			adjustment->updateType = version < 1 ? kAdjustmentUpdateFile : kAdjustmentUpdateNone;

			//get transforms, or just use itself
			Json::Value transforms = value.get("transforms", value);

			for (auto it = transforms.begin(); it != transforms.end(); ++it) {
				SamTransform transform;
				//read version 2 if non zero version
				ReadTransformJson(transform, *it, (version > 0 ? kTransformJsonPose : kTransformJsonTranspose));
				NodeKey nodeKey = GetNodeKeyFromString(it.key().asCString());
				if (nodeKey.key) {
					adjustment->map->emplace(nodeKey, transform);
				}
			}
		}
		catch (...) {
			_Logs("Failed to read ", filename);
			adjustment->updateType = kAdjustmentUpdateNone;
			return false;
		}

		return true;
	}

	/*
	* v0 was just a simple key->transform map
	* There was also a bug causing the output rotations to be transposed, this is fixed in future versions
	* 
	* V1 header/version/name added
	* V2 skeleton added for compatibility with the .hkx pose converter
	*/

	bool SavePoseFile(std::string filename, TransformMap* poseMap, const char* skeleton) {
		Json::StyledWriter writer;
		Json::Value value;

		value["version"] = 2;
		value["name"] = getFilename(filename);
		value["skeleton"] = (skeleton ? skeleton : "Vanilla");

		Json::Value transforms(Json::ValueType::objectValue);

		for (auto& kvp : *poseMap) {
			Json::Value member;
			WriteTransformJson(&kvp.second, member, kTransformJsonPose);
			transforms[GetNodeKeyName(kvp.first)] = member;
		}

		value["transforms"] = transforms;

		IFileStream file;

		std::string path("Data\\F4SE\\Plugins\\SAF\\Poses\\");
		path += filename;
		path += ".json";

		IFileStream::MakeAllDirs(path.c_str());
		if (!file.Create(path.c_str())) {
			_Logs("Failed to create file ", filename);
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
			_Logs(path.c_str(), " not found");
			return false;
		}

		std::string adjustmentString;
		ReadAll(&file, &adjustmentString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(adjustmentString, value)) {
			_Logs("Failed to parse ", path);
			return false;
		}

		try {
			//If no version treat as simple key->transform map, else get map from transforms property
			UInt32 version = value.get("version", 0).asUInt();

			//get transforms, or just use itself
			Json::Value transforms = value.get("transforms", value);

			for (auto it = transforms.begin(); it != transforms.end(); ++it) {
				SamTransform transform;
				ReadTransformJson(transform, *it, version > 0 ? kTransformJsonPose : kTransformJsonTranspose);
				NodeKey nodeKey = GetNodeKeyFromString(it.key().asCString());
				if (nodeKey.key) {
					poseMap->emplace(nodeKey, transform);
				}
			}
		}
		catch (...) {
			_Logs("Failed to read ", path);
			return false;
		}

		return true;
	}

	bool LoadPoseFile(std::string filename, TransformMap* poseMap)
	{
		std::string path("Data\\F4SE\\Plugins\\SAF\\Poses\\");
		path += filename;
		path += ".json";

		return LoadPosePath(path, poseMap);
	}

	bool SaveOutfitStudioXml(std::string filename, TransformMap* poseMap)
	{
		IFileStream file;

		std::string path("Data\\tools\\bodyslide\\PoseData\\");
		path += filename;
		path += ".xml";

		IFileStream::MakeAllDirs(path.c_str());
		if (!file.Create(path.c_str())) {
			_Logs("Failed to create file ", filename);
			return false;
		}

		xml_document<> doc;
		xml_node<>* declaration = doc.allocate_node(node_declaration);
		declaration->append_attribute(doc.allocate_attribute("version", "1.0"));
		declaration->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(declaration);
		xml_node<>* poseData = doc.allocate_node(node_element, "PoseData");
		doc.append_node(poseData);
		xml_node<>* pose = doc.allocate_node(node_element, "Pose");
		pose->append_attribute(doc.allocate_attribute("name", filename.c_str()));
		poseData->append_node(pose);

		for (auto& node : *poseMap) {
			xml_node<>* bone = doc.allocate_node(node_element, "Bone");
			bone->append_attribute(doc.allocate_attribute("name", doc.allocate_string(GetNodeKeyName(node.first).c_str())));

			Vector3 rot = MatrixToOutfitStudioVector(node.second.rot);
			bone->append_attribute(doc.allocate_attribute("rotX", doc.allocate_string(std::to_string(-rot.x).c_str())));
			bone->append_attribute(doc.allocate_attribute("rotY", doc.allocate_string(std::to_string(-rot.y).c_str())));
			bone->append_attribute(doc.allocate_attribute("rotZ", doc.allocate_string(std::to_string(-rot.z).c_str())));

			bone->append_attribute(doc.allocate_attribute("transX", doc.allocate_string(std::to_string(node.second.pos.x).c_str())));
			bone->append_attribute(doc.allocate_attribute("transY", doc.allocate_string(std::to_string(node.second.pos.y).c_str())));
			bone->append_attribute(doc.allocate_attribute("transZ", doc.allocate_string(std::to_string(node.second.pos.z).c_str())));

			bone->append_attribute(doc.allocate_attribute("scale", doc.allocate_string(std::to_string(node.second.scale).c_str())));

			pose->append_node(bone);
		}

		std::string out;
		print(std::back_inserter(out), doc, 0);

		try {
			file.WriteBuf(out.c_str(), out.size() - 1);
			file.Close();
		}
		catch (...) {
			_Logs("Failed to write ", path);
			return false;
		}

		return true;
	}

	void SaveSettingsFile(const char* path)
	{
		IFileStream file;

		try {
			Json::StyledWriter writer;
			Json::Value value;

			g_settings.ToJson(value);

			std::string jsonString = writer.write(value);

			if (file.Create(path))
			{
				file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
				file.Close();
			}
		}
		catch (...) {
			_DMESSAGE("Failed to create settings json");
		}
	}

	void LoadSettingsFile(const char* path)
	{
		IFileStream file;

		//if settings doesn't exist generate it
		if (!file.Open(path)) {

			g_settings.Initialize();
			SaveSettingsFile(path);

			return;
		}

		std::string settingString;
		ReadAll(&file, &settingString);
		file.Close();

		Json::Reader reader;
		Json::Value value;

		if (!reader.parse(settingString, value)) {
			_DMESSAGE("Failed to parse settings json");
			g_settings.Initialize();
			return;
		}

		g_settings.FromJson(value);

		//new settings might have been added so resave the json
		SaveSettingsFile(path);
	}

	void LoadAllFiles() {
		LoadSettingsFile("Data\\F4SE\\Plugins\\SAF\\settings.json");

		//node set jsons for legacy support
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\NodeMaps", "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadNodeSetsJson(path);
		}

		//node set text files
		for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\NodeMaps", "*.txt"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadNodeSetsTsv(path);
		}

		//for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\Race", "*.json"); !iter.Done(); iter.Next())
		//{
		//	std::string	path = iter.GetFullPath();
		//	LoadRaceAdjustmentFile(path);
		//}
		//for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\Actors", "*.json"); !iter.Done(); iter.Next())
		//{
		//	std::string	path = iter.GetFullPath();
		//	LoadUniqueAdjustmentFile(path);
		//}
	}
}