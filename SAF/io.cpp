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

#include <filesystem>
#include <io.h>
#include <fcntl.h>

namespace SAF {
	std::regex tabSeperatedRegex("([^\\t]+)\\t+([^\\t]+)");		//matches (1)\t(2)
	std::regex tabOptionalRegex("([^\\t]+)(?:\\t+([^\\t]+))?");	//matches (1) or (1)\t(2)

	InsensitiveUInt32Map menuHeaderMap = {
		{"form", kMenuHeaderForm},
		{"race", kMenuHeaderForm},
		{"npc", kMenuHeaderForm},
		{"actor", kMenuHeaderForm},
		{"mod", kMenuHeaderMod},
		{"sex", kMenuHeaderSex},
		{"gender", kMenuHeaderSex},
		{"type", kMenuHeaderType},
	};

	InsensitiveUInt32Map genderMap = {
		{"m", kGenderMale },
		{"male", kGenderMale },
		{"boy", kGenderMale },
		{"femboy", kGenderMale }, //O_O
		{"f", kGenderFemale },
		{"female", kGenderFemale },
		{"girl", kGenderFemale }
	};

	float ReadJsonFloat(Json::Value& value, const char* key, float defaultValue) {
		Json::Value result = value.get(key, defaultValue);
		
		if (result.isDouble()) {
			return result.asFloat();
		}
		else if (result.isString()) {
			try {
				return std::stof(result.asString());
			}
			catch (...) {
				return defaultValue;
			}
		}

		return defaultValue;
	}

	void WriteJsonFloat(Json::Value& json, const char* key, float value, char* buffer, const char* format)
	{
		sprintf_s(buffer, FLOAT_BUFFER_LEN, format, value);
		json[key] = Json::Value(buffer);
	}

	bool ReadJsonFile(const char* path, Json::Value& value)
	{
		std::ifstream stream;
		stream.open(path);

		if (stream.fail()) {
			_Log("Failed to open json file for read: ", path);
			return false;
		}
			
		Json::Reader reader;
		if (!reader.parse(stream, value)) {
			_DMESSAGE(reader.getFormattedErrorMessages().c_str());
			return false;
		}

		return true;
	}

	//ofstreams weren't opening correctly the regular way so i'm using CreateFile
	//When opened this way you need to close the stream manually to release the handles
	OutStreamWrapper::OutStreamWrapper(const char* path) 
	{
		fail = true;

		IFileStream::MakeAllDirs(path);

		auto fileHandle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE)
			return;

		auto file_descriptor = _open_osfhandle((intptr_t)fileHandle, 0);
		if (file_descriptor == -1)
			return;

		FILE* file = _fdopen(file_descriptor, "w");

		stream = std::ofstream(file);

		if (stream.fail()) {
			_Log("Failed to open output file for write: ", path);
			return;
		}

