#include "sam.h"

#include "f4se/GameEvents.h"
#include "f4se/GameMenus.h"
#include "f4se/GameData.h"
#include "f4se/GameTypes.h"
#include "f4se/GameRTTI.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/ScaleformLoader.h"
#include "f4se/PluginManager.h"
#include "f4se/CustomMenu.h"
#include "f4se/Serialization.h"
#include "f4se/PapyrusScaleformAdapter.h"

#include "SAF/util.h"
#include "SAF/hacks.h"
#include "SAF/eyes.h"

#include "constants.h"
#include "pose.h"
#include "mfg.h"
#include "idle.h"
#include "hacks.h"
#include "positioning.h"
#include "compatibility.h"
#include "options.h"
#include "camera.h"
#include "lights.h"
#include "papyrus.h"
#include "input.h"
#include "gfx.h"
#include "io.h"
#include "data.h"
#include "functions.h"
#include "scaleform.h"
#include "coc.h"

#include <WinUser.h>
#include <libloaderapi.h>

SAF::SafMessagingInterface* saf = nullptr;
SelectedRefr selected;
SamManager samManager;

ScreenArcherMenu::ScreenArcherMenu() : GameMenuBase()
{
	//looksmenu = 0x8058404
	flags =
		IMenu::kFlag_AlwaysOpen |
		IMenu::kFlag_UsesCursor |
		IMenu::kFlag_Modal |
		IMenu::kFlag_DisablePauseMenu |
		IMenu::kFlag_UpdateUsesCursor |
		IMenu::kFlag_CustomRendering |
		IMenu::kFlag_AssignCursorToRenderer |
		//IMenu::kFlag_HasButtonBar | 
		//IMenu::kFlag_IsTopButtonBar |
		IMenu::kFlag_UsesMovementToDirection;

	depth = 6;
	context = 0x22;

	if ((*g_inputDeviceMgr)->IsGamepadEnabled())
		flags &= ~IMenu::kFlag_UsesCursor;

	UInt32 movieFlags = 3;
	UInt32 extendedFlags = 3;

	if (CALL_MEMBER_FN((*g_scaleformManager), LoadMovie)(this, movie, SAM_MENU_NAME, SAM_MENU_ROOT, movieFlags))
	{
		GFxValue flagsValue(flags);
		GFxValue movieFlagsValue(movieFlags);
		GFxValue extendedFlagsValue(extendedFlags);
		stage.SetMember("menuFlags", &flagsValue);
		stage.SetMember("movieFlags", &movieFlagsValue);
		stage.SetMember("extendedFlags", &extendedFlagsValue);

		CreateBaseShaderTarget(filterHolder, stage);
		filterHolder->SetFilterColor(false);
		shaderFXObjects.Push(filterHolder);
		(*g_colorUpdateDispatcher)->eventDispatcher.AddEventSink(filterHolder);
	}
	else {
		_DMESSAGE("Failed to open SAM menu");
	}
}

void ScreenArcherMenu::RegisterFunctions()
{
	auto it = samFunctions.names.begin();
	int idx = 0;
	for (; it != samFunctions.names.end(); ++it, ++idx) {
		RegisterNativeFunction(it->c_str(), idx);
	}
}

void ScreenArcherMenu::Invoke(Args* args)
{
	samFunctions.funcs.at(args->optionID)->Call(args);
}

NiColor GetNiColor(float r, float g, float b) {
	NiColor color;

	color.r = r;
	color.g = g;
	color.b = b;

	return color;
}

RelocAddr<NiColor> gameHudColor(0x6577D90);

void SetShaderColor(BSGFxShaderFXTarget* target, float r, float g, float b) {

	auto hudColor = (NiColor*)gameHudColor.GetUIntPtr();

	NiColor color;
	color.r = r == 0 ? 0 : hudColor->r / r;
	color.g = g == 0 ? 0 : hudColor->g / g;
	color.b = b == 0 ? 0 : hudColor->b / b;

	target->EnableColorMultipliers(&color, 1.0f);
}

void ScreenArcherMenu::PushNodeMarker(NiAVObject* node, BSFixedString name) {
	auto& emplaced = boneDisplay.nodes.emplace_back(BoneDisplay::NodeMarker(node, name));
	GFxValue nodeName(node->m_name.c_str());
	movie->movieRoot->CreateObject(&emplaced.marker, "NodeMarker", &nodeName, 1);
	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &emplaced.marker, 1);
	movie->movieRoot->Invoke("root1.Menu_mc.AddNodeMarker", nullptr, &emplaced.marker, 1);
}

