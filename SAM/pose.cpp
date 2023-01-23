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
std::vector<std::string> poseFavorties;

bool GetActorAdjustments(std::shared_ptr<ActorAdjustments>* adjustments)
{
	if (!selected.refr)
		return false;

	*adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!*adjustments)
		return false;

	return true;
}

bool GetAdjustment(UInt32 handle, std::shared_ptr<Adjustment>* adjustment)
{
	if (!selected.refr)
		return false;

	auto actorAdjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!actorAdjustments)
		return false;

	*adjustment = actorAdjustments->GetAdjustment(handle);
	if (!*adjustment)
		return false;

	return true;
}

bool GetActorAndAdjustment(UInt32 handle, std::shared_ptr<ActorAdjustments>* adjustments, std::shared_ptr<Adjustment>* adjustment)
{
	if (!selected.refr)
		return false;

	*adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments)
		return false;

	*adjustment = (*adjustments)->GetAdjustment(handle);
	if (!*adjustment)
		return false;

	return true;
}

NodeKey GetActorNodeKey(std::shared_ptr<ActorAdjustments> adjustments, const char* key)
{
	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);

	//If offset only, force offset to true
	if (adjustments->IsNodeOffset(nodeKey))
		nodeKey.SetOffset(true);
	
	return nodeKey;
}

void SetAdjustmentPos(const char* key, UInt32 adjustmentHandle, float x, float y, float z) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformPosition, x, y, z);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void SetAdjustmentRot(const char* key, UInt32 adjustmentHandle, float yaw, float pitch, float roll) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformRotation, yaw, pitch, roll);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void SetAdjustmentSca(const char* key, UInt32 adjustmentHandle, float scale) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformScale, scale, 0, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void ResetAdjustmentTransform(const char* key, int adjustmentHandle) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformReset, 0, 0, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void NegateTransform(const char* key, UInt32 adjustmentHandle) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformNegate, 0, 0, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void SaveAdjustmentFile(const char* filename, int adjustmentHandle) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	safDispatcher.SaveAdjustment(selected.refr->formID, filename, adjustmentHandle);
	adjustments->UpdateAllAdjustments();
}

bool LoadAdjustmentFile(const char* filename) 
{
	std::string path = GetPathWithExtension(ADJUSTMENTS_PATH, filename, ".json");

	return LoadAdjustmentPath(path.c_str());
}

bool LoadAdjustmentPath(const char* path) 
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return false;

	safDispatcher.LoadAdjustment(selected.refr->formID, path);

	if (safDispatcher.GetResult()) {
		adjustments->UpdateAllAdjustments();
		return true;
	}

	return false;
}

void PushNewAdjustment(const char* name) {
	if (!selected.refr) 
		return;

	safDispatcher.CreateAdjustment(selected.refr->formID, name);
}

void EraseAdjustment(UInt32 adjustmentHandle) {
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	safDispatcher.RemoveAdjustment(selected.refr->formID, adjustmentHandle);

	adjustments->UpdateAllAdjustments();
}

void ClearAdjustment(UInt32 adjustmentHandle)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	safDispatcher.ResetAdjustment(selected.refr->formID, adjustmentHandle);

	adjustments->UpdateAllAdjustments();
}

void GetAdjustmentNegate(GFxResult& result)
{
	std::shared_ptr<ActorAdjustments> adjustments;
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

void SetAdjustmentNegate(GFxResult& result, const char* adjustmentGroup, UInt32 adjustmentHandle)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	MenuCategoryList* groupsMenu = GetMenu(&selected, &groupsMenuCache);
	if (!groupsMenu)
		return result.SetError("Could not find negation groups for targeted actor");

	//find the correct adjustment group and negate all transforms

	for (auto it = groupsMenu->begin(); it < groupsMenu->end(); it++) {
		if (it->first == adjustmentGroup) {
			for (auto& kvp : it->second) {
				safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle,
					NodeKey(BSFixedString(kvp.first.c_str()), false), kAdjustmentTransformNegate, 0, 0, 0);
			}
		}
	}

	adjustments->UpdateAllAdjustments();
}

bool ShiftAdjustment(UInt32 adjustmentHandle, bool increment)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return false;

	UInt32 fromIndex = adjustments->GetAdjustmentIndex(adjustmentHandle);
	if (fromIndex == -1) 
		return false;

	UInt32 toIndex = fromIndex + (increment ? 1 : -1);

	safDispatcher.MoveAdjustment(selected.refr->formID, fromIndex, toIndex);

	if (safDispatcher.GetResult()) {
		adjustments->UpdateAllAdjustments();
		return true;
	}

	return false;
}

void SetAdjustmentName(GFxResult& result, UInt32 adjustmentHandle, const char* name)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	safDispatcher.RenameAdjustment(selected.refr->formID, adjustmentHandle, name);
}

void MergeAdjustment(GFxResult& result, UInt32 adjustmentHandle)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	safDispatcher.MergeAdjustmentDown(selected.refr->formID, adjustmentHandle);
}

void MirrorAdjustment(GFxResult& result, UInt32 adjustmentHandle)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	std::shared_ptr<Adjustment> adjustment;
	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	if (adjustments->nodeSets->mirror.empty())
		return result.SetError("Missing mirror nodeset for targeted race");

	safDispatcher.MirrorAdjustment(selected.refr->formID, adjustmentHandle);
}

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
	std::shared_ptr<ActorAdjustments> adjustments;
	std::shared_ptr<Adjustment> adjustment;
	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateValues();
	result.PushValue((SInt32)std::round(adjustment->scale * 100));
}

void SetAdjustmentScale(GFxResult& result, UInt32 adjustmentHandle, int scale)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	std::shared_ptr<Adjustment> adjustment;
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
	std::shared_ptr<ActorAdjustments> adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);

	if (!adjustments || !adjustments->nodeSets) return nullptr;
	
	MenuList list;
	for (auto& nodeKey : adjustments->nodeSets->all) {
		std::string name = safDispatcher.GetNodeKeyName(nodeKey);
		list.push_back(std::make_pair(name, name));
	}

	MenuCategoryList categories;
	categories.push_back(std::make_pair("All", list));

	poseMenuCache[selected.race] = categories;

	return &poseMenuCache[selected.race];
}

void GetAdjustmentsGFx(GFxResult& result)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(SKELETON_ERROR);

	result.CreateMenuItems();

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
		result.PushItem(adjustment->name.c_str(), adjustment->handle);
	});
}

bool CheckMenuHasNode(std::shared_ptr<ActorAdjustments> adjustments, MenuList& list)
{
	for (auto& kvp : list) {
		if (adjustments->HasNode(kvp.first.c_str())) 
			return true;
	}
	return false;
}

void GetCategoriesGFx(GFxResult& result)
{
	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) {
		result.SetError("Adjustment category could not be found");
		return;
	}

	std::shared_ptr<ActorAdjustments> adjustments;
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

void GetNodesGFx(GFxResult& result, int categoryIndex)
{
	MenuCategoryList* categories = GetAdjustmentMenu();
	if (!categories || categoryIndex >= categories->size()) {
		result.SetError("Adjustment bones could not be found");
		return;
	}
		
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateMenuItems();

	auto category = &(*categories)[categoryIndex].second;

	int size = category->size();
	for (SInt32 i = 0; i < size; ++i)
	{
		if (adjustments->HasNode((*category)[i].first.c_str())) {
			result.PushItem((*category)[i].second.c_str(), i);
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

void GetTransformGFx(GFxResult& result, const char* nodeName, int adjustmentHandle) 
{
	std::shared_ptr<ActorAdjustments> adjustments;
	std::shared_ptr<Adjustment> adjustment;
	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment)) 
		result.SetError(ADJUSTMENT_MISSING_ERROR);

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(nodeName);

	NiTransform transform = adjustment->GetTransformOrDefault(nodeKey);

	result.CreateValues();
	PushBackTransformGFx(&result.params[1], transform);
}

void GetPoseListGFx(GFxResult& result)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		result.SetError(ADJUSTMENT_MISSING_ERROR);

	result.CreateMenuItems();

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
		result.PushItem(adjustment->name.c_str(), adjustment->file.empty());
	});
}

void SaveJsonPose(const char* filename, GFxValue& checkedAdjustments, int exportType)
{
	std::shared_ptr<ActorAdjustments> adjustments;
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

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
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
		std::string osPath = GetPathWithExtension(safDispatcher.manager->settings.outfitStudioPosePath.c_str(), filename, ".xml");

		safDispatcher.SaveOSPose(selected.refr->formID, osPath.c_str(), &exports);
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

	safDispatcher.SavePose(selected.refr->formID, exportPath.string().c_str(), &exports);
}

bool LoadPoseFile(const char* filename)
{
	std::string path = GetPathWithExtension(POSES_PATH, filename, ".json");

	return LoadPosePath(path.c_str());
}

bool LoadPosePath(const char* path)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return false;

	safDispatcher.LoadPose(selected.refr->formID, path);

	if (safDispatcher.GetResult()) {
		adjustments->UpdateAllAdjustments();

		lastSelectedPose[selected.refr->formID] = GetRelativePath(constStrLen(POSES_PATH), constStrLen(".json"), path);
		return true;
	}

	return false;
}

void ResetJsonPose()
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return;

	safDispatcher.ResetPose(selected.refr->formID);
	adjustments->UpdateAllAdjustments();
}

const char* GetCurrentPoseName()
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments))
		return nullptr;

	auto adjustment = adjustments->GetAdjustmentByType(kAdjustmentTypePose);
	if (!adjustment)
		return nullptr;

	return adjustment->name.c_str();
}

void GetSkeletonAdjustmentsGFx(GFxResult& result, const char* path, bool race)
{
	std::shared_ptr<ActorAdjustments> adjustments;
	if (!GetActorAdjustments(&adjustments)) 
	{
		result.SetError(SKELETON_ERROR);
		return;
	}

	//build a set of all race/skeleton adjustments to compare to
	InsensitiveStringSet adjustmentNames;
	if (race) {
		adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
			if (adjustment->type == kAdjustmentTypeRace) {
				adjustmentNames.insert(adjustment->file);
			}
		});
	}
	else {
		adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
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
		GFxValue checked((bool)adjustmentNames.count(filePath));
		result.PushFileCheckbox(file.first.c_str(), file.second.c_str(), &checked);
	}
}

void LoadSkeletonAdjustment(const char* filename, bool race, bool clear, bool enable)
{
	if (!selected.refr) 
		return;

	std::shared_ptr<ActorAdjustments> adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) 
		return;

	//If clearing they are in single select so forcing enable makes more sense
	if (clear) 
		enable = true;

	//if race adjustment is true, send the formId instead of race
	UInt32 formId = race ? selected.race : adjustments->formId;

	safDispatcher.LoadSkeletonAdjustment(formId, selected.isFemale, filename, race, clear, enable);
}

void RotateAdjustmentXYZ(const char* key, int adjustmentHandle, int type, int dif) {
	std::shared_ptr<ActorAdjustments> adjustments;
	std::shared_ptr<Adjustment> adjustment;
	if (!GetActorAndAdjustment(adjustmentHandle, &adjustments, &adjustment))
		return;

	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safDispatcher.TransformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformRotate, type, dif * 0.01, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void GetPoseExportTypesGFx(GFxResult& result)
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

void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex)
{
	if (selected.refr) {
		std::shared_ptr<ActorAdjustments> adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
		if (adjustments != nullptr) {

			MenuCategoryList* categories = GetAdjustmentMenu();
			if (categories && categoryIndex < categories->size() && nodeIndex < (*categories)[categoryIndex].second.size()) {

				NodeKey nodeKey = GetActorNodeKey(adjustments, (*categories)[categoryIndex].second[nodeIndex].first.c_str());

				selectedNode = safDispatcher.GetNodeKeyName(nodeKey);
				result->SetString(selectedNode.c_str());
			}
		}
	}
}

bool GetNodeIsOffset(const char* nodeName)
{
	NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(nodeName);

	//If it fails assume offset to prevent toggle
	if (!nodeKey.key)
		return true;

	if (!selected.refr) 
		return true;

	std::shared_ptr<ActorAdjustments> adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) 
		return true;

	return adjustments->IsNodeOffset(nodeKey);
}

void ToggleNodeName(GFxValue* result, const char* nodeName)
{
	if (selected.refr) {
		std::shared_ptr<ActorAdjustments> adjustments = safDispatcher.GetActorAdjustments(selected.refr->formID);
		if (adjustments != nullptr) {
			NodeKey nodeKey = safDispatcher.GetNodeKeyFromString(nodeName);

			//If offset only, force to offset to prevent errors
			if (adjustments->IsNodeOffset(nodeKey)) {
				nodeKey.SetOffset(true);
			}
			else {
				nodeKey.SetOffset(!nodeKey.offset);
			}
			
			selectedNode = safDispatcher.GetNodeKeyName(nodeKey);
			result->SetString(selectedNode.c_str());
			return;
		}
	}
	
	//keep same node on failure
	result->SetString(nodeName);
}

void FindNodeIndexes(NodeKey& nodeKey, SInt32* categoryIndex, SInt32* nodeIndex)
{
	*categoryIndex = -1;
	*nodeIndex = -1;

	auto menu = GetAdjustmentMenu();
	if (!menu)
		return;

	for (SInt32 i = 0; i < menu->size(); ++i) {
		for (SInt32 j = 0; j < (*menu)[i].second.size(); ++j) {
			NodeKey menuKey = safDispatcher.GetNodeKeyFromString((*menu)[i].second[j].first.c_str());
			if (menuKey.key == nodeKey.key) {
				*categoryIndex = i;
				*nodeIndex = j;
				return;
			}
		}
	}
}

void GetPoseFavorites(GFxResult& result)
{
	result.CreateMenuItems();

	//natural sort by stem
	NaturalSortedMap map;
	for (auto& item : poseFavorties) {
		map.emplace(std::filesystem::path(item).stem().string(), item);
	}

	for (auto& item : map) {
		result.PushItem(item.first.c_str(), item.second.c_str());
	}
}

void AppendPoseFavorite(GFxResult& result)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto it = lastSelectedPose.find(selected.refr->formID);
	if (it == lastSelectedPose.end())
		return result.SetError("Current SAM pose could not be found");

	std::ofstream stream;
	if (!SAF::OpenOutFileStream(POSE_FAVORITES, &stream))
		return;

	stream << it->second << std::endl;
	stream.close();

	poseFavorties.push_back(it->second);
}

void PlayPoseFavorite(GFxResult& result, const char* poseName)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	std::string path = GetPathWithExtension(POSES_PATH, poseName, ".json");
	if (!std::filesystem::exists(path))
		return result.SetError("SAM Pose file could not be found");

	if (!LoadPosePath(path.c_str()))
		return result.SetError("Failed to load SAM Pose");
}

bool LoadPoseFavorites()
{
	if (!std::filesystem::exists(POSE_FAVORITES)) {
		IFileStream::MakeAllDirs(POSE_FAVORITES);
		IFileStream file;
		if (!file.Create(POSE_FAVORITES)) {
			_Log("Failed to create pose favorites: ", POSE_FAVORITES);
			return false;
		}
		file.Close();
	}

	std::ifstream stream;
	stream.open(POSE_FAVORITES);

	if (stream.fail()) {
		_Log("Failed to read pose favorites: ", POSE_FAVORITES);
		return false;
	}

	poseFavorties.clear();

	std::string line;
	while (std::getline(stream, line, '\n'))
	{
		if (line.back() == '\r')
			line = line.substr(0, line.size() - 1);
		poseFavorties.push_back(line);
	}

	return true;
}