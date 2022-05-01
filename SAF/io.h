#pragma once

#include "common/IFileStream.h"
#include "adjustments.h"

namespace SAF {
	void ReadAll(IFileStream* file, std::string* str);
	void LoadAllFiles();
	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment);
	bool LoadAdjustmentFile(std::string filename, TransformMap* map);
	bool SavePoseFile(std::string filename, TransformMap* poseMap);
	bool LoadPoseFile(std::string filename, TransformMap* poseMap);
	bool LoadPosePath(std::string filename, TransformMap* poseMap);
}