void ScreenArcherMenu::PushBoneMarker(BoneDisplay::NodeMarker* start, BoneDisplay::NodeMarker* end) {
	auto& emplaced = boneDisplay.bones.emplace_back(BoneDisplay::BoneMarker(start, end));
	movie->movieRoot->CreateObject(&emplaced.marker, "BoneMarker", nullptr, 0);
	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &emplaced.marker, 1);
}

void ScreenArcherMenu::VisitNodes(SAF::BSFixedStringSet& set, SAF::BSFixedStringSet& found, NiAVObject* parent, BSFixedString parentName)
{
	NiPointer<NiNode> node(parent->GetAsNiNode());

	if (node) {
		for (UInt32 i = 0; i < node->m_children.m_emptyRunStart; i++) {
			NiPointer<NiAVObject> object(node->m_children.m_data[i]);
			if (object) {

				//if child is found, add markers and recursive call with new parent
				if (set.count(object->m_name) && !found.count(object->m_name)) 
				{
					PushNodeMarker(object, parentName);
					found.emplace(object->m_name);
					VisitNodes(set, found, object, object->m_name);
				}
				//Else keep searching for children
				else {
					VisitNodes(set, found, object, parentName);
				}
			}
		}
	}
}

void ScreenArcherMenu::AddBones() 
{
	//make a map of parents
	std::unordered_map<BSFixedString, BoneDisplay::NodeMarker*, SAF::BSFixedStringHash, SAF::BSFixedStringKeyEqual> parents;
	for (auto it = boneDisplay.nodes.begin(); it != boneDisplay.nodes.end(); ++it) {
		parents.emplace(it->node->m_name, &*it);
	}
	//push start->end bones
	for (auto it = boneDisplay.nodes.begin(); it != boneDisplay.nodes.end(); ++it) {
		auto parent = parents.find(it->parent);
		if (parent != parents.end()) {
			PushBoneMarker(parent->second, &*it);
		}
	}
}

void ScreenArcherMenu::EnableBoneDisplay(SAF::ActorAdjustmentsPtr actorAdjustments)
{
	std::lock_guard lock(boneDisplay.mutex);

	if (boneDisplay.enabled || !menuOptions.boneoverlay)
		return;

	boneDisplay.actor = actorAdjustments;

	boneDisplay.enabled = true;
	boneDisplay.Reserve(actorAdjustments->nodeSets->baseStrings.size() + 1);

	PushNodeMarker(actorAdjustments->root, BSFixedString());

	GFxValue dimension;
	movie->movieRoot->GetVariable(&dimension, "root1.Menu_mc.x");
	boneDisplay.dimensions.x = -dimension.GetNumber();
	movie->movieRoot->GetVariable(&dimension, "root1.Menu_mc.y");
	boneDisplay.dimensions.y = -dimension.GetNumber();
	movie->movieRoot->GetVariable(&dimension, "root1.Menu_mc.stage.stageWidth");
	boneDisplay.dimensions.width = dimension.GetNumber();
	movie->movieRoot->GetVariable(&dimension, "root1.Menu_mc.stage.stageHeight");
	boneDisplay.dimensions.height = dimension.GetNumber();

	SAF::BSFixedStringSet foundNodes;
	VisitNodes(boneDisplay.actor->nodeSets->baseStrings, foundNodes, boneDisplay.actor->root, actorAdjustments->root->m_name);
	AddBones();
	UpdateBoneFilter();
}

void ScreenArcherMenu::GetNodeSet(SAF::BSFixedStringSet* set)
{
	auto menu = GetMenu(&selected, &filterMenuCache);
	if (!menu) {
		*set = boneDisplay.actor->nodeSets->baseStrings;
		return;
	}

	auto filter = GetCachedBoneFilter(menu);
	if (!filter) {
		*set = boneDisplay.actor->nodeSets->baseStrings;
		return;
	}

	for (int i = 0; i < filter->size(); ++i) {
		if ((*filter)[i]) {
			for (auto& kvp : (*menu)[i].second) {
				set->emplace(kvp.first.c_str());
			}
		}
	}
}

