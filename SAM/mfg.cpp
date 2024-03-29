#include "mfg.h"

#include "f4se/GameReferences.h"
#include "f4se/GameTypes.h"
#include "f4se/NiTypes.h"
#include "f4se_common/Relocation.h"

#include "common/IFileStream.h"

#include "SAF/hacks.h"
#include "SAF/adjustments.h"
#include "SAF/util.h"
#include "SAF/conversions.h"

#include "constants.h"
#include "sam.h"
#include "pose.h"
#include "io.h"

#include <regex>
#include <unordered_map>

RelocAddr<UInt64> faceGenAnimDataVfTable(0x2CE9C58);

std::regex mfgRegex("\\s*mfg\\s+morphs\\s+(\\d+)\\s+(\\d+).*"); //mfg morphs (n) (n)
std::regex tongueRegex("\\s*;tongue\\s+(\\d+)\\s+(\\w+)\\s+(\\S+)"); //;tongue (n) (w) (f)

//#define TONGUE_NODES_SIZE 5

float* GetMorphPointer() {
	if (!selected.refr) 
		return nullptr;

	Actor::MiddleProcess* middleProcess = ((Actor*)selected.refr)->middleProcess;
	if (!middleProcess) 
		return nullptr;

	Actor::MiddleProcess::Data08* middleProcessData = middleProcess->unk08;
	if (!middleProcessData) 
		return nullptr;

	BSFaceGenAnimationData* faceGenAnimData = (BSFaceGenAnimationData*)middleProcessData->unk3B0[3];
	if (!faceGenAnimData || faceGenAnimData->vfTable != faceGenAnimDataVfTable) 
		return nullptr;

	return faceGenAnimData->mfgMorphs;
}

void SetFaceMorph(SInt32 morphIndex, SInt32 scale, SInt32 categoryIndex)
{
	MenuCategoryList* menu = GetMenu(&selected, &morphsMenuCache);
	if (!menu) 
		return;

	float* ptr = GetMorphPointer();
	if (!ptr) 
		return;

	if (GetForceMorphUpdate() != kHackEnabled)
		SetForceMorphUpdate(true);

	if (categoryIndex < 0 || categoryIndex >= menu->size())
		return;

	auto& category = (*menu)[categoryIndex];

	if (morphIndex < 0 || morphIndex >= category.second.size())
		return;

	UInt32 key = StringToUInt32(category.second[morphIndex].first.c_str());

	ptr[key] = scale * 0.0099999998f;

	//update blink hack if left/right upper eyelid morph
	if (key == 18 || key == 41) {
		if (GetBlinkState() != kHackEnabled)
			SetBlinkState(true);
	}
}

enum {
	kTonguePropX,
	kTonguePropY,
	kTonguePropZ,
	kTonguePropYaw,
	kTonguePropPitch,
	kTonguePropRoll,
	kTonguePropScale
};

SAF::InsensitiveUInt32Map tonguePropertyMap = {
	{"x", kTonguePropX},
	{"y", kTonguePropY},
	{"z", kTonguePropZ},
	{"yaw", kTonguePropYaw},
	{"pitch", kTonguePropPitch},
	{"roll", kTonguePropRoll},
	{"scale", kTonguePropScale}
};

//need an intermediary type to get yaw/pitch/roll because i can't assume they will be in the text file
struct TongueTransform {
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	float yaw = 0.0f;
	float pitch = 0.0f;
	float roll = 0.0f;
	float scale = 1.0f;
};

MenuList* FindFirstTongueMenu(std::shared_ptr<SAF::ActorAdjustments> adjustments) {
	for (auto& tongue : tongueMenuCache) {
		if (CheckMenuHasNode(adjustments, tongue.second))
		{
			return &tongue.second;
		}
	}

	return nullptr;
}

//SAF::NodeKey GetTongueNodeKey(int i)
//{
//	if (!selected.refr || i < 0 || i >= TONGUE_NODES_SIZE)
//		return SAF::NodeKey();
//
//	std::shared_ptr<SAF::ActorAdjustments> adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
//	if (!adjustments)
//		return SAF::NodeKey();
//
//	//Find the first available tongue category
//	auto tongueMenu = FindFirstTongueMenu(adjustments);
//	if (!tongueMenu)
//		return SAF::NodeKey();
//
//	return SAF::NodeKey((*tongueMenu)[i].first.c_str(), false);
//}

void SaveMfg(const char* filename) {
	float* ptr = GetMorphPointer();
	if (!ptr) return;

	IFileStream file;

	std::string path = GetPathWithExtension(FACEMORPHS_PATH, filename, ".txt");

	IFileStream::MakeAllDirs(path.c_str());

	if (!file.Create(path.c_str())) {
		_DMESSAGE("Failed to create file");
		return;
	}

	std::stringstream ss;

	for (int i = 0; i < MORPH_MAX; ++i) {
		UInt32 scale = std::round(ptr[i] * 100);
		scale = max(0, min(100, scale));
		if (scale != 0) {
			ss << "mfg morphs " << i << " " << scale << std::endl;
		}
	}

	//Get tongue transforms
	SAF::ActorAdjustmentsPtr adjustments;
	if (GetActorAdjustments(&adjustments) && adjustments->baseMap) {
		UInt32 tongueHandle = adjustments->GetHandleByType(SAF::kAdjustmentTypeTongue);
		auto adjustment = adjustments->GetAdjustment(tongueHandle);
		if (adjustment) {

			auto tongueMenu = FindFirstTongueMenu(adjustments);

			if (tongueMenu) {
				for (int i = 0; i < tongueMenu->size(); ++i) {

					SAF::NodeKey nodeKey = SAF::NodeKey((*tongueMenu)[i].first.c_str(), false);

					//Save as pose instead of adjustment to prevent errors caused by underlying animation
					NiTransform* transform = adjustment->GetTransform(nodeKey);

					if (transform && !SAF::TransformIsDefault(*transform)) {
						NiTransform* baseNode = SAF::GetFromBaseMap(*adjustments->baseMap, nodeKey.name);
						if (baseNode) {

							NiTransform result = SAF::MultiplyNiTransform(*baseNode, *transform);

							ss << ";tongue " << i << " x " << result.pos.x << std::endl;
							ss << ";tongue " << i << " y " << result.pos.y << std::endl;
							ss << ";tongue " << i << " z " << result.pos.z << std::endl;

							float yaw, pitch, roll;
							SAF::MatrixToEulerYPR(result.rot, yaw, pitch, roll);
							ss << ";tongue " << i << " yaw " << yaw * SAF::RADIAN_TO_DEGREE << std::endl;
							ss << ";tongue " << i << " pitch " << pitch * SAF::RADIAN_TO_DEGREE << std::endl;
							ss << ";tongue " << i << " roll " << roll * SAF::RADIAN_TO_DEGREE << std::endl;

							ss << ";tongue " << i << " scale " << result.scale << std::endl;
						}
					}
				}
			}
		}
	}

	file.WriteString(ss.str().c_str());
	file.Close();
}

bool LoadMfgFile(const char* filename) 
{
	std::string path = GetPathWithExtension(FACEMORPHS_PATH, filename, ".txt");

	return LoadMfgPath(path.c_str());
}

bool LoadMfgPath(const char* path) {
	float* ptr = GetMorphPointer();
	if (!ptr) return false;

	IFileStream file;

	if (!file.Open(path)) {
		_Log("Failed to open ", path);
		return false;
	}

	char buf[512];
	std::cmatch match;

	float morphs[MORPH_MAX];
	std::memset(morphs, 0, sizeof(morphs));

	bool blinkHack = false;
	bool morphHack = false;

	TongueTransform tongues[5];

	while (!file.HitEOF()) {
		file.ReadString(buf, 512, '\n', '\r');
		if (std::regex_match(buf, match, mfgRegex)) {
			try {
				int id = max(0, min(MORPH_MAX - 1, std::stoi(match[1].str())));
				int scale = max(0, min(100, std::stoi(match[2].str())));

				morphs[id] = scale * 0.0099999998f;

				//check if blink hack needs to be applied to left/right upper eye lid morph
				if (id == 18 || id == 41)
					blinkHack = true;

				//aways update morph hack
				morphHack = true;
			}
			catch (...) {
				_DMESSAGE("Failed to read morph id or value");
			}
		} 
		//get tongue properties
		else if (std::regex_match(buf, match, tongueRegex)) {
			try {

				int id = std::stoi(match[1].str());
				if (id >= 0 && id < 5) {

					auto tongueProperty = tonguePropertyMap.find(match[2].str().c_str());
					if (tongueProperty != tonguePropertyMap.end()) 
					{
						float value = std::stof(match[3].str());

						switch (tongueProperty->second) {
						case kTonguePropX: tongues[id].x = value; break;
						case kTonguePropY: tongues[id].y = value; break;
						case kTonguePropZ: tongues[id].z = value; break;
						case kTonguePropYaw: tongues[id].yaw = value; break;
						case kTonguePropPitch: tongues[id].pitch = value; break;
						case kTonguePropRoll: tongues[id].roll = value; break;
						case kTonguePropScale: tongues[id].scale = value; break;
						}
					}
				}
			}
			catch (...) {
				_DMESSAGE("Failed to read face morphs tongue property");
			}
		}
	}

	file.Close();

	//set the morphs
	for (int i = 0; i < MORPH_MAX; ++i) {
		ptr[i] = morphs[i];
	}

	//build the tongue transform map
	SAF::TransformMap transformMap;

	SAF::ActorAdjustmentsPtr adjustments = nullptr;
	if (GetActorAdjustments(&adjustments)) {
		auto tongueMenu = FindFirstTongueMenu(adjustments);
		if (tongueMenu) {
			for (int i = 0; i < tongueMenu->size(); ++i) {
				NiTransform transform;
				transform.pos = NiPoint3(tongues[i].x, tongues[i].y, tongues[i].z);
				SAF::MatrixFromEulerYPR(transform.rot,
					tongues[i].yaw * SAF::DEGREE_TO_RADIAN, tongues[i].pitch * SAF::DEGREE_TO_RADIAN, tongues[i].roll * SAF::DEGREE_TO_RADIAN);
				transform.scale = tongues[i].scale;

				SAF::NodeKey nodeKey = SAF::NodeKey((*tongueMenu)[i].first.c_str(), false);
				
				transformMap.emplace(nodeKey, transform);
			}
		}
	}

	if (adjustments)
		saf->LoadTongueAdjustment(adjustments, &transformMap);

	if (blinkHack && GetBlinkState() != kHackEnabled)
		SetBlinkState(true);

	if (morphHack && GetForceMorphUpdate() != kHackEnabled)
		SetForceMorphUpdate(true);

	return true;
}

void ResetMfg() {
	float* ptr = GetMorphPointer();
	if (!ptr) 
		return;

	memset(ptr, 0, sizeof(float) * MORPH_MAX);

	//send a nullptr to clear the tongue adjustment
	SAF::ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	saf->LoadTongueAdjustment(adjustments, nullptr);
}

UInt32 GetOrCreateTongueHandle(SAF::ActorAdjustmentsPtr adjustments) {
	//if no tongue handle, create
	UInt32 tongueHandle = adjustments->GetHandleByType(SAF::kAdjustmentTypeTongue);

	if (!tongueHandle) {
		tongueHandle = saf->CreateAdjustment(adjustments, "Face Morphs Tongue", SAM_ESP);

		auto adjustment = adjustments->GetAdjustment(tongueHandle);
		if (!adjustment)
			return 0;

		adjustment->type = SAF::kAdjustmentTypeTongue;
	}

	return tongueHandle;
}

void GetMorphCategories(GFxResult& result)
{
	result.CreateMenuItems();

	MenuCategoryList* menu = GetMenu(&selected, &morphsMenuCache);
	if (!menu)
		return result.SetError("Could not get morph information for targeted race");

	for (SInt32 i = 0; i < menu->size(); ++i) {
		result.PushItem(menu->at(i).first.c_str(), i);
	}

	SAF::ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	//TODO: We're just taking the first available for now
	MenuList* tongueMenu = FindFirstTongueMenu(adjustments);

	if (tongueMenu)
		result.PushItem(TONGUE_BONES_MENU, (SInt32)0);
}

void SetFaceMorphCategory(GFxResult& result, SInt32 index, UInt32 value)
{
	MenuCategoryList* menu = GetMenu(&selected, &morphsMenuCache);
	if (!menu)
		return result.SetError("Could not get morph information for targeted race");

	if (index < menu->size())
		samManager.PushMenu("FaceMorphSliders");
	else {
		SAF::ActorAdjustmentsPtr adjustments;
		if (!GetActorAdjustments(&adjustments))
			return;

		auto tongueHandle = GetOrCreateTongueHandle(adjustments);
		if (!tongueHandle)
			return result.SetError("Failed to create tongue adjustment");

		GFxValue tongeHandleValue(tongueHandle);
		samManager.SetLocal("adjustmentHandle", &tongeHandleValue);
		samManager.PushMenu("TongueBones");
	}
}

void GetMorphs(GFxResult& result, SInt32 categoryIndex)
{
	result.CreateMenuItems();

	MenuCategoryList* menu = GetMenu(&selected, &morphsMenuCache);
	if (!menu || categoryIndex >= menu->size()) 
		return;

	float* ptr = GetMorphPointer();
	if (!ptr) 
		return;

	for (auto& kvp : (*menu)[categoryIndex].second) {
		UInt32 key = StringToUInt32(kvp.first.c_str());
		key = max(0, min(MORPH_MAX - 1, key));

		SInt32 rounded = std::round(ptr[key] * 100);
		result.PushItem(kvp.second.c_str(), rounded);
	}
}

void GetMorphsTongueNodes(GFxResult& result)
{
	std::shared_ptr<SAF::ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(SKELETON_ERROR);

	//TODO: We're just taking the first available for now
	auto tongueMenu = FindFirstTongueMenu(adjustments);

	if (!tongueMenu)
		return result.SetError(TONGUE_BONES_ERROR);

	result.CreateMenuItems();

	for (auto& kvp : *tongueMenu) {
		if (adjustments->HasNode(kvp.first.c_str())) {
			result.PushItem(kvp.second.c_str(), kvp.first.c_str());
		}
	}
}