		fail = false;
	}

	OutStreamWrapper::~OutStreamWrapper()
	{
		stream.close();
	}

	//bool OpenOutFileStream(const char* path, std::ofstream* stream)
	//{
	//	IFileStream::MakeAllDirs(path);

	//	auto fileHandle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	//	if (fileHandle == INVALID_HANDLE_VALUE)
	//		return false;

	//	auto file_descriptor = _open_osfhandle((intptr_t)fileHandle, 0);
	//	if (file_descriptor == -1)
	//		return false;

	//	FILE* file = _fdopen(file_descriptor, "w");

	//	*stream = std::ofstream(file);

	//	if (stream->fail())
	//	{
	//		_Log("Failed to open output file for write: ", path);
	//		stream->close();
	//		return false;
	//	}
	//	
	//	return true;
	//}

	//bool OpenAppendFileStream(const char* path, std::ofstream* stream)
	//{
	//	IFileStream::MakeAllDirs(path);

	//	auto fileHandle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	//	if (fileHandle == INVALID_HANDLE_VALUE)
	//		return false;

	//	auto file_descriptor = _open_osfhandle((intptr_t)fileHandle, _O_APPEND);
	//	if (file_descriptor == -1)
	//		return false;

	//	FILE* file = _fdopen(file_descriptor, "a");

	//	*stream = std::ofstream(file);

	//	if (stream->fail())
	//	{
	//		_Log("Failed to open output file for write: ", path);
	//		stream->close();
	//		return false;
	//	}

	//	return true;
	//}

	bool WriteJsonFile(const char* path, Json::Value& value)
	{
		OutStreamWrapper wrapper(path);
		if (wrapper.fail)
			return false;
		
		Json::StyledStreamWriter writer;
		writer.write(wrapper.stream, value);

		return true;
	}

	void InsertNode(NodeSets* nodeSet, const char* name, bool offset)
	{
		BSFixedString baseStr(name);
		BSFixedString offsetStr((name + g_adjustmentManager.settings.offset).c_str());
		
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

	void UpdateCaseMap(UInt64 key, const char* name) {

		//if adding a node to the human male, add it to the casing map
		if (key == 0x13746) {
			g_adjustmentManager.nodeCasingMap.emplace(BSFixedString(name), name);
		}
	}

	bool LoadNodeSetsJson(const char* path)
	{
		Json::Value value;
		if (!ReadJsonFile(path, value))
			return false;

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
			_Log("Failed to read ", path);
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
		{ "node", kMenuTypeNodes },
		{ "race", kMenuTypeRace },
		{ "races", kMenuTypeRace },
		{ "actors", kMenuTypeActor },
		{ "actor", kMenuTypeActor },
		{ "npc", kMenuTypeActor },
		{ "npcs", kMenuTypeActor }
	};

	enum {
		kNodeTypeOffset = 1,
		kNodeTypePose,
		kNodeTypeRoot,
		kNodeTypeCenter,
		kNodeTypeMirror
	};

	InsensitiveUInt32Map nodeTypeMap = {
		{ "offset", kNodeTypeOffset },
		{ "pose", kNodeTypePose },
		{ "override", kNodeTypePose },
		{ "overide", kNodeTypePose },
		{ "root", kNodeTypeRoot },
		{ "center", kNodeTypeCenter },
		{ "centre", kNodeTypeCenter },
		{ "mirror", kNodeTypeMirror },
	};

	enum {
		kDefaultAdjustment = 1
	};

	InsensitiveUInt32Map defaultAdjustmentTypeMap = {
		{ "adjustment", kDefaultAdjustment },
		{ "adjustments", kDefaultAdjustment }
	};

	bool TsvReader::Read() {
		IFileStream file;

		if (!file.Open(path.c_str())) {
			_Log("Failed to open ", path.c_str());
			return false;
		}

		char buf[REGEX_BUFFER_LEN];
		std::cmatch match;
		bool finalized = false;

		try {
			while (!file.HitEOF()) {
				file.ReadString(buf, REGEX_BUFFER_LEN, '\n', '\r');
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
			_Log("Failed to read ", path.c_str());
			return false;
		}

		return true;
	}

	void TsvReader::ReadHeader(std::string m1, std::string m2)
	{
		auto it = menuHeaderMap.find(m1.c_str());
		if (m2.size() && it != menuHeaderMap.end())
		{
			switch (it->second)
			{
			case kMenuHeaderForm:
				header.form = m2;
				break;
			case kMenuHeaderMod:
				header.mod = m2;
				break;
			case kMenuHeaderSex:
			{
				auto genderIt = genderMap.find(m2.c_str());
				header.isFemale = (genderIt != genderMap.end() ? (genderIt->second == kGenderFemale) : false);
				break;
			}
			case kMenuHeaderType:
				auto value = typeMap->find(m2.c_str());
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
				_Log("Failed to read header type: ", path.c_str());
				return false;
			}

			key = GetFormId(header.mod.c_str(), header.form.c_str());

			if (!key)
			{
				_Log("Failed to read header race or mod: ", path.c_str());
				return false;
			}
			if (header.isFemale)
				key |= 0x100000000;

			switch (header.type) {
			case kMenuTypeNodes: {
				nodeSet = &g_adjustmentManager.nodeSets[key];

				//These are defaults and will get replaced by Category Root
				if (key == 0x1D698) { //DogmeatRace
					nodeSet->rootName = BSFixedString("Dogmeat_Root");
				}
				else {
					nodeSet->rootName = BSFixedString("Root");
				}
				break;
			}
			}

			return true;
		}

		void ReadCategory(std::string m1, std::string m2) 
		{
			switch (header.type) {
			case kMenuTypeNodes:
			{
				auto it = nodeTypeMap.find(m2.c_str());
				categoryIndex = it != nodeTypeMap.end() ? it->second : 0;
				break;
			}
			case kMenuTypeRace:
			case kMenuTypeActor:
			{
				auto it = defaultAdjustmentTypeMap.find(m2.c_str());
				categoryIndex = it != defaultAdjustmentTypeMap.end() ? it->second : 0;
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
					UpdateCaseMap(key, m1.c_str());
					break;
				case kNodeTypePose:
					InsertNode(nodeSet, m1.c_str(), false);
					UpdateCaseMap(key, m1.c_str());
					break;
				case kNodeTypeRoot:
					nodeSet->rootName = BSFixedString(m1.c_str());
					break;
				case kNodeTypeCenter:
					nodeSet->center.insert(BSFixedString(m1.c_str()));
					break;
				case kNodeTypeMirror:
					nodeSet->mirror.emplace(BSFixedString(m1.c_str()), BSFixedString(m2.c_str()));
					break;
				}
				break;
			}
			case kMenuTypeRace:
				g_adjustmentManager.defaultRaceCache[key].emplace(m1);
				break;
			case kMenuTypeActor:
				g_adjustmentManager.defaultActorCache[key].emplace(m1);
				break;
			}
		}
	};

	bool LoadSafTsv(std::string path)
	{
		SafTsvReader reader(path, &safTypeMap);
		return reader.Read();
	}

	enum {
		kTransformJsonTranspose = 0,
		kTransformJsonPose,
		kTransformJsonAdjustment
	};

	void ReadTransformJson(NiTransform& transform, Json::Value& value, UInt32 version) {
		transform.pos.x = ReadJsonFloat(value, "x", 0.0f);
		transform.pos.y = ReadJsonFloat(value, "y", 0.0f);
		transform.pos.z = ReadJsonFloat(value, "z", 0.0f);

		float yaw, pitch, roll;
		yaw = ReadJsonFloat(value, "yaw", 0.0f);
		pitch = ReadJsonFloat(value, "pitch", 0.0f);
		roll = ReadJsonFloat(value, "roll", 0.0f);

		switch (version) {
		case kTransformJsonTranspose:
			MatrixFromDegree(transform.rot, yaw, pitch, roll);
			break;
		case kTransformJsonPose:
			MatrixFromPose(transform.rot, yaw, pitch, roll);
			break;
		case kTransformJsonAdjustment:
			MatrixFromEulerYPR(transform.rot, yaw * DEGREE_TO_RADIAN, pitch * DEGREE_TO_RADIAN, roll * DEGREE_TO_RADIAN);
			break;
		}

		transform.scale = ReadJsonFloat(value, "scale", 1.0f);
	}

	void WriteTransformJson(NiTransform* transform, Json::Value& value, UInt32 version) 
	{
		char buffer[FLOAT_BUFFER_LEN];

		WriteJsonFloat(value, "x", transform->pos.x, buffer, "%.06f");
		WriteJsonFloat(value, "y", transform->pos.y, buffer, "%.06f");
		WriteJsonFloat(value, "z", transform->pos.z, buffer, "%.06f");

		float yaw, pitch, roll;
		switch (version) {
		case kTransformJsonTranspose:
			MatrixToDegree(transform->rot, yaw, pitch, roll);
			break;
		case kTransformJsonPose:
			MatrixToPose(transform->rot, yaw, pitch, roll);
			break;
		case kTransformJsonAdjustment:
			MatrixToEulerYPR(transform->rot, yaw, pitch, roll);
			yaw *= RADIAN_TO_DEGREE;
			pitch *= RADIAN_TO_DEGREE;
			roll *= RADIAN_TO_DEGREE;
			break;
		}

		WriteJsonFloat(value, "yaw", yaw, buffer, "%.02f");
		WriteJsonFloat(value, "pitch", pitch, buffer, "%.02f");
		WriteJsonFloat(value, "roll", roll, buffer, "%.02f");
		WriteJsonFloat(value, "scale", transform->scale, buffer, "%.06f");
	}

	/*
	* v0 was just a simple key->transform map
	* The wrong conversion from euler to matrix was being used so we need to add versioning for legacy support
	*
	* v1 header/version/name added
	*/

	bool SaveAdjustmentFile(const char* filename, std::shared_ptr<Adjustment> adjustment) 
	{
		Json::Value value;
		value["version"] = 1;

		std::filesystem::path filepath(filename);
		value["name"] = filepath.stem().string();

		Json::Value transforms(Json::ValueType::objectValue);

		adjustment->ForEachTransform([&](const NodeKey* nodeKey, NiTransform* transform) {
			if (!TransformIsDefault(*transform)) {
				Json::Value member;
				WriteTransformJson(transform, member, kTransformJsonAdjustment);
				transforms[g_adjustmentManager.GetNodeKeyName(*nodeKey)] = member;
			}
		});

		value["transforms"] = transforms;

		std::string path = GetPathWithExtension(ADJUSTMENTS_PATH, filename, ".json");

		return WriteJsonFile(path.c_str(), value);
	}

	bool LoadAdjustmentFile(const char* filename, LoadedAdjustment* adjustment)
	{
		std::string path = GetPathWithExtension(ADJUSTMENTS_PATH, filename, ".json");

		return LoadAdjustmentPath(path.c_str(), adjustment);
	}

	bool LoadAdjustmentPath(const char* path, LoadedAdjustment* adjustment) 
	{
		Json::Value value;
		if (!ReadJsonFile(path, value)) {
			adjustment->updateType = kAdjustmentUpdateNone;
			return false;
		}

		//If no version treat as simple key->transform map, else get map from transforms property
		UInt32 version = value.get("version", 0).asUInt();

		//if prior to version 1 it needs to be updated
		adjustment->updateType = version < 1 ? kAdjustmentUpdateFile : kAdjustmentUpdateNone;

		//get transforms, or just use itself
		Json::Value transforms = value.get("transforms", value);

		for (auto it = transforms.begin(); it != transforms.end(); ++it) {
			NiTransform transform;
			//read version 2 if non zero version
			ReadTransformJson(transform, *it, (version > 0 ? kTransformJsonPose : kTransformJsonTranspose));
			NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(it.key().asCString());
			if (nodeKey.key) {
				adjustment->map->emplace(nodeKey, transform);
			}
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

	bool SavePoseFile(const char* filename, TransformMap* poseMap, const char* skeleton) 
	{
		Json::Value value;

		value["version"] = 2;
		value["skeleton"] = (skeleton ? skeleton : "Vanilla");

		std::filesystem::path filepath(filename);
		value["name"] = filepath.stem().string();

		Json::Value transforms(Json::ValueType::objectValue);

		for (auto& kvp : *poseMap) {
			Json::Value member;
			WriteTransformJson(&kvp.second, member, kTransformJsonPose);
			transforms[g_adjustmentManager.GetNodeKeyName(kvp.first)] = member;
		}

		value["transforms"] = transforms;

		std::string path = GetPathWithExtension(POSES_PATH, filename, ".json");

		return WriteJsonFile(path.c_str(), value);
	}

	bool LoadPosePath(const char* path, TransformMap* poseMap) 
	{
		Json::Value value;
		if (!ReadJsonFile(path, value))
			return false;

		//If no version treat as simple key->transform map, else get map from transforms property
		UInt32 version = value.get("version", 0).asUInt();

		//get transforms, or just use itself
		Json::Value transforms = value.get("transforms", value);

		for (auto it = transforms.begin(); it != transforms.end(); ++it) {
			NiTransform transform;
			ReadTransformJson(transform, *it, version > 0 ? kTransformJsonPose : kTransformJsonTranspose);
			NodeKey nodeKey = g_adjustmentManager.GetNodeKeyFromString(it.key().asCString());
			if (nodeKey.key) {
				poseMap->emplace(nodeKey, transform);
			}
		}

		return true;
	}

	bool LoadPoseFile(const char* filename, TransformMap* poseMap)
	{
		std::string path = GetPathWithExtension(POSES_PATH, filename, ".json");

		return LoadPosePath(path.c_str(), poseMap);
	}

	bool SaveOutfitStudioXml(const char* path, TransformMap* poseMap)
	{
		IFileStream file;

		IFileStream::MakeAllDirs(path);
		if (!file.Create(path)) {
			_Log("Failed to create outfit studio xml: ", path);
			return false;
		}

		std::string name = std::filesystem::path(path).stem().string();

		xml_document<> doc;
		xml_node<>* declaration = doc.allocate_node(node_declaration);
		declaration->append_attribute(doc.allocate_attribute("version", "1.0"));
		declaration->append_attribute(doc.allocate_attribute("encoding", "UTF-8"));
		doc.append_node(declaration);
		xml_node<>* poseData = doc.allocate_node(node_element, "PoseData");
		doc.append_node(poseData);
		xml_node<>* pose = doc.allocate_node(node_element, "Pose");
		pose->append_attribute(doc.allocate_attribute("name", name.c_str()));
		poseData->append_node(pose);

		for (auto& node : *poseMap) {
			xml_node<>* bone = doc.allocate_node(node_element, "Bone");
			bone->append_attribute(doc.allocate_attribute("name", doc.allocate_string(g_adjustmentManager.GetNodeKeyName(node.first).c_str())));

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
			_Log("Failed to write to outfit studio xml: ", path);
			return false;
		}

		return true;
	}

	bool SaveSettingsFile(const char* path)
	{
		Json::Value value;

		g_adjustmentManager.settings.ToJson(value);

		bool result = WriteJsonFile(path, value);

		if (!result)
			_Log("Failed to save settings file: ", path);

		return result;
	}

	bool LoadSettingsFile(const char* path)
	{
		//if settings doesn't exist generate it
		if (!std::filesystem::exists(path)) {
			g_adjustmentManager.settings.Initialize();
			SaveSettingsFile(path);
			return false;
		}

		Json::Value value;
		if (!ReadJsonFile(path, value)) {
			g_adjustmentManager.settings.Initialize();
			return false;
		}

		g_adjustmentManager.settings.FromJson(value);

		//new settings might have been added so resave the json
		SaveSettingsFile(path);

		return true;
	}

	void LoadAllFiles() {
		if (!LoadSettingsFile(SETTINGS_PATH))
			_Log("Failed to load settings file: ", SETTINGS_PATH);

		//node set jsons for legacy support
		for (IDirectoryIterator iter(NODEMAPS_PATH, "*.json"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadNodeSetsJson(path.c_str());
		}

		//node set text files
		for (IDirectoryIterator iter(NODEMAPS_PATH, "*.txt"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadSafTsv(path);
		}

		//default
		for (IDirectoryIterator iter(DEFAULT_PATH, "*.txt"); !iter.Done(); iter.Next())
		{
			std::string	path = iter.GetFullPath();
			LoadSafTsv(path);
		}
	}
}