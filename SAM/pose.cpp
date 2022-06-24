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

bool SafMessageDispatcher::GetResult() {
	bool _result = result;
	result = false;
	return _result;
}

bool CheckSelectedSkeleton() {
	if (!selected.refr) return false;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return false;

	return true;
}

void SetAdjustmentPos(const char* key, UInt32 adjustmentHandle, float x, float y, float z) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key, kAdjustmentTransformPosition, x, y, z);
	adjustments->UpdateAdjustments(key);
}

void SetAdjustmentRot(const char* key, UInt32 adjustmentHandle, float yaw, float pitch, float roll) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key, kAdjustmentTransformRotation, yaw, pitch, roll);
		
	adjustments->UpdateAdjustments(key);
}

void SetAdjustmentSca(const char* key, UInt32 adjustmentHandle, float scale) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key, kAdjustmentTransformScale, scale, 0, 0);
	adjustments->UpdateAdjustments(key);
}

void ResetAdjustmentTransform(const char* key, int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key, kAdjustmentTransformReset, 0, 0, 0);
	adjustments->UpdateAdjustments(key);
}

void NegateTransform(const char* key, UInt32 adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key, kAdjustmentTransformNegate, 0, 0, 0);
	adjustments->UpdateAdjustments(key);
}

void SaveAdjustmentFile(const char* filename, int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	safMessageDispatcher.saveAdjustment(selected.refr->formID, filename, adjustmentHandle);
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
				std::string key = kvp.second + safAdjustmentManager->overridePostfix;
				safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key.c_str(), kAdjustmentTransformNegate, 0, 0, 0);
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

std::vector<NiAVObject*> FindAdjustableChildren(NiAVObject* root, NodeSet* set) {
	std::vector<NiAVObject*> nodes;
	NiAVObjectVisitAll(root, [&](NiAVObject* object) {
		std::string nodeName(object->m_name.c_str());
		if (set->count(nodeName)) {
			nodes.push_back(object);
			return true;
		}
		return false;
		});
	return nodes;
}

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
	for (auto& name : adjustments->nodeSets->all) {
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
		if (adjustments->HasNode(kvp.second)) return true;
	}
	return false;
}

void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	for (auto& kvp : *categories) {
		if (CheckMenuHasNode(adjustments, kvp.second)) {
			GFxValue category(kvp.first.c_str());
			result->PushBack(&category);
		}
	}
}

void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex)
{
	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();
	if (!categories || categoryIndex >= categories->size()) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	for (auto& kvp : (*categories)[categoryIndex].second) {
		if (adjustments->HasNode(kvp.second)) {
			GFxValue node(kvp.first.c_str());
			result->PushBack(&node);
		}
	}
}

void PushBackTransformGFx(GFxValue* result, NiTransform& transform) {
	float yaw, pitch, roll;
	MatrixToDegree(transform.rot, yaw, pitch, roll);
	//MatrixToEulerRPY(transform.rot, roll, pitch, yaw);

	GFxValue rx(yaw);
	GFxValue ry(pitch);
	GFxValue rz(roll);
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

void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex, int nodeIndex, int adjustmentHandle) {

	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories || categoryIndex >= categories->size() || nodeIndex >= (*categories)[categoryIndex].second.size()) return;

	std::string name = (*categories)[categoryIndex].second[nodeIndex].second;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NiTransform transform = adjustment->GetTransformOrDefault(name);

	PushBackTransformGFx(result, transform);

	GFxValue nodeName(name.c_str());
	result->PushBack(&nodeName);
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

		GFxValue value(!adjustment->isDefault);
		values.PushBack(&value);

		GFxValue handle(adjustment->handle);
		handles.PushBack(&handle);
	});

	result->SetMember("names", &names);
	result->SetMember("values", &values);
	result->SetMember("handles", &handles);
}

void SaveJsonPose(const char* filename, GFxValue selectedAdjustments)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	std::unordered_set<UInt32> handles;
	UInt32 size = selectedAdjustments.GetArraySize();
	for (int i = 0; i < size; ++i) {
		GFxValue handle;
		selectedAdjustments.GetElement(i, &handle);
		handles.insert(handle.GetUInt());
	}

	std::string exportPath = "Exports\\" + std::string(filename);

	adjustments->SavePose(exportPath, handles);
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

void GetDefaultAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result)
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

	std::unordered_set<std::string> adjustmentNames = adjustments->GetAdjustmentNames();

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

void LoadDefaultAdjustment(const char* filename, bool npc, bool clear, bool enable)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	//If clearing they are in single select so forcing enable makes more sense
	if (clear) enable = true;

	//if single target npc is true, send the formId instead of race
	//UInt32 formId = npc ? adjustments->formId : selected.race;
	
	//single target npc disabled for now
	UInt32 formId = selected.race;

	safMessageDispatcher.loadDefaultAdjustment(formId, selected.isFemale, filename, npc, clear, enable);
}

void RotateAdjustmentXYZ(GFxMovieRoot* root, GFxValue* result, const char* key, int adjustmentHandle, int type, int dif) {
	root->CreateArray(result);

	if (!selected.refr) return;

	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	safMessageDispatcher.transformAdjustment(selected.refr->formID, adjustmentHandle, key, kAdjustmentTransformRotate, type, dif * 0.01, 0);

	adjustments->UpdateAdjustments(key);

	NiTransform transform = adjustment->GetTransformOrDefault(key);

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