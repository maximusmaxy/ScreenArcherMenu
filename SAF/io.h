#pragma once

#include "adjustments.h"

namespace SAF {
	void LoadFiles();
	bool SaveAdjustmentFile(std::string filename, std::shared_ptr<Adjustment> adjustment);
	bool LoadAdjustmentFile(std::string filename, NodeSet* set, std::unordered_map<std::string, NiTransform>* adjustment);
	bool SavePoseFile(std::string filename, std::shared_ptr<ActorAdjustments> adjustments);
	bool LoadPoseFile(std::string filename, std::shared_ptr<ActorAdjustments> adjustments);
}