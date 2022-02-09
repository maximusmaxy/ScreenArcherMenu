#include "pose.h"

#include "f4se/NiTypes.h"

#include <regex>
#include <unordered_map>

#include "sam.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"
using namespace SAF;

AdjustmentManager* safAdjustmentManager;

void SetAdjustmentPos(std::string name, UInt32 adjustmentHandle, float x, float y, float z) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetListAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->SetTransformPos(name, x, y, z);
	adjustments->UpdateAdjustments(name);
}

void SetAdjustmentRot(std::string name, UInt32 adjustmentHandle, float heading, float attitude, float bank) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->SetTransformRot(name, heading / 180 * MATH_PI, attitude / 180 * MATH_PI, bank / 180 * MATH_PI);
	adjustments->UpdateAdjustments(name);
}

void SetAdjustmentSca(std::string name, UInt32 adjustmentHandle, float scale) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->SetTransformSca(name, scale);
	adjustments->UpdateAdjustments(name);
}

void ResetAdjustmentTransform(std::string name, int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	adjustment->ResetTransform(name);
	adjustments->UpdateAdjustments(name);
}

void SaveAdjustmentFile(std::string filename, int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->SaveAdjustment(filename, adjustmentHandle);
}

void LoadAdjustmentFile(std::string filename) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->LoadAdjustment(filename, "ScreenArcherMenu.esp", true, false);
	adjustments->UpdateAllAdjustments();
}

void PushNewAdjustment(std::string name) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->CreateAdjustment(name, "ScreenArcherMenu.esp", false, false);
}

void EraseAdjustment(int adjustmentHandle) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->RemoveAdjustment(adjustmentHandle);
	adjustments->UpdateAllAdjustments();
}

NiMatrix43 RotateMatrixXYZ(NiMatrix43 matrix, float x, float y, float z) {
	NiMatrix43 rot;

	float xSin = std::sinf(x);
	float xCos = std::cosf(x);
	rot.data[0][0] = 1.0f;
	rot.data[0][1] = 0.0f;
	rot.data[0][2] = 0.0f;
	rot.data[1][0] = 0.0f;
	rot.data[1][1] = xCos;
	rot.data[1][2] = xSin;
	rot.data[2][0] = 0.0f;
	rot.data[2][1] = -xSin;
	rot.data[2][2] = xCos;
	matrix = matrix * rot;

	float ySin = std::sinf(y);
	float yCos = std::cosf(y);
	rot.data[0][0] = yCos;
	rot.data[0][1] = 0.0f;
	rot.data[0][2] = -ySin;
	rot.data[1][0] = 0.0f;
	rot.data[1][1] = 1.0f;
	rot.data[1][2] = 0.0f;
	rot.data[2][0] = ySin;
	rot.data[2][1] = 0.0f;
	rot.data[2][2] = yCos;
	matrix = matrix * rot;

	float zSin = std::sinf(z);
	float zCos = std::cosf(z);
	rot.data[0][0] = zCos;
	rot.data[0][1] = zSin;
	rot.data[0][2] = 0.0f;
	rot.data[1][0] = -zSin;
	rot.data[1][1] = zCos;
	rot.data[1][2] = 0.0f;
	rot.data[2][0] = 0.0f;
	rot.data[2][1] = 0.0f;
	rot.data[2][2] = 1.0f;
	matrix = matrix * rot;

	return matrix;
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

MenuCategoryList* GetAdjustmentMenu()
{
	MenuCategoryList* menu = GetMenu(&poseMenuCache);

	if (menu) return menu;

	//If for some reason the human menu can't be found dump it all into one menu
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);

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
	static std::string samEsp("ScreenArcherMenu.esp");

	root->CreateObject(result);

	GFxValue names;
	root->CreateArray(&names);

	GFxValue values;
	root->CreateArray(&values);

	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
		if (!adjustment->hidden || adjustment->esp == samEsp) {
			GFxValue name(adjustment->name.c_str());
			names.PushBack(&name);

			GFxValue value(adjustment->handle);
			values.PushBack(&value);
		}
	});

	result->SetMember("names", &names);
	result->SetMember("values", &values);
}

void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) return;

	for (auto& kvp : *categories) {
		GFxValue category(kvp.first.c_str());
		result->PushBack(&category);
	}
}

void GetNodesGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex)
{
	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) return;

	for (auto& kvp : (*categories)[categoryIndex].second) {
		GFxValue node(kvp.first.c_str());
		result->PushBack(&node);
	}
}

void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex, int nodeIndex, int adjustmentHandle) {

	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

	if (!categories) return;

	std::string name = (*categories)[categoryIndex].second[nodeIndex].second;

	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetAdjustment(adjustmentHandle);
	if (!adjustment) return;

	NiTransform transform = adjustment->GetTransformOrDefault(name);

	float heading, attitude, bank;
	transform.rot.GetEulerAngles(&heading, &attitude, &bank);

	//heading attitude bank, y z x
	GFxValue rx(bank * 180 / MATH_PI);
	GFxValue ry(heading * 180 / MATH_PI);
	GFxValue rz(attitude * 180 / MATH_PI);
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

	GFxValue nodeName(name.c_str());
	result->PushBack(&nodeName);
}