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

void SetPersistence(UInt32 adjustmentHandle, bool isPersistent)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->persistent = isPersistent;
}

void SetScale(UInt32 adjustmentHandle, int scale)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->scale = scale * 0.01;
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

	GFxValue persistent(adjustment->persistent);
	result->SetMember("persistent", &persistent);

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
		if (!adjustment->hidden) {
			GFxValue name(adjustment->name.c_str());
			names.PushBack(&name);

			GFxValue value(adjustment->handle);
			values.PushBack(&value);
		}
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
		if (!adjustment->hidden) {
			GFxValue name(adjustment->name.c_str());
			names.PushBack(&name);

			GFxValue value(!adjustment->isDefault && !adjustment->saved);
			values.PushBack(&value);

			GFxValue handle(adjustment->handle);
			handles.PushBack(&handle);
		}
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

	adjustments->SavePose(filename, handles);
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

if (!selected.refr) return;
std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
if (!adjustments) return;

std::unordered_set<const char*> adjustmentNames = adjustments->GetAdjustmentNames();

for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAF\\Adjustments", "*.json"); !iter.Done(); iter.Next())
{
	std::string filename(iter.Get()->cFileName);
	const char* noExtension = filename.substr(filename.find_last_of('\\') + 1, filename.length() - 5).c_str();

	GFxValue name(noExtension);
	names.PushBack(&name);

	bool enabled = adjustmentNames.count(noExtension);
	GFxValue value(enabled);
	values.PushBack(&values);
}

result->SetMember("names", &names);
result->SetMember("values", &values);
}

void LoadDefaultAdjustment(const char* filename, bool clear, bool enable)
{
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safMessageDispatcher.GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	//If clearing they are in single select so forcing enable makes more sense
	if (clear) enable = true;

	safMessageDispatcher.loadDefaultAdjustment(selected.race, selected.isFemale, filename, clear, enable);
}

void RotateAdjustmentXYZ(GFxMovieRoot* root, GFxValue* result, const char* key, int adjustmentHandle, int type, double dif) {
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

void AddFolderGFx(GFxMovieRoot* root, GFxValue* result, const char* path) {
	for (IDirectoryIterator iter(path, "*"); !iter.Done(); iter.Next())
	{
		const char* cFileName = iter.Get()->cFileName;
		if (!isDotOrDotDot(cFileName)) {

			std::string filename(iter.Get()->cFileName);
			std::string filepath = iter.GetFullPath();

			if (std::filesystem::is_directory(filepath)) {
				GFxValue folder;
				root->CreateObject(&folder);

				GFxValue name(filename.c_str());
				folder.SetMember("name", &name);

				GFxValue contents;
				root->CreateArray(&contents);
				AddFolderGFx(root, &contents, filepath.c_str());
				folder.SetMember("contents", &contents);

				GFxValue isFolder(true);
				folder.SetMember("folder", &isFolder);

				result->PushBack(&folder);
			}
			else {
				UInt32 size = filename.size();
				if (size >= 5) {
					if (!_stricmp(&filename.c_str()[size - 5], ".json")) {
						GFxValue file;
						root->CreateObject(&file);

						std::string noextension = filename.substr(0, filename.length() - 5);
						GFxValue name(noextension.c_str());
						file.SetMember("name", &name);

						GFxValue pathname(filepath.c_str());
						file.SetMember("path", &pathname);

						result->PushBack(&file);
					}
				}
			}
		}
	}
}

void GetSamPosesGFx(GFxMovieRoot* root, GFxValue* result) {
	root->CreateArray(result);

	AddFolderGFx(root, result, "Data\\F4SE\\Plugins\\SAF\\Poses");
}