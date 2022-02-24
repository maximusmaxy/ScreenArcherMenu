#pragma once

#include "adjustments.h"

namespace SAF {
	void LoadFiles();
	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment);
	bool LoadAdjustmentFile(std::string filename, TransformMap* map);
	bool SavePoseFile(std::string filename, TransformMap* poseMap);
	bool LoadPoseFile(std::string filename, TransformMap* poseMap);
}