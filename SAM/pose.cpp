#include "pose.h"

#include "f4se/NiTypes.h"

#include <regex>
#include <unordered_map>
#include <unordered_set>

#include "sam.h"

#include "common/IDirectoryIterator.h"

#include <filesystem>

#include "SAF/conversions.h"
#include "SAF/adjustments.h"
using namespace SAF;

AdjustmentManager* safAdjustmentManager = nullptr;

SafMessageDispatcher safMessageDispatcher;

std::string selectedNode;

std::shared_ptr<ActorAdjustments> SafMessageDispatcher::GetActorAdjustments(UInt32 formId) {
	std::lock_guard<std::mutex> lock(mutex);

	if (!safAdjustmentManager) return nullptr;

	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);

	if (!adjustments) {
		createActorAdjustments(formId);
		adjustments = actorAdjustments;
		actorAdjustments = nullptr;
	}

	return adjustments;
}

UInt32 SafMessageDispatcher::GetResult() {
	UInt32 _result = result;
	result = 0;
	return _result;
}

bool CheckSelectedSkeleton() {
	if (!selected.refr) return false;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return false;

	return true;
}

NodeKey GetActorNodeKey(std::shared_ptr<ActorAdjustments> adjustments, const char* key)
{
	NodeKey nodeKey = GetNodeKeyFromString(key);

	//If offset only, force offset to true
	if (adjustments->IsNodeOffset(nodeKey))
		nodeKey.SetOffset(true);
	
	return nodeKey;
}

void SetAdjustmentPos(const char* key, UInt32 adjustmentHandle, float x, float y, float z) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformPosition, x, y, z);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void SetAdjustmentRot(const char* key, UInt32 adjustmentHandle, float yaw, float pitch, float roll) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformRotation, yaw, pitch, roll);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void SetAdjustmentSca(const char* key, UInt32 adjustmentHandle, float scale) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformScale, scale, 0, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void ResetAdjustmentTransform(const char* key, int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformReset, 0, 0, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void NegateTransform(const char* key, UInt32 adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformNegate, 0, 0, 0);
		adjustments->UpdateNode(nodeKey.name);
	}
}

void SaveAdjustmentFile(const char* filename, int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	safMessageDispatcher.saveAdjustment(selected.refr->formID, filename, adjustmentHandle);
	adjustments->UpdateAllAdjustments();
}

bool LoadAdjustmentFile(const char* filename) {
	if (!selected.refr) return false;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return false;

	safMessageDispatcher.loadAdjustment(selected.refr->formID, filename);

	if (safMessageDispatcher.GetResult()) {
		adjustments->UpdateAllAdjustments();
		return true;
	}

	return false;
}

void PushNewAdjustment(const char* name) {
	if (!selected.refr) return;

	safMessageDispatcher.createAdjustment(selected.refr->formID, name);
}

void EraseAdjustment(UInt32 adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	safMessageDispatcher.removeAdjustment(selected.refr->formID, adjustmentHandle);

	adjustments->UpdateAllAdjustments();
}

void ClearAdjustment(UInt32 adjustmentHandle)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	safMessageDispatcher.resetAdjustment(selected.refr->formID, adjustmentHandle);

	adjustments->UpdateAllAdjustments();
}

void NegateAdjustments(UInt32 adjustmentHandle, const char* adjustmentGroup)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	MenuCategoryList* groupsMenu = GetMenu(&groupsMenuCache);
	if (!groupsMenu) return;

	//find the correct adjustment group and negate all transforms
	for (auto it = groupsMenu->begin(); it < groupsMenu->end(); it++) {
		if (it->first == adjustmentGroup) {
			for (auto& kvp : it->second) {
				safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle,
					NodeKey(BSFixedString(kvp.second.c_str()), false), kAdjustmentTransformNegate, 0, 0, 0);
			}
		}
	}

	adjustments->UpdateAllAdjustments();
}

bool ShiftAdjustment(UInt32 adjustmentHandle, bool increment)
{
	if (!selected.refr) return false;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return false;

	UInt32 fromIndex = adjustments->GetAdjustmentIndex(adjustmentHandle);
	if (fromIndex == -1) return false;
	UInt32 toIndex = fromIndex + (increment ? 1 : -1);

	safMessageDispatcher.moveAdjustment(selected.refr->formID, fromIndex, toIndex);

	if (safMessageDispatcher.GetResult()) {
		adjustments->UpdateAllAdjustments();
		return true;
	}

	return false;
}

void SetAdjustmentName(UInt32 adjustmentHandle, const char* name)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	safMessageDispatcher.renameAdjustment(selected.refr->formID, adjustmentHandle, name);
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

//std::vector<NiAVObject*> FindAdjustableChildren(NiAVObject* root, NodeSet* set) {
//	std::vector<NiAVObject*> nodes;
//	NiAVObjectVisitAll(root, [&](NiAVObject* object) {
//		BSFixedString nodeName(object->m_name.c_str());
//		if (set->count(nodeName)) {
//			nodes.push_back(object);
//			return true;
//		}
//		return false;
//	});
//	return nodes;
//}

void SetScale(UInt32 adjustmentHandle, int scale)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->SetScale(scale * 0.01);
	adjustments->UpdateAllAdjustments(adjustment);
}

void GetAdjustmentGFx(GFxMovieRoot* root, GFxValue* result, int adjustmentHandle)
{
	root->CreateObject(result);

	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	GFxValue scale((SInt32)std::round(adjustment->scale * 100));
	result->SetMember("scale", &scale);

	GFxValue groups;
	root->CreateArray(&groups);

	MenuCategoryList* groupsMenu = GetMenu(&groupsMenuCache);
	if (groupsMenu) {
		for (auto& kvp : *groupsMenu) {
			GFxValue groupName(kvp.first.c_str());
			groups.PushBack(&groupName);
		}
	}

	result->SetMember("groups", &groups);
}

MenuCategoryList* GetAdjustmentMenu()
{
	MenuCategoryList* menu = GetMenu(&poseMenuCache);

	if (menu) return menu;

	//If for some reason the human menu can't be found dump it all into one menu
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);

	if (!adjustments || !adjustments->nodeSets) return nullptr;
	
	MenuList list;
	for (auto& nodeKey : adjustments->nodeSets->all) {
		std::string name = GetNodeKeyName(nodeKey);
		list.push_back(std::make_pair(name, name));
	}

	MenuCategoryList categories;
	categories.push_back(std::make_pair("All", list));

	poseMenuCache[selected.race] = categories;

	return &poseMenuCache[selected.race];
}

void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateObject(result);

	GFxValue names;
	root->CreateArray(&names);

	GFxValue values;
	root->CreateArray(&values);

	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
		GFxValue name(adjustment->name.c_str());
		names.PushBack(&name);

		GFxValue value(adjustment->handle);
		values.PushBack(&value);
	});

	result->SetMember("names", &names);
	result->SetMember("values", &values);
}

bool CheckMenuHasNode(std::shared_ptr<ActorAdjustments> adjustments, MenuList& list)
{
	for (auto& kvp : list) {
		if (adjustments->HasNode(kvp.second.c_str())) return true;
	}
	return false;
}

void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateObject(result);

	GFxValue names;
	root->CreateArray(&names);

	GFxValue values;
	root->CreateArray(&values);

	result->SetMember("names", &names);
	result->SetMember("values", &values);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	int size = categories->size();
	for (SInt32 i = 0; i < size; ++i)
	{
		if (CheckMenuHasNode(adjustments, (*categories)[i].second)) {
			GFxValue category((*categories)[i].first.c_str());
			names.PushBack(&category);
			GFxValue index(i);
			values.PushBack(&index);
		}
	}
}

void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex)
{
	root->CreateArray(result);

	GFxValue names;
	root->CreateArray(&names);

	GFxValue values;
	root->CreateArray(&values);

	result->SetMember("names", &names);
	result->SetMember("values", &values);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();
	if (!categories || categoryIndex >= categories->size()) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	auto category = &(*categories)[categoryIndex].second;

	int size = category->size();
	for (SInt32 i = 0; i < size; ++i)
	{
		if (adjustments->HasNode((*category)[i].second.c_str())) {
			GFxValue node((*category)[i].first.c_str());
			names.PushBack(&node);
			GFxValue index(i);
			values.PushBack(&index);
		}
	}
}

void PushBackTransformGFx(GFxValue* result, NiTransform& transform) {
	float yaw, pitch, roll;
	//MatrixToDegree(transform.rot, yaw, pitch, roll);
	//MatrixToEulerRPY(transform.rot, roll, pitch, yaw);
	MatrixToEulerYPR2(transform.rot, yaw, pitch, roll);

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

void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, const char* nodeName, int adjustmentHandle) {

	root->CreateArray(result);

	if (!selected.refr) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(nodeName);

	NiTransform transform = adjustment->GetTransformOrDefault(nodeKey);

	PushBackTransformGFx(result, transform);
}

void GetPoseListGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateObject(result);

	GFxValue names;
	root->CreateArray(&names);

	GFxValue values;
	root->CreateArray(&values);

	GFxValue handles;
	root->CreateArray(&handles);

	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
		GFxValue name(adjustment->name.c_str());
		names.PushBack(&name);

		GFxValue value(adjustment->file.empty());
		values.PushBack(&value);

		GFxValue handle(adjustment->handle);
		handles.PushBack(&handle);
	});

	result->SetMember("names", &names);
	result->SetMember("values", &values);
	result->SetMember("handles", &handles);
}

void SaveJsonPose(const char* filename, GFxValue selectedAdjustments, int exportType)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	auto menu = GetMenu(&exportMenuCache);

	//+2 for All/Outfit Studio
	if (exportType < 0 || exportType > menu->size() + 1)
		return;

	std::string exportPath = "Exports\\" + std::string(filename);

	ExportSkeleton exports;

	UInt32 size = selectedAdjustments.GetArraySize();
	for (int i = 0; i < size; ++i) {
		GFxValue handle;
		selectedAdjustments.GetElement(i, &handle);
		exports.handles.insert(handle.GetUInt());
	}

	//menu size +1 = Outfit Studio Xml
	if (exportType == menu->size() + 1) 
	{
		adjustments->SaveOutfitStudioPose(filename, &exports);
		return;
	}

	NodeSet nodeSet;

	//Types out of range will be considered as exporting all
	if (exportType < menu->size()) 
	{
		for (auto& node : (*menu)[exportType].second) {
			nodeSet.insert(NodeKey(BSFixedString(node.second.c_str()), false));
		}
		exports.nodes = &nodeSet;
		exports.skeleton = (*menu)[exportType].first.c_str();
	}
	else {
		exports.nodes = nullptr;
	}

	adjustments->SavePose(exportPath, &exports);
}

bool LoadJsonPose(const char* filename)
{
	if (!selected.refr) return false;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return false;

	safMessageDispatcher.loadPose(selected.refr->formID, filename);

	if (safMessageDispatcher.GetResult()) {
		adjustments->UpdateAllAdjustments();
		return true;
	}

	return false;
}

void ResetJsonPose()
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	safMessageDispatcher.resetPose(selected.refr->formID);
	adjustments->UpdateAllAdjustments();
}

void GetSkeletonAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result, bool race)
{
	root->CreateObject(result);

	GFxValue names;
	root->CreateArray(&names);

	GFxValue values;
	root->CreateArray(&values);

	result->SetMember("names", &names);
	result->SetMember("values", &values);

	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

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

	for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\Adjustments", "*.json"); !iter.Done(); iter.Next())
	{
		std::string filename(iter.Get()->cFileName);
		std::string noExtension = filename.substr(0, filename.length() - 5);

		GFxValue name(noExtension.c_str());
		names.PushBack(&name);

		bool enabled = adjustmentNames.count(noExtension.c_str());

		GFxValue value(enabled);
		values.PushBack(&value);
	}
}

void LoadSkeletonAdjustment(const char* filename, bool race, bool clear, bool enable)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	//If clearing they are in single select so forcing enable makes more sense
	if (clear) enable = true;

	//if race adjustment is true, send the formId instead of race
	UInt32 formId = race ? selected.race : adjustments->formId;

	safMessageDispatcher.loadDefaultAdjustment(formId, selected.isFemale, filename, race, clear, enable);
}

void RotateAdjustmentXYZ(GFxMovieRoot* root, GFxValue* result, const char* key, int adjustmentHandle, int type, int dif) {
	root->CreateArray(result);

	if (!selected.refr) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NodeKey nodeKey = GetNodeKeyFromString(key);
	if (nodeKey.key) {
		safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, nodeKey, kAdjustmentTransformRotate, type, dif * 0.01, 0);
		adjustments->UpdateNode(nodeKey.name);
	}

	NiTransform transform = adjustment->GetTransformOrDefault(nodeKey);

	PushBackTransformGFx(result, transform);
}

bool isDotOrDotDot(const char* cstr) {
	if (cstr[0] != '.') return false;
	if (cstr[1] == 0) return true;
	if (cstr[1] != '.') return false;
	return (cstr[2] == 0);
}

void GetSamPosesGFx(GFxMovieRoot* root, GFxValue* result, const char* path) {
	root->CreateArray(result);

	std::map<std::string, std::string, NaturalSort> folders;
	std::map<std::string, std::string, NaturalSort> files;

	for (IDirectoryIterator iter(path, "*"); !iter.Done(); iter.Next())
	{
		const char* cFileName = iter.Get()->cFileName;
		if (!isDotOrDotDot(cFileName)) {

			std::string filename(iter.Get()->cFileName);
			std::string filepath = iter.GetFullPath();

			if (std::filesystem::is_directory(filepath)) {
				folders[filename] = filepath;
			}
			else {
				UInt32 size = filename.size();
				if (size >= 5) {
					if (!_stricmp(&filename.c_str()[size - 5], ".json")) {
						std::string noExtension = filename.substr(0, filename.length() - 5);
						files[noExtension] = filepath;
					}
				}
			}
		}
	}

	for (auto& folder : folders) {
		GFxValue value;
		root->CreateObject(&value);

		std::string folderName = folder.first;
		GFxValue name(folderName.c_str());
		value.SetMember("name", &name);

		GFxValue isFolder(true);
		value.SetMember("folder", &isFolder);

		GFxValue pathname(folder.second.c_str());
		value.SetMember("path", &pathname);

		result->PushBack(&value);
	}

	for (auto& file : files) {
		GFxValue value;
		root->CreateObject(&value);

		GFxValue name(file.first.c_str());
		value.SetMember("name", &name);

		GFxValue pathname(file.second.c_str());
		value.SetMember("path", &pathname);

		result->PushBack(&value);
	}
}

void GetPoseExportTypesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	auto menu = GetMenu(&exportMenuCache);
	for (auto& category : *menu) 
	{
		result->PushBack(&GFxValue(category.first.c_str()));
	}

	result->PushBack(&GFxValue("All"));
	result->PushBack(&GFxValue("Outfit Studio"));
}

void GetNodeNameFromIndexes(GFxValue* result, UInt32 categoryIndex, UInt32 nodeIndex)
{
	if (selected.refr) {
		std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
		if (adjustments != nullptr) {

			MenuCategoryList* categories = GetAdjustmentMenu();
			if (categories && categoryIndex < categories->size() && nodeIndex < (*categories)[categoryIndex].second.size()) {

				NodeKey nodeKey = GetActorNodeKey(adjustments, (*categories)[categoryIndex].second[nodeIndex].second.c_str());

				selectedNode = GetNodeKeyName(nodeKey);
				result->SetString(selectedNode.c_str());
			}
		}
	}
}

bool GetNodeIsOffset(const char* nodeName)
{
	NodeKey nodeKey = GetNodeKeyFromString(nodeName);

	//If it fails assume offset to prevent toggle
	if (!nodeKey.key)
		return true;

	if (!selected.refr) return true;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return true;

	return adjustments->IsNodeOffset(nodeKey);
}

void ToggleNodeName(GFxValue* result, const char* nodeName)
{
	if (selected.refr) {
		std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
		if (adjustments != nullptr) {
			NodeKey nodeKey = GetNodeKeyFromString(nodeName);

			//If offset only, force to offset to prevent errors
			if (adjustments->IsNodeOffset(nodeKey)) {
				nodeKey.SetOffset(true);
			}
			else {
				nodeKey.SetOffset(!nodeKey.offset);
			}

			selectedNode = GetNodeKeyName(nodeKey);
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
			NodeKey menuKey = GetNodeKeyFromString((*menu)[i].second[j].second.c_str());
			if (menuKey.key == nodeKey.key) {
				*categoryIndex = i;
				*nodeIndex = j;
				return;
			}
		}
	}
}