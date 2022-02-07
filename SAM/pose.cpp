#include "pose.h"

#include "f4se/NiTypes.h"

#include "common/IDirectoryIterator.h"
#include "common/IFileStream.h"

#include <regex>
#include <unordered_map>

#include "sam.h"

#include "SAF/util.h"
#include "SAF/adjustments.h"
using namespace SAF;

AdjustmentManager* safAdjustmentManager;

MenuCache menuCache;

void SetAdjustmentPos(std::string name, UInt32 adjustmentIndex, float x, float y, float z) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetListAdjustment(adjustmentIndex);
	if (!adjustment) return;

	adjustment->SetTransformPos(name, x, y, z);
	adjustments->UpdateAdjustments(name);
}

void SetAdjustmentRot(std::string name, UInt32 adjustmentIndex, float heading, float attitude, float bank) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetListAdjustment(adjustmentIndex);
	if (!adjustment) return;

	adjustment->SetTransformRot(name, heading, attitude, bank);
	adjustments->UpdateAdjustments(name);
}

void SetAdjustmentSca(std::string name, UInt32 adjustmentIndex, float scale) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetListAdjustment(adjustmentIndex);
	if (!adjustment) return;

	adjustment->SetTransformSca(name, scale);
	adjustments->UpdateAdjustments(name);
}

void ResetAdjustmentTransform(std::string name, int adjustmentIndex) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetListAdjustment(adjustmentIndex);
	if (!adjustment) return;

	adjustment->ResetTransform(name);
	adjustments->UpdateAdjustments(name);
}

void SaveAdjustmentFile(std::string filename, int adjustmentIndex) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->SaveListAdjustment(filename, adjustmentIndex);
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

void EraseAdjustment(int adjustmentIndex) {
	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->RemoveListAdjustment(adjustmentIndex);
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
	if (menuCache.count(selected.key))
		return &menuCache[selected.key];
	if (selected.isFemale && menuCache.count(selected.race))
		return &menuCache[selected.race];

	//Just dump everything into one menu option if it's not defined
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	
	MenuList list;
	for (auto& name : adjustments->nodeSets->all) {
		list.push_back(std::make_pair(name, name));
	}

	MenuCategoryList categories;
	categories.push_back(std::make_pair("All", list));

	menuCache[selected.race] = categories;

	return &menuCache[selected.race];
}

void GetAdjustmentsGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	if (!selected.refr) return;
	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;

	adjustments->ForEachAdjustment([&](std::shared_ptr<Adjustment> adjustment) {
		GFxValue adjustmentName(adjustment->name.c_str());
		result->PushBack(&adjustmentName);
	});
}

void GetCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();

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

	for (auto& kvp : (*categories)[categoryIndex].second) {
		GFxValue node(kvp.first.c_str());
		result->PushBack(&node);
	}
}

void GetTransformGFx(GFxMovieRoot* root, GFxValue* result, int categoryIndex, int nodeIndex, int adjustmentIndex) {

	root->CreateArray(result);

	if (!selected.refr) return;

	MenuCategoryList* categories = GetAdjustmentMenu();
	std::string name = (*categories)[categoryIndex].second[nodeIndex].second;

	std::shared_ptr<ActorAdjustments> adjustments = safAdjustmentManager->GetActorAdjustments(selected.refr->formID);
	if (!adjustments) return;
	std::shared_ptr<Adjustment> adjustment = adjustments->GetListAdjustment(adjustmentIndex);
	if (!adjustment) return;

	NiTransform transform = adjustment->GetTransformOrDefault(name);

	GFxValue px(transform.pos.x);
	GFxValue py(transform.pos.y);
	GFxValue pz(transform.pos.z);
	result->PushBack(&px);
	result->PushBack(&py);
	result->PushBack(&pz);

	float heading, attitude, bank;
	transform.rot.GetEulerAngles(&heading, &attitude, &bank);

	//heading attitude bank, y z x
	GFxValue rx(bank);
	GFxValue ry(heading);
	GFxValue rz(attitude);
	result->PushBack(&rx);
	result->PushBack(&ry);
	result->PushBack(&rz);

	GFxValue scale(transform.scale);
	result->PushBack(&scale);

	GFxValue nodeName(name.c_str());
	result->PushBack(&nodeName);
}

std::regex menuCategoryRegex("([^\\t]+)\\t+([^\\t]+)");
std::unordered_map<std::string, UInt32> menuHeaderMap = {
	{"Race", 0},
	{"Mod", 1},
	{"Sex", 2}
};

bool LoadMenuFile(std::string path) {
	IFileStream file;

	if (!file.Open(path.c_str())) {
		_DMESSAGE("File not found");
		return false;
	}

	char buf[512];
	std::cmatch match;
	std::string categoryIdentifier = "Category";
	std::string header[3];

	//header
	for (int i = 0; i < 3; ++i) {
		file.ReadString(buf, 512, '\n', '\r');
		bool fail = false;
		if (std::regex_match(buf, match, menuCategoryRegex)) {
			std::string key = match[1].str();
			if (menuHeaderMap.count(key))
				header[menuHeaderMap[key]] = match[2].str();
			else
				fail = true;
		}
		else {
			fail = true;
		}
		if (fail) {
			_LogCat("Failed to read header ", path);
			file.Close();
			return false;
		}
	}

	UInt64 key = GetFormID(header[1], header[0]);
	if (header[2] == "female" || header[2] == "Female")
		key |= 0x100000000;

	//categories
	while (!file.HitEOF()) {
		file.ReadString(buf, 512, '\n', '\r');
		if (std::regex_match(buf, match, menuCategoryRegex)) {
			if (match[1].str() == categoryIdentifier) {
				std::vector<std::pair<std::string, std::string>> list;
				menuCache[key].push_back(std::make_pair(match[2].str(), list));
			}
			else {
				menuCache[key].back().second.push_back(std::make_pair(match[2].str(), match[1].str()));
			}
		}
	}

	file.Close();

	return true;
}

void LoadMenuFiles() {
	for (IDirectoryIterator iter("Data\\F4SE\\Plugins\\SAM\\Menus", ".txt"); !iter.Done(); iter.Next())
	{
		std::string	path = iter.GetFullPath();
		LoadMenuFile(path);
	}
}