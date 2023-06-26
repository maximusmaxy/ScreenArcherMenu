#include "settings.h"

#include "util.h"

namespace SAF {
	Settings g_settings;

	void Settings::Initialize() {
		offset = "_Offset";
		pose = "_Pose";
		aaf = "AAF.esm";
		outfitStudioPosePath = "Data\\tools\\bodyslide\\PoseData";
		doppelgangerID = 0x72E2;
		fullDoppelgangerID = GetFormId(BSFixedString(aaf.c_str()).c_str(), doppelgangerID);
	}

	void Settings::ToJson(Json::Value& value) {
		value["offset"] = offset;
		value["pose"] = pose;
		value["aaf"] = aaf.c_str();
		value["outfitstudioposepath"] = outfitStudioPosePath;
		value["doppelgangerID"] = Json::Value::UInt(doppelgangerID);
	}

	void Settings::FromJson(Json::Value& value) {
		offset = value.get("offset", "_Offset").asString();
		pose = value.get("pose", "_Pose").asString();
		aaf = BSFixedString(value.get("aafName", "AAF.esm").asCString());
		outfitStudioPosePath = value.get("outfitstudioposepath", "Data\\tools\\bodyslide\\PoseData").asString();
		doppelgangerID = value.get("doppelgangerID", 0x72E2).asUInt();
		fullDoppelgangerID = GetFormId(aaf.c_str(), doppelgangerID);
	}
}