void ScreenArcherMenu::UpdateBoneFilter()
{
	SAF::BSFixedStringSet set;
	GetNodeSet(&set);

	for (auto& node : boneDisplay.nodes) {
		node.enabled = set.count(node.node->m_name);
	}
}

void ScreenArcherMenu::DisableBoneDisplay()
{
	std::lock_guard lock(boneDisplay.mutex);

	if (!boneDisplay.enabled)
		return;

	for (auto it = boneDisplay.bones.rbegin(); it != boneDisplay.bones.rend(); ++it) {
		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
	}

	for (auto it = boneDisplay.nodes.rbegin(); it != boneDisplay.nodes.rend(); ++it) {
		it->marker.Invoke("destroy", nullptr, nullptr, 0);
		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
	}

	movie->movieRoot->Invoke("root1.Menu_mc.ClearNodeMarkers", nullptr, nullptr, 0);

	boneDisplay.Clear();
	boneDisplay.enabled = false;
}

void ScreenArcherMenu::EnableRotateDisplay() {
	if (!boneDisplay.rotateMarker && menuOptions.posinggizmo) {
		boneDisplay.rotateMarker = std::make_unique<GFxValue>();
		movie->movieRoot->CreateObject(boneDisplay.rotateMarker.get(), "RotateTool", nullptr, 0);
		movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, boneDisplay.rotateMarker.get(), 1);

		auto hudColor = (NiColor*)gameHudColor.GetUIntPtr();
		GFxValue color[] = {
			GFxValue(hudColor->r == 0 ? 1.0 : 1.0 / hudColor->r),
			GFxValue(hudColor->g == 0 ? 1.0 : 1.0 / hudColor->g),
			GFxValue(hudColor->b == 0 ? 1.0 : 1.0 / hudColor->b)
		};
		boneDisplay.rotateMarker->Invoke("setColor", nullptr, color, 3);
	}
}

void ScreenArcherMenu::DisableRotateDisplay() {
	if (boneDisplay.rotateMarker) {
		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, boneDisplay.rotateMarker.get(), 1);
		boneDisplay.rotateMarker = nullptr;
	}
}

void SetRotatedDisplayInfo(GFxValue& value, float x1, float x2, float y1, float y2)
{
	GFxValue::DisplayInfo info;
	value.GetDisplayInfo(&info);

	double xDif = x2 - x1;
	double yDif = y2 - y1;
	double distance = std::sqrt((xDif * xDif) + (yDif * yDif));

	double angle;
	if (yDif < 0.0) {
		angle = std::asin(-xDif / distance) + MATH_PI;
	}
	else {
		angle = std::asin(xDif / distance);
	}

	info.SetScale(100, distance);
	info.SetPosition(x1, y1);
	info.SetRotation(angle * -SAF::RADIAN_TO_DEGREE);

	value.SetDisplayInfo(&info);
}

typedef void(*_NiMatrix3ToEulerAnglesZXY2)(NiMatrix43* matrix, float* x, float* y, float* z);
RelocAddr<_NiMatrix3ToEulerAnglesZXY2> NiMatrix3ToEulerAnglesZXY2(0x1B926F0);

typedef void(*_NiMatrix3ToEulerAnglesXZY2)(NiMatrix43* matrix, float* x, float* y, float* z);
RelocAddr<_NiMatrix3ToEulerAnglesXZY2> NiMatrix3ToEulerAnglesXZY2(0x1B907B0);

struct GFxMatrix2x4 {
	//float m[2][4];
	float m[8];
};

typedef void(*_SetDisplayMatrix)(void* unk, GFxValue::ObjectInterface* objInterface, GFxMatrix2x4* matrix);
RelocAddr<_SetDisplayMatrix> SetDisplayMatrix(0x20CF910);

