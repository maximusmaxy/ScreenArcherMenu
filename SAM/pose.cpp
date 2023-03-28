#include "pose.h"

#include "common/IDirectoryIterator.h"

#include "f4se/NiTypes.h"

#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>

#include "constants.h"
#include "sam.h"
#include "idle.h"
#include "types.h"
#include "io.h"

#include "SAF/conversions.h"
#include "SAF/types.h"
using namespace SAF;

std::string selectedNode;
std::unordered_map<UInt32, std::string> lastSelectedPose;

bool GetActorAdjustments(ActorAdjustmentsPtr* adjustments)
{
	if (!selected.refr)
		return false;

	*adjustments = saf->GetActorAdjustments(selected.refr->formID);
	if (!*adjustments)
		return false;

	return true;
}

bool GetAdjustment(UInt32 handle, AdjustmentPtr* adjustment)
{
	if (!selected.refr)
		return false;

	auto actorAdjustments = saf->GetActorAdjustments(selected.refr->formID);
	if (!actorAdjustments)
		return false;

	*adjustment = actorAdjustments->GetAdjustment(handle);
	if (!*adjustment)
		return false;

	return true;
}

bool GetActorAndAdjustment(UInt32 handle, ActorAdjustmentsPtr* adjustments, AdjustmentPtr* adjustment)
{
	if (!selected.refr)
		return false;

	*adjustments = saf->GetActorAdjustments(selected.refr->formID);
	if (!*adjustments)
		return false;

	*adjustment = (*adjustments)->GetAdjustment(handle);
	if (!*adjustment)
		return false;

	return true;
}

NodeKey GetActorNodeKey(ActorAdjustmentsPtr adjustments, const char* key)
{
	NodeKey nodeKey = saf->manager->GetNodeKeyFromString(key);

	//If offset only, force offset to true
	if (adjustments->IsNodeOffset(nodeKey))
		nodeKey.SetOffset(true);
	
	return nodeKey;
}

bool GetActorAdjustmentAndNodeKey(const char* key, UInt32 handle, ActorAdjustmentsPtr* adjustments, AdjustmentPtr* adjustment, NodeKey* nodeKey)
{
	if (!selected.refr)
		return false;

	*adjustments = saf->GetActorAdjustments(selected.refr->formID);
	if (!*adjustments)
		return false;

	*adjustment = (*adjustments)->GetAdjustment(handle);
	if (!*adjustment)
		return false;

	*nodeKey = GetActorNodeKey(*adjustments, key);
	if (!nodeKey->key)
		return false;

	return true;
}

enum {
	kBoneTransformYaw = 0,
	kBoneTransformPitch,
	kBoneTransformRoll,
	kBoneTransformX,
	kBoneTransformY,
	kBoneTransformZ,
	kBoneTransformScale,
	kBoneTransformTouchYaw,
	kBoneTransformTouchPitch,
	kBoneTransformTouchRoll
};

void SetBoneTransform(GFxResult& result, SInt32 index, float value, UInt32 adjustmentHandle, const char* key, float yaw, float pitch, float roll) {
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, adjustmentHandle, &adjustments, &adjustment, &nodeKey))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	auto transform = adjustment->GetTransformOrDefault(nodeKey);

	switch (index) {
	case kBoneTransformYaw:
	case kBoneTransformPitch:
	case kBoneTransformRoll:
		SAF::MatrixFromEulerYPR(transform.rot, yaw * DEGREE_TO_RADIAN, pitch * DEGREE_TO_RADIAN, roll * DEGREE_TO_RADIAN);
		break;
	case kBoneTransformX: transform.pos.x = value; break;
	case kBoneTransformY: transform.pos.y = value; break;
	case kBoneTransformZ: transform.pos.z = value; break;
	case kBoneTransformScale: transform.scale = value; break;
	}

	saf->SetTransform(adjustment, nodeKey, transform);
	adjustments->UpdateNode(nodeKey.name);
}

void ResetAdjustmentTransform(const char* key, UInt32 handle) {
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, handle, &adjustments, &adjustment, &nodeKey))
		return;

	StartBoneEdit(handle, key);
	saf->SetTransform(adjustment, nodeKey, SAF::TransformIdentity());
	EndBoneEdit(handle, key);
	adjustments->UpdateNode(nodeKey.name);
}

void NegateTransform(const char* key, UInt32 handle) {
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, handle, &adjustments, &adjustment, &nodeKey))
		return;

	StartBoneEdit(handle, key);
	saf->NegateTransform(adjustments, adjustment, nodeKey);
	EndBoneEdit(handle, key);
	adjustments->UpdateNode(nodeKey.name);
}

void SaveAdjustmentFile(GFxResult& result, const char* filename, int handle) {
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	if (!saf->SaveAdjustment(adjustments, handle, filename))
		return result.SetError("Failed to save adjustment");

	adjustments->UpdateAllAdjustments();
	samManager.SetLocal("adjustmentName", &GFxValue(filename));
}

bool LoadAdjustmentFile(const char* filename) 
{
	std::string path = GetPathWithExtension(ADJUSTMENTS_PATH, filename, ".json");

	return LoadAdjustmentPath(path.c_str());
}

bool LoadAdjustmentPath(const char* path) 
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return false;

	UInt32 result = saf->LoadAdjustment(adjustments, path);
	if (!result)
		return false;

	adjustments->UpdateAllAdjustments();
	return true;
}

void LoadAdjustmentPathGFx(GFxResult& result, const char* path)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(SKELETON_ERROR);

	UInt32 handle = saf->LoadAdjustment(adjustments, path);
	if (!handle)
		return result.SetError("Failed to load adjustment");

	adjustments->UpdateAllAdjustments();
}

void PushNewAdjustment(const char* name) {
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	saf->CreateAdjustment(adjustments, name, SAM_ESP);
}

void EraseAdjustment(UInt32 handle) {
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	saf->RemoveAdjustment(adjustments, handle);
	adjustments->UpdateAllAdjustments();
}

void ClearAdjustment(UInt32 handle) {
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAndAdjustment(handle, &adjustments, &adjustment))
		return;

	saf->ResetAdjustment(adjustment);
	adjustments->UpdateAllAdjustments();
}

void GetAdjustmentNegate(GFxResult& result)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	MenuCategoryList* groupsMenu = GetMenu(&selected, &groupsMenuCache);
	if (!groupsMenu)
		return result.SetError("Could not find negation groups for targeted actor");

	result.CreateNames();

	for (auto it = groupsMenu->begin(); it != groupsMenu->end(); ++it) {
		result.PushName(it->first.c_str());
	}
}

void SetAdjustmentNegate(GFxResult& result, const char* adjustmentGroup, UInt32 handle)
{
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAndAdjustment(handle, &adjustments, &adjustment))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	MenuCategoryList* groupsMenu = GetMenu(&selected, &groupsMenuCache);
	if (!groupsMenu)
		return result.SetError("Could not find negation groups for targeted actor");

	//find the correct adjustment group and negate all transforms
	for (auto it = groupsMenu->begin(); it < groupsMenu->end(); it++) {
		if (it->first == adjustmentGroup) {
			for (auto& kvp : it->second) {
				saf->NegateTransform(adjustments, adjustment, NodeKey(BSFixedString(kvp.first.c_str()), false));
			}
		}
	}

	adjustments->UpdateAllAdjustments();
}

bool ShiftAdjustment(UInt32 handle, bool increment)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return false;

	UInt32 fromIndex = adjustments->GetAdjustmentIndex(handle);
	if (fromIndex == -1) 
		return false;

	UInt32 toIndex = fromIndex + (increment ? 1 : -1);

	UInt32 handleResult = saf->MoveAdjustment(adjustments, fromIndex, toIndex);
	if (!handleResult)
		return false;
	
	adjustments->UpdateAllAdjustments();
	return true;
}

void SetAdjustmentName(GFxResult& result, UInt32 handle, const char* name)
{
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAndAdjustment(handle, &adjustments, &adjustment))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	saf->RenameAdjustment(adjustment, name);
	samManager.SetLocal("adjustmentName", &GFxValue(adjustment->name.c_str()));
}

void SetLocalAdjustmentName(UInt32 handle)
{
	AdjustmentPtr adjustment;
	if (!GetAdjustment(handle, &adjustment))
		return;

	samManager.SetLocal("adjustmentName", &GFxValue(adjustment->name.c_str()));
}

void MergeAdjustment(GFxResult& result, UInt32 adjustmentHandle)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	saf->MergeAdjustmentDown(adjustments, adjustmentHandle);
}

//void MirrorAdjustment(GFxResult& result, UInt32 adjustmentHandle)
//{
//	ActorAdjustmentsPtr adjustments;
//	AdjustmentPtr adjustment;
//	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment))
//		return result.SetError(ADJUSTMENT_MISSING_ERROR);
//
//	if (adjustments->nodeSets->mirror.empty())
//		return result.SetError("Missing mirror nodeset for targeted race");
//
//	safDispatcher.MirrorAdjustment(selected.refr->formID, adjustmentHandle);
//}

bool NiAVObjectVisitAll(NiAVObject* root, const std::function<bool(NiAVObject*)>& functor)
{
	if (functor(root))
		return true;

	NiPointer<NiNode> node(root->GetAsNiNode());
	if (node) {
		for (UInt32 i = 0; i < node->m_children.m_emptyRunStart; i++) {
			NiPointer<NiAVObject> object(node->m_children.m_data[i]);
			if (object) {
				NiAVObjectVisitAll(object, functor);
			}
		}
	}

	return false;
}

void GetAdjustmentScale(GFxResult& result, UInt32 adjustmentHandle)
{
	ActorAdjustmentsPtr adjustments;
	AdjustmentPtr adjustment;
	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateValues();
	result.PushValue((SInt32)std::round(adjustment->scale * 100));
}

void SetAdjustmentScale(GFxResult& result, UInt32 adjustmentHandle, int scale)
{
	ActorAdjustmentsPtr adjustments;
	AdjustmentPtr adjustment;
	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	adjustment->SetScale(scale * 0.01);
	adjustments->UpdateAllAdjustments(adjustment);
}

MenuCategoryList* GetAdjustmentMenu()
{
	MenuCategoryList* menu = GetMenu(&selected, &poseMenuCache);

	if (menu) 
		return menu;

	//If for some reason the human menu can't be found dump it all into one menu
	ActorAdjustmentsPtr adjustments = saf->GetActorAdjustments(selected.refr->formID);

	if (!adjustments || !adjustments->nodeSets) return nullptr;
	
	MenuList list;
	for (auto& nodeKey : adjustments->nodeSets->all) {
		std::string name = saf->manager->GetNodeKeyName(nodeKey);
		list.push_back(std::make_pair(name, name));
	}

	MenuCategoryList categories;
	categories.push_back(std::make_pair("All", list));

	poseMenuCache[selected.race] = categories;

	return &poseMenuCache[selected.race];
}

void GetAdjustments(GFxResult& result)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(SKELETON_ERROR);

	result.CreateMenuItems();

	adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
		result.PushItem(adjustment->name.c_str(), adjustment->handle);
	});
}

bool CheckMenuHasNode(ActorAdjustmentsPtr adjustments, MenuList& list)
{
	for (auto& kvp : list) {
		if (adjustments->HasNode(kvp.first.c_str())) 
			return true;
	}
	return false;
}

void GetBoneCategories(GFxResult& result)
{
	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) {
		result.SetError("Adjustment category could not be found");
		return;
	}

	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateMenuItems();

	int size = categories->size();

	for (SInt32 i = 0; i < size; ++i)
	{
		if (CheckMenuHasNode(adjustments, (*categories)[i].second)) {
			result.PushItem((*categories)[i].first.c_str(), i);
		}
	}
}

void GetBoneNames(GFxResult& result, SInt32 categoryIndex)
{
	MenuCategoryList* categories = GetAdjustmentMenu();
	if (!categories || categoryIndex >= categories->size())
		return result.SetError("Adjustment bones could not be found");
		
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateMenuItems();

	auto& category = (*categories)[categoryIndex].second;

	for (auto& kvp : category) {
		if (adjustments->HasNode(kvp.first.c_str())) {
			result.PushItem(kvp.second.c_str(), kvp.first.c_str());
		}
	}
}

void PushBackTransformGFx(GFxValue* result, NiTransform& transform) {
	float yaw, pitch, roll;
	//MatrixToDegree(transform.rot, yaw, pitch, roll);
	//MatrixToEulerRPY(transform.rot, roll, pitch, yaw);
	MatrixToEulerYPR(transform.rot, yaw, pitch, roll);

	GFxValue rx(yaw * RADIAN_TO_DEGREE);
	GFxValue ry(pitch * RADIAN_TO_DEGREE);
	GFxValue rz(roll * RADIAN_TO_DEGREE);
	result->PushBack(&rx);
	result->PushBack(&ry);
	result->PushBack(&rz);

	GFxValue px(transform.pos.x);
	GFxValue py(transform.pos.y);
	GFxValue pz(transform.pos.z);
	result->PushBack(&px);
	result->PushBack(&py);
	result->PushBack(&pz);

	GFxValue scale(transform.scale);
	result->PushBack(&scale);
}

void GetBoneTransform(GFxResult& result, const char* key, int handle) 
{
	ActorAdjustmentsPtr adjustments;
	AdjustmentPtr adjustment;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, handle, &adjustments, &adjustment, &nodeKey)) 
		result.SetError(ADJUSTMENT_MISSING_ERROR);

	NiTransform transform = adjustment->GetTransformOrDefault(nodeKey);

	result.CreateValues();
	PushBackTransformGFx(&result.params[1], transform);
}

void GetPoseAdjustments(GFxResult& result)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateMenuItems();

	adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
		result.PushItem(adjustment->name.c_str(), adjustment->file.empty());
	});
}

void SaveJsonPose(const char* filename, GFxValue& checkedAdjustments, int exportType)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	auto menu = GetMenu(&selected, &exportMenuCache);
	if (!menu)
		return;

	//+2 for All/Outfit Studio
	if (exportType < 0 || exportType > menu->size() + 1)
		return;

	ExportSkeleton exports;

	int i = 0;

	//TODO This will get the wrong adjustments if they change while this menu is open, maybe store handles in the menu items
	adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
		GFxValue checked;
		checkedAdjustments.GetElement(i, &checked);
		if (checked.GetBool()) {
			exports.handles.insert(adjustment->handle);
		}
		i++;
	});

	//menu size +1 = Outfit Studio Xml
	if (exportType == menu->size() + 1) 
	{
		std::string osPath = GetPathWithExtension(saf->manager->settings.outfitStudioPosePath.c_str(), filename, ".xml");
		saf->SaveOSPose(adjustments, osPath.c_str(), &exports);
		return;
	}

	NodeSet nodeSet;

	if (exportType < menu->size()) 
	{
		for (auto& node : (*menu)[exportType].second) {
			nodeSet.insert(NodeKey(BSFixedString(node.first.c_str()), false));
		}
		exports.nodes = &nodeSet;
		exports.skeleton = (*menu)[exportType].first.c_str();
	}
	else {
		//Will be assigned all nodes if null
		exports.nodes = nullptr;

		//Get the first skeleton or Vanilla if none exist
		if (menu->size() < 3) {
			exports.skeleton = "Vanilla";
		}
		else {
			exports.skeleton = menu->front().first.c_str();
		}
	}

	std::filesystem::path exportPath("Exports");
	exportPath.append(filename);

	saf->SavePose(adjustments, exportPath.string().c_str(), &exports);
}

bool LoadPoseFile(const char* filename)
{
	std::string path = GetPathWithExtension(POSES_PATH, filename, ".json");

	return LoadPosePath(path.c_str());
}

bool LoadPosePath(const char* path)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return false;

	UInt32 resultHandle = saf->LoadPose(adjustments, path);
	if (!resultHandle)
		return false;

	adjustments->UpdateAllAdjustments();

	lastSelectedPose[selected.refr->formID] = GetRelativePath(constStrLen(POSES_PATH), constStrLen(".json"), path);
	return true;
}

void LoadPoseGFx(GFxResult& result, const char* path) {
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(SKELETON_ERROR);

	UInt32 resultHandle = saf->LoadPose(adjustments, path);
	if (!resultHandle)
		return result.SetError("Failed to load pose");

	adjustments->UpdateAllAdjustments();

	lastSelectedPose[selected.refr->formID] = GetRelativePath(constStrLen(POSES_PATH), constStrLen(".json"), path);

	std::string stem = std::filesystem::path(path).stem().string();
	samManager.ShowNotification(stem.c_str(), true);
}

void ResetJsonPose()
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	saf->ResetPose(adjustments);
	adjustments->UpdateAllAdjustments();
}

const char* GetCurrentPoseName()
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments))
		return nullptr;

	auto adjustment = adjustments->GetAdjustmentByType(kAdjustmentTypePose);
	if (!adjustment)
		return nullptr;

	return adjustment->name.c_str();
}

void GetSkeletonAdjustments(GFxResult& result, const char* path, const char* ext, bool race)
{
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAdjustments(&adjustments)) 
		return result.SetError(SKELETON_ERROR);

	result.CreateFolderCheckbox();

	//build a set of all race/skeleton adjustments to compare to
	InsensitiveStringSet adjustmentNames;
	if (race) {
		adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
			if (adjustment->type == kAdjustmentTypeRace) {
				adjustmentNames.insert(adjustment->file);
			}
		});
	}
	else {
		adjustments->ForEachAdjustment([&](AdjustmentPtr adjustment) {
			if (adjustment->type == kAdjustmentTypeSkeleton) {
				adjustmentNames.insert(adjustment->file);
			}
		});
	}

	NaturalSortedMap files;
	NaturalSortedMap folders;

	GetSortedFilesAndFolders(path, ".json", files, folders);

	for (auto& folder : folders) {
		result.PushFolder(folder.first.c_str(), folder.second.c_str());
	}

	for (auto& file : files) {
		std::string filePath = GetAdjustmentNameFromPath(file.second.c_str());
		bool checked = adjustmentNames.count(filePath);
		result.PushFileCheckbox(file.first.c_str(), file.second.c_str(), checked);
	}
}

void LoadSkeletonAdjustment(GFxResult& result, const char* path, bool checked, bool checkbox, bool race)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	ActorAdjustmentsPtr adjustments = saf->GetActorAdjustments(selected.refr->formID);
	if (!adjustments)
		return result.SetError(SKELETON_ERROR);

	//if race adjustment is true, send the formId instead of race
	UInt32 formId = race ? selected.race : adjustments->formId;

	saf->LoadSkeletonAdjustment(formId, selected.isFemale, path, race, !checkbox, checked);
}

void RotateAdjustmentXYZ(const char* key, int adjustmentHandle, int type, float dif) {
	ActorAdjustmentsPtr adjustments;
	AdjustmentPtr adjustment;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, adjustmentHandle, &adjustments, &adjustment, &nodeKey))
		return;

	saf->RotateTransformXYZ(adjustments, adjustment, nodeKey, type, dif);
	adjustments->UpdateNode(nodeKey.name);
}

void GetPoseExportTypes(GFxResult& result)
{
	auto menu = GetMenu(&selected, &exportMenuCache);
	if (!menu) {
		result.SetError(EXPORT_ERROR);
		return;
	}

	result.CreateMenuItems();

	for (SInt32 i = 0; i < menu->size(); ++i) {
		result.PushItem((*menu)[i].first.c_str(), i);
	}

	SInt32 allIndex = menu->size();
	SInt32 osIndex = menu->size() + 1;

	result.PushItem("All", allIndex);
	result.PushItem("Outfit Studio", osIndex);
}

//void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex)
//{
//	if (selected.refr) {
//		ActorAdjustmentsPtr adjustments = saf->GetActorAdjustments(selected.refr->formID);
//		if (adjustments != nullptr) {
//
//			MenuCategoryList* categories = GetAdjustmentMenu();
//			if (categories && categoryIndex < categories->size() && nodeIndex < (*categories)[categoryIndex].second.size()) {
//
//				NodeKey nodeKey = GetActorNodeKey(adjustments, (*categories)[categoryIndex].second[nodeIndex].first.c_str());
//
//				selectedNode = safDispatcher.GetNodeKeyName(nodeKey);
//				result->SetString(selectedNode.c_str());
//			}
//		}
//	}
//}

bool GetNodeIsOffset(const char* nodeName)
{
	NodeKey nodeKey = saf->manager->GetNodeKeyFromString(nodeName);
	return nodeKey.offset;
}

const char* GetBoneInit(const char* nodeName)
{
	NodeKey nodeKey = saf->manager->GetNodeKeyFromString(nodeName);

	if (!nodeKey.key)
		return nodeName;

	if (!selected.refr)
		return nodeName;

	ActorAdjustmentsPtr adjustments = saf->manager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments)
		return nodeName;
	
	//Force offset only to false and force others to true
	nodeKey.SetOffset(adjustments->IsNodeOffset(nodeKey));
	selectedNode = saf->manager->GetNodeKeyName(nodeKey);

	return selectedNode.c_str();
}

void ToggleNodeName(GFxValue* result, const char* nodeName)
{
	if (selected.refr) {
		ActorAdjustmentsPtr adjustments = saf->GetActorAdjustments(selected.refr->formID);
		if (adjustments != nullptr) {
			NodeKey nodeKey = saf->manager->GetNodeKeyFromString(nodeName);

			if (nodeKey.key) {
				if (adjustments->IsNodeOffset(nodeKey)) {
					nodeKey.SetOffset(true);
				}
				else {
					nodeKey.SetOffset(!nodeKey.offset);
				}

				selectedNode = saf->manager->GetNodeKeyName(nodeKey);
				result->SetString(selectedNode.c_str());
				return;
			}
		}
	}
	
	//keep same node on failure
	result->SetString(nodeName);
}

//void FindNodeIndexes(NodeKey& nodeKey, SInt32* categoryIndex, SInt32* nodeIndex)
//{
//	*categoryIndex = -1;
//	*nodeIndex = -1;
//
//	auto menu = GetAdjustmentMenu();
//	if (!menu)
//		return;
//
//	for (SInt32 i = 0; i < menu->size(); ++i) {
//		for (SInt32 j = 0; j < (*menu)[i].second.size(); ++j) {
//			NodeKey menuKey = safDispatcher.GetNodeKeyFromString((*menu)[i].second[j].first.c_str());
//			if (menuKey == nodeKey) {
//				*categoryIndex = i;
//				*nodeIndex = j;
//				return;
//			}
//		}
//	}
//}

void AppendPoseFavorite(GFxResult& result)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto it = lastSelectedPose.find(selected.refr->formID);
	if (it == lastSelectedPose.end())
		return result.SetError("Current SAM pose could not be found");

	std::string inPath = GetPathWithExtension(POSES_PATH, it->second.c_str(), ".json");
	if (!std::filesystem::exists(inPath))
		return result.SetError("Current SAM pose could not be found");

	std::string stem = std::filesystem::path(it->second).stem().string();

	std::filesystem::path outPath(POSES_PATH);
	outPath.append(POSE_FAVORITES);
	outPath.append(stem);
	outPath.concat(".json");

	if (std::filesystem::exists(outPath))
		return result.SetError("SAM pose has already been favorited");

	IFileStream::MakeAllDirs(outPath.string().c_str());

	std::filesystem::copy_file(inPath, outPath, std::filesystem::copy_options::overwrite_existing);

	std::string notif = stem + " has been favorited!";
	samManager.ShowNotification(notif.c_str(), false);
}

struct BoneEditAction {
	UInt32 handle;
	NodeKey nodeKey;
	NiTransform src;
	NiTransform dst;
};

struct BoneEditManager {
	SInt32 index;
	BoneEditAction stored;
	std::vector<BoneEditAction> actions;

	BoneEditManager() : index(-1) {}
};

std::unique_ptr<BoneEditManager> boneEdits = nullptr;

void ClearBoneEdit(GFxResult& result)
{
	boneEdits.reset(new BoneEditManager());
}

void StartBoneEdit(UInt32 handle, const char* key)
{
	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, handle, &adjustments, &adjustment, &nodeKey))
		return;

	boneEdits->stored = BoneEditAction{ handle, nodeKey, adjustment->GetTransformOrDefault(nodeKey) };
}

void EndBoneEdit(UInt32 handle, const char* key)
{
	//Check if the handle/nodekey are the same
	if (handle != boneEdits->stored.handle)
		return;

	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, handle, &adjustments, &adjustment, &nodeKey))
		return;

	if (nodeKey != boneEdits->stored.nodeKey)
		return;

	//check if updated
	NiTransform dst = adjustment->GetTransformOrDefault(nodeKey);
	if (SAF::TransformEqual(boneEdits->stored.src, dst))
		return;


	boneEdits->index++;
	boneEdits->actions.resize(boneEdits->index);
	boneEdits->actions.emplace_back(BoneEditAction{
		handle,
		nodeKey,
		boneEdits->stored.src,
		dst
	});
}

void UndoBoneEdit(GFxResult& result)
{
	if (boneEdits->index < 0)
		return result.SetError("");

	auto& action = boneEdits->actions.at(boneEdits->index);
	boneEdits->index--;

	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAndAdjustment(action.handle, &adjustments, &adjustment))
		return;
	
	saf->SetTransform(adjustment, action.nodeKey, action.src);
	adjustments->UpdateNode(action.nodeKey.name);
}

void RedoBoneEdit(GFxResult& result)
{
	if (boneEdits->index + 1 >= (SInt32)boneEdits->actions.size())
		return result.SetError("");

	boneEdits->index++;
	auto& action = boneEdits->actions.at(boneEdits->index);

	AdjustmentPtr adjustment;
	ActorAdjustmentsPtr adjustments;
	if (!GetActorAndAdjustment(action.handle, &adjustments, &adjustment))
		return;

	saf->SetTransform(adjustment, action.nodeKey, action.dst);
	adjustments->UpdateNode(action.nodeKey.name);
}

std::unordered_map<uintptr_t, std::vector<bool>> boneFilterCache;

std::vector<bool>* GetCachedBoneFilter(MenuCategoryList* list) {
	auto ptr = (uintptr_t)GetMenu(&selected, &filterMenuCache);
	if (!ptr)
		return nullptr;

	auto it = boneFilterCache.find(ptr);
	if (it != boneFilterCache.end())
		return &it->second;
	
	auto emplaced = boneFilterCache.emplace(ptr, std::vector<bool> { true });
	emplaced.first->second.resize(list->size(), false);

	return &emplaced.first->second;
}

void GetBoneFilter(GFxResult& result)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto menu = GetMenu(&selected, &filterMenuCache);
	if (!menu)
		return result.SetError("Could not find filter menu for target");

	auto filter = GetCachedBoneFilter(menu);

	result.CreateMenuItems();

	for (int i = 0; i < menu->size(); ++i) {
		result.PushItem(menu->at(i).first.c_str(), (*filter)[i]);
	}
}

void SetBoneFilter(GFxResult& result, SInt32 index, bool enabled)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto menu = GetMenu(&selected, &filterMenuCache);
	if (!menu)
		return result.SetError("Could not find filter menu for target");

	auto filter = GetCachedBoneFilter(menu);
	(*filter)[index] = enabled;

	UpdateBoneFilter();
}

enum {
	kTransformRotateX = 1,
	kTransformRotateY,
	kTransformRotateZ,
	kTransformTranslateX,
	kTransformTranslateY,
	kTransformTranslateZ,
	kTransformScale
};

void UpdateTransform(UInt32 adjustmentHandle, const char* key, SInt32 type, float scalar)
{
	ActorAdjustmentsPtr adjustments;
	AdjustmentPtr adjustment;
	NodeKey nodeKey;
	if (!GetActorAdjustmentAndNodeKey(key, adjustmentHandle, &adjustments, &adjustment, &nodeKey))
		return;

	auto transform = adjustment->GetTransformOrDefault(nodeKey);

	switch (type) {
	case kTransformRotateX:
	case kTransformRotateY:
	case kTransformRotateZ:
		saf->RotateTransformXYZ(adjustments, adjustment, nodeKey, type, scalar);
		adjustments->UpdateNode(nodeKey.name);
		return;
	case kTransformTranslateX: transform.pos.x += scalar; break;
	case kTransformTranslateY: transform.pos.y += scalar; break;
	case kTransformTranslateZ: transform.pos.z += scalar; break;
	case kTransformScale: transform.scale += scalar; break;
	}

	saf->SetTransform(adjustment, nodeKey, transform);
	adjustments->UpdateNode(nodeKey.name);
}