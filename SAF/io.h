#pragma once

#include "common/IFileStream.h"
#include "adjustments.h"

#include "json/json.h"

#include <regex>

#define WriteJsonFloat(K, V, D) sprintf_s(buffer, D, V); \
K = Json::Value(buffer);

extern std::regex tabSeperatedRegex;
extern std::regex tabOptionalRegex;

typedef std::vector<std::pair<std::string, std::string>> MenuList;
typedef std::vector<std::pair<std::string, MenuList>> MenuCategoryList;
typedef std::unordered_map<UInt64, MenuCategoryList> MenuCache;

struct MenuHeader
{
	std::string race;
	std::string mod;
	bool isFemale = false;
	UInt32 type = 0;
};

enum {
	kMenuHeaderRace = 0,
	kMenuHeaderMod,
	kMenuHeaderSex,
	kMenuHeaderType
};

extern std::unordered_map<std::string, UInt32> menuHeaderMap;

namespace SAF {

	void ReadAll(IFileStream* file, std::string* str);
	float ReadJsonFloat(Json::Value& value);
	void LoadAllFiles();
	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment);
	bool LoadAdjustmentFile(std::string filename, LoadedAdjustment* map);
	bool SavePoseFile(std::string filename, TransformMap* poseMap, const char* skeleton);
	bool LoadPoseFile(std::string filename, TransformMap* poseMap);
	bool LoadPosePath(std::string filename, TransformMap* poseMap);
	bool SaveOutfitStudioXml(std::string name, TransformMap* poseMap);
}