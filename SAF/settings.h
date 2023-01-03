#pragma once

#include "f4se/GameTypes.h"

#include "json/json.h"

namespace SAF {
	class Settings {
	public:
		std::string offset;
		std::string pose;
		std::string aaf;
		std::string outfitStudioPosePath;
		UInt32 doppelgangerID;
		UInt32 fullDoppelgangerID;

		void Initialize();
		void ToJson(Json::Value& value);
		void FromJson(Json::Value& value);
	};
}