void ScreenArcherMenu::BoneDisplay::Update()
{
	std::lock_guard lock(mutex);

	bool rotateVisible = !!rotateMarker;

	for (auto& it : nodes)
	{
		NiPoint3 outPos;
		WorldToScreen_Internal(&it.node->m_worldTransform.pos, &outPos);

		it.visible = it.enabled && !rotateVisible && (outPos.z >= 0.0);
		GFxValue isVisible(it.visible);
		it.marker.SetMember("visible", &isVisible);

		if (it.visible) {
			GFxValue::DisplayInfo displayInfo;
			it.marker.GetDisplayInfo(&displayInfo);
			displayInfo.SetPosition(dimensions.x + (dimensions.width * outPos.x), dimensions.y + (dimensions.height * (1 - outPos.y)));
			it.marker.SetDisplayInfo(&displayInfo);
		}
	}

	for (auto& it : bones)
	{
		bool boneVisible = (it.start->visible && it.end->visible);

		if (boneVisible) {
			GFxValue::DisplayInfo startInfo, endInfo;
			it.start->marker.GetDisplayInfo(&startInfo);
			it.end->marker.GetDisplayInfo(&endInfo);

			SetRotatedDisplayInfo(it.marker, startInfo._x, endInfo._x, startInfo._y, endInfo._y);
		}

		GFxValue boneVisibleValue(boneVisible);
		it.marker.SetMember("visible", &boneVisibleValue);
	}

	if (rotateVisible) {

		bool visible = !!selectedNode;
		NiPoint3 outPos;
		if (visible) {
			WorldToScreen_Internal(&selectedNode->node->m_worldTransform.pos, &outPos);
			if (outPos.z < 0.0)
				visible = false;
		}

		GFxValue isVisible(visible);
		rotateMarker->SetMember("visible", &isVisible);

		if (visible) {
			GFxValue::DisplayInfo info;
			rotateMarker->GetDisplayInfo(&info);
			info.SetPosition(dimensions.x + (dimensions.width * outPos.x), dimensions.y + (dimensions.height * (1 - outPos.y)));
			rotateMarker->SetDisplayInfo(&info);
		}
	}
}

void ScreenArcherMenu::AdvanceMovie(float unk0, void* unk1)
{
	if (boneDisplay.enabled)
		boneDisplay.Update();

	__super::AdvanceMovie(unk0, unk1);
}

IMenu* CreateScreenArcherMenu()
{
	return new ScreenArcherMenu();
}

void SetBoneDisplay(GFxResult& result, bool enabled) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto wrapped = samManager.GetWrapped();
	if (!wrapped.menu)
		return result.SetError(MENU_MISSING_ERROR);

	auto actorAdjustments = saf->GetActorAdjustments(selected.refr->formID);
	if (!actorAdjustments)
		return result.SetError(SKELETON_ERROR);

	auto menu = (ScreenArcherMenu*)wrapped.menu;

	if (enabled)
		menu->EnableBoneDisplay(actorAdjustments);
	else
		menu->DisableBoneDisplay();
}

void SetRotateTool(GFxResult& result, bool enabled) {
	auto wrapped = samManager.GetWrapped();
	if (!wrapped.menu)
		return result.SetError(MENU_MISSING_ERROR);

	auto menu = (ScreenArcherMenu*)wrapped.menu;

	if (enabled)
		menu->EnableRotateDisplay();
	else
		menu->DisableRotateDisplay();
}

bool ScreenArcherMenu::SelectNode(const char* name) {
	std::lock_guard lock(boneDisplay.mutex);

	BSFixedString nodeName(name);

	auto nodeIt = std::find_if(boneDisplay.nodes.begin(), boneDisplay.nodes.end(), [&nodeName](const BoneDisplay::NodeMarker& marker) {
		return marker.node->m_name == nodeName;
	});

	if (nodeIt == boneDisplay.nodes.end()) {
		boneDisplay.selectedNode = nullptr;
		return false;
	}

	boneDisplay.selectedNode = nodeIt._Ptr;

	return true;
}

void ScreenArcherMenu::UnselectNode() {
	std::lock_guard lock(boneDisplay.mutex);

	boneDisplay.selectedNode = nullptr;
}

void SelectNodeMarker(GFxResult& result, const char* name, bool pushMenu) {
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto wrapped = samManager.GetWrapped();
	if (!wrapped.menu)
		return result.SetError(MENU_MISSING_ERROR);
	auto menu = (ScreenArcherMenu*)wrapped.menu;

	if (!menu->SelectNode(name))
		return result.SetError("Failed to select node");

	if (pushMenu)
		samManager.PushMenu("BoneEdit");
}

void UnselectNodeMarker(GFxResult& result)
{
	auto wrapped = samManager.GetWrapped();
	if (!wrapped.menu)
		return result.SetError(MENU_MISSING_ERROR);
	auto menu = (ScreenArcherMenu*)wrapped.menu;

	menu->UnselectNode();
}

void UpdateBoneFilter()
{
	auto wrapped = samManager.GetWrapped();
	if (!wrapped.menu)
		return;
	auto menu = (ScreenArcherMenu*)wrapped.menu;

	menu->UpdateBoneFilter();
}

NiPoint3 GetCameraPivot()
{
	NiPoint3 pos = selected.refr->pos;
	pos.z += 100;

	auto wrapped = samManager.GetWrapped();
	if (!wrapped.menu)
		return pos;
	auto menu = (ScreenArcherMenu*)wrapped.menu;

	if (menu->boneDisplay.selectedNode)
		return menu->boneDisplay.selectedNode->node->m_worldTransform.pos;

	return pos;
}

IMenuWrapper::IMenuWrapper(IMenu* _menu) {
	menu = _menu;
	if (menu)
		ScaleformRefCountImplAddRef(menu);
}

IMenuWrapper::~IMenuWrapper() {
	if (menu)
		ScaleformRefCountImplRelease(menu);
}

GFxMovieRoot* IMenuWrapper::GetRoot() {
	if (!menu)
		return nullptr;

	if (!((byte)menu->flags & 0x40))
		return nullptr;

	GFxMovieView* view = menu->movie;
	if (!view)
		return nullptr;

	return view->movieRoot;
}

bool IMenuWrapper::IsOpen() {
	if (!menu)
		return false;

	return ((byte)menu->flags & 0x40);
}

IMenuWrapper GetWrappedMenu(BSFixedString name) {
	BSReadLocker lock(g_menuTableLock);

	auto tableItem = (*g_ui)->menuTable.Find(&name);
	if (!tableItem || !tableItem->menuInstance)
		return IMenuWrapper();

	return IMenuWrapper(tableItem->menuInstance);
}

void SetMenuVisible(BSFixedString menuName, const char* visiblePath, bool visible)
{
	auto wrapped = GetWrappedMenu(menuName);
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue isVisible(visible);
	if (!root->SetVariable(visiblePath, &isVisible))
		_Log("Failed to set visibility of menu: ", menuName.c_str());
}

IMenuWrapper SamManager::StoreMenu() {
	std::lock_guard lock(mutex);

	BSReadLocker tableLock(g_menuTableLock);

	BSFixedString samMenuName(SAM_MENU_NAME);
	auto tableItem = (*g_ui)->menuTable.Find(&samMenuName);
	if (!tableItem)
		return IMenuWrapper();

	auto menu = tableItem->menuInstance;
	if (!menu)
		return IMenuWrapper();

	ScaleformRefCountImplAddRef(menu);
	storedMenu = menu;

	return IMenuWrapper(storedMenu);
}

bool SamManager::ReleaseMenu() {
	std::lock_guard lock(mutex);

	if (!storedMenu)
		return false;

	ScaleformRefCountImplRelease(storedMenu);
	storedMenu = nullptr;

	return true;
}

IMenuWrapper SamManager::GetWrapped() {
	std::lock_guard lock(mutex);

	return IMenuWrapper(storedMenu);
}

RelocAddr<bool> bLoadingMenuOpen(0x58D08B3);

void SamManager::OpenOrCloseMenu(const char* menuName) {
	BSFixedString menuStr(SAM_MENU_NAME);
	BSFixedString containerMenu(CONTAINER_MENU_NAME);
	BSFixedString looksMenu(LOOKS_MENU_NAME);

	//should make this a set or something
	if (!(*g_ui)->IsMenuOpen(containerMenu) &&
		!(*g_ui)->IsMenuOpen(looksMenu)) {
		auto wrapped = GetWrapped();

		if (wrapped.IsOpen()) {
			if (!menuName || !_stricmp(storedName.c_str(), menuName)) {
				auto root = wrapped.GetRoot();
				if (root) {

					bool result = false;
					GFxValue gfxResult;
					if (root->Invoke("root1.Menu_mc.TryClose", &gfxResult, nullptr, 0))
						result = gfxResult.GetBool();

					if (result) {
						//*((bool*)bLoadingMenuOpen.GetUIntPtr()) = true;
						CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Close);
					}	
				}
			}
		}
		else {
			if ((*g_ui)->IsMenuRegistered(menuStr)) {
				if (!(*g_ui)->IsMenuOpen(menuStr))
				{
					storedName = menuName ? menuName : MAIN_MENU_NAME;
					CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Open);
				}
			}
			else {
				ShowHudMessage(F4SE_NOT_INSTALLED);
			}
		}
	}
}

void SamManager::ToggleMenu() {
	OpenOrCloseMenu(nullptr);
}

void SamManager::OpenExtensionMenu(const char* extensionName)
{
	OpenOrCloseMenu(extensionName);
}

void SamManager::CloseMenu()
{
	auto wrapped = GetWrapped();
	if (wrapped.IsOpen()) {
		BSFixedString samMenuName(SAM_MENU_NAME);
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(samMenuName, kMessage_Close);
	}
}

bool SamManager::Invoke(const char* func, GFxValue* result, GFxValue* args, UInt32 numArgs)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return false;

	if (!root->Invoke(func, result, args, numArgs)) {
		_Log("Failed to invoke UI function: ", func);
		return false;
	}

	return true;
}

void SamManager::SetVariable(const char* pVarPath, const GFxValue* value, UInt32 setType)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	if (!root->SetVariable(pVarPath, value, setType))
		_Log("Failed to set menu variable", pVarPath);
}

void SamManager::SaveData(GFxValue* saveData)
{
	if (!selected.refr)
		return;

	//copying to a json for saved menu data instead of trying to understand how GFx managed memory works
	data.clear();
	GFxToJsonObjVisitor visitor(data);
	saveData->VisitMembers(&visitor);

	refr = selected.refr;
}

bool SamManager::LoadData(GFxMovieRoot* root, GFxValue* result)
{
	//if ref updated save data is invalidated so ignore
	if (!refr || selected.refr != refr) {
		ClearData();
		return false;
	}

	//Check the saved menu name matches the menu being opened
	if (_stricmp(data.get("rootMenu", "").asCString(), storedName.c_str())) {
		ClearData();
		return false;
	}

	JsonToGFx(root, result, data);

	ClearData();
	return true;
}

void SamManager::ClearData()
{
	refr = nullptr;
}

void SamManager::ForceQuit()
{
	ClearData();

	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	root->Invoke("root1.Menu_mc.CleanUp", nullptr, nullptr, 0);
	BSFixedString samMenuName(SAM_MENU_NAME);
	CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(samMenuName, kMessage_Close);
}

void SamManager::PushMenu(const char* name)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue result;
	GFxValue args(name);
	root->Invoke("root1.Menu_mc.PushMenu", &result, &args, 1);
}

void SamManager::PopMenu()
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.PopMenu", &ret, nullptr, 0);
}

void SamManager::PopMenuTo(const char* name)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	GFxValue args(name);
	root->Invoke("root1.Menu_mc.PopMenuTo", &ret, &args, 1);
}

void SamManager::RefreshMenu()
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.RefreshValues", nullptr, nullptr, 0);
}

void SamManager::UpdateMenu()
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue ret;
	root->Invoke("root1.Menu_mc.ReloadMenu", nullptr, nullptr, 0);
}

void SamManager::ShowNotification(const char* msg, bool store)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args[2];
	args[0] = GFxValue(msg);
	args[1] = GFxValue(true);

	root->Invoke("root1.Menu_mc.ShowNotification", nullptr, args, 2);
}

void SamManager::SetNotification(const char* msg)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.SetNotification(root, msg);
	result.InvokeCallback();
}

void SamManager::SetTitle(const char* title)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.SetTitle(root, title);
	result.InvokeCallback();
}

void SamManager::SetMenuNames(VMArray<BSFixedString>& vmNames)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.CreateNames();

	for (int i = 0; i < vmNames.Length(); i++) {
		BSFixedString name;
		vmNames.Get(&name, i);
		result.PushName(name.c_str());
	}

	result.InvokeCallback();
}

void SamManager::SetMenuValues(VMArray<VMVariable>& vmValues)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);
	result.CreateValues();

	for (int i = 0; i < vmValues.Length(); i++) {
		VMVariable var;
		vmValues.Get(&var, i);

		GFxValue value;
		VMVariableToGFx(root, &value, &var);

		result.PushValue(&value);
	}

	result.InvokeCallback();
}

void SamManager::SetMenuItems(VMArray<BSFixedString>& vmNames, VMArray<VMVariable>& vmValues)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args;
	GFxResult result(root, &args);

	//make sure both arrays are the same length
	//if (vmNames.Length() != vmValues.Length()) {
	//	result.SetError(MENU_ITEM_LENGTH_ERROR);
	//	return result.Invoke(root, "root1.Menu_mc.PapyrusResult");
	//}

	result.CreateMenuItems();

	for (int i = 0; i < vmNames.Length(); ++i) {
		BSFixedString name;
		vmNames.Get(&name, i);
		result.PushName(name.c_str());
	}

	for (int i = 0; i < vmValues.Length(); i++) {


		VMVariable var;
		vmValues.Get(&var, i);

		GFxValue value;
		VMVariableToGFx(root, &value, &var);

		result.PushValue(&value);
	}

	result.InvokeCallback();
}

void SamManager::SetSuccess()
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(root, &value);
	result.InvokeCallback();
}

void SamManager::SetError(const char* error)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue value;
	GFxResult result(root, &value);
	result.SetError(error);
	result.InvokeCallback();
}

void SamManager::SetLocal(const char* key, GFxValue* value)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue args[2];
	args[0] = GFxValue(key);
	args[1] = *value;
	root->Invoke("root1.Menu_mc.SetLocalVariable", nullptr, args, 2);
}

void SamManager::SetVisible(bool isVisible)
{
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root)
		return;

	GFxValue value(isVisible);
	root->SetVariable("root1.Menu_mc.visible", &value);
}

void MenuAlwaysOn(BSFixedString menuStr, bool enabled) {
	auto menu = GetWrappedMenu(menuStr);
	if (menu.IsOpen()) {
		if (enabled)
			menu.menu->flags |= 0x02;
		else
			menu.menu->flags &= ~0x02;
	}
}

bool GetCursor(SInt32* pos)
{
	POINT point;
	bool result = GetCursorPos(&point);
	if (result) {
		pos[0] = point.x;
		pos[1] = point.y;
	}
	return result;
}

bool SetCursor(SInt32 x, SInt32 y)
{
	return SetCursorPos(x, y);
}

void GetCursorPosition(GFxResult& result) {
	result.CreateValues();

	SInt32 pos[2];
	if (!GetCursor(pos))
		return result.SetError("Failed to get cursor position");

	result.PushValue(pos[0]);
	result.PushValue(pos[1]);
}

TESObjectREFR* GetRefr() {
	UInt32 handle = (*g_consoleHandle);
	NiPointer<TESObjectREFR> refr;
	if (handle == 0 || handle == *g_invalidRefHandle) {
		refr = *g_player;
	}
	else {
		LookupREFRByHandle(handle, refr);
		if (!refr || refr->formType != kFormType_ACHR || (refr->flags & TESObjectREFR::kFlag_IsDeleted))
			return nullptr;
	}
	return refr;
}

void SelectedRefr::Update(TESObjectREFR* newRefr) {
	if (!newRefr) {
		Clear();
		return;
	}
	refr = newRefr;
	TESNPC* npc = (TESNPC*)refr->baseForm;
	if (npc) {
		isFemale = (CALL_MEMBER_FN(npc, GetSex)() == 1 ? true : false);
		TESRace* actorRace = refr->GetActorRace();
		race = (actorRace ? actorRace->formID : 0x13746);
	}
	else {
		isFemale = false;
		race = 0x13746;
	}
	key = race;
	if (isFemale)
		key += 0x100000000;
}

void SelectedRefr::Clear() {
	refr = nullptr;
}

void GetMenuTarget(GFxValue& data) {

	if (selectedNonActor.refr)
	{
		const char* name;
		if (selectedNonActor.refr->baseForm) {
			name = selectedNonActor.refr->baseForm->GetFullName();
		}
		else {
			name = selectedNonActor.refr->GetFullName();
		}
		if (name && *name) {
			GFxValue value(name);
			data.SetMember("title", &value);
		}
		else {
			GFxValue value(" ");
			data.SetMember("title", &value);
		}
	}
	else {
		GFxValue value("No Target");
		data.SetMember("title", &value);
	}
}

bool SamManager::OnMenuOpen() {
	auto wrapped = StoreMenu();
	auto root = wrapped.GetRoot();
	if (!root) {
		_DMESSAGE("Failed to get root on menu open");
		return false;
	}

	SetInputEnableLayer(true);
	LockFfc(true);
	if (menuOptions.extrahotkeys)
		(*g_inputMgr)->AllowTextInput(true);

	TESObjectREFR* refr = GetRefr();
	selected.Update(refr);
	UpdateNonActorRefr();

	GFxValue data;
	root->CreateObject(&data);

	GFxValue storedNameValue(storedName.c_str());
	data.SetMember("menuName", &storedNameValue);

	GetMenuTarget(data);

	GFxValue saved;
	if (LoadData(root, &saved))
		data.SetMember("saved", &saved);

	GFxValue widescreen(menuOptions.widescreen);
	data.SetMember("widescreen", &widescreen);

	GFxValue alignment(menuOptions.alignment);
	data.SetMember("swap", &alignment);

	GFxValue extraHotkeys(menuOptions.extrahotkeys);
	data.SetMember("extraHotkeys", &extraHotkeys);

	auto globalJson = GetCachedMenu("Global");
	if (globalJson) {
		GFxValue global;
		JsonToGFx(root, &global, *globalJson);
		data.SetMember("global", &global);
	}

	root->Invoke("root1.Menu_mc.MenuOpened", nullptr, &data, 1);
	GFxValue isMenuVisible(true);
	root->SetVariable("root1.Menu_mc.visible", &isMenuVisible);

	return true;
}

bool SamManager::OnMenuClose() {
	if (menuOptions.extrahotkeys)
		(*g_inputMgr)->AllowTextInput(false);
	SetInputEnableLayer(false);
	LockFfc(false);
	selected.Clear();
	//*((bool*)bLoadingMenuOpen.GetUIntPtr()) = false;

	if (storedCoc)
		CenterOnCell(nullptr, storedCoc);
	storedCoc = nullptr;

	return ReleaseMenu();
}

bool SamManager::OnConsoleUpdate() {
	auto wrapped = GetWrapped();
	auto root = wrapped.GetRoot();
	if (!root) {
		_DMESSAGE("Failed to get menu root on console update");
		return false;
	}

	GFxValue data;
	root->CreateObject(&data);

	UpdateNonActorRefr();
	TESObjectREFR* refr = GetRefr();
	bool refUpdated = false;

	if (selected.refr != refr && menuOptions.hotswap)
	{
		selected.Update(refr);
		refUpdated = true;
	}

	GFxValue updated(refUpdated);
	data.SetMember("updated", &updated);

	GetMenuTarget(data);

	root->Invoke("root1.Menu_mc.ConsoleRefUpdated", nullptr, &data, 1);

	return true;
}

enum {
	kWidgetBoneDisplay,
	kWidgetRotateTool,
};

SAF::InsensitiveUInt32Map widgetTypes {
	{"BoneDisplay", kWidgetBoneDisplay},
	{"RotateTool", kWidgetRotateTool},
};

void SetWidget(GFxResult& result, const char* type, bool enabled)
{
	auto it = widgetTypes.find(type);
	switch (it->second) {
	case kWidgetBoneDisplay: return SetBoneDisplay(result, enabled);
	case kWidgetRotateTool: return SetRotateTool(result, enabled);
	}
}

void SamSerializeSave(const F4SESerializationInterface* ifc)
{
	ifc->OpenRecord('CAM', CAM_SERIALIZE_VERSION);
	SerializeCamera(ifc, CAM_SERIALIZE_VERSION);
	ifc->OpenRecord('LIGH', LIGHTS_SERIALIZE_VERSION);
	SerializeLights(ifc, LIGHTS_SERIALIZE_VERSION);
}

void SamSerializeLoad(const F4SESerializationInterface* ifc)
{
	UInt32 type, length, version;

	while (ifc->GetNextRecordInfo(&type, &version, &length))
	{
		switch (type)
		{
		case 'CAM': DeserializeCamera(ifc, version); break;
		case 'LIGH': DeserializeLights(ifc, version); break;
		}
	}
}

void SamSerializeRevert(const F4SESerializationInterface* ifc)
{
	RevertCamera();
	RevertLights();
	lastSelectedPose.clear();
}

//struct TESEquipEvent {
//	TESObjectREFR* source;
//	UInt32 formId;
//};
//
//DECLARE_EVENT_DISPATCHER(TESEquipEvent, 0x00442870)