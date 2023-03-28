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
#include "scripts.h"
#include "input.h"
#include "gfx.h"
#include "io.h"
#include "data.h"

#include <WinUser.h>
#include <libloaderapi.h>

SAF::SafMessagingInterface* saf;
SelectedRefr selected;
SamManager samManager;

typedef void(*_BSGFxShaderFXTargetSetUseAlphaForDropShadow)(BSGFxShaderFXTarget* target, bool enabled);
RelocAddr<_BSGFxShaderFXTargetSetUseAlphaForDropShadow> BSGFxShaderFXTargetSetUseAlphaForDropShadow(0x20F1E70);

//typedef void(*_BSGFxShaderFXTargetSetBackgroundEnabled)(BSGFxShaderFXTarget* target, bool enabled);
//RelocAddr<_BSGFxShaderFXTargetSetBackgroundEnabled> BSGFxShaderFXTargetSetBackgroundEnabled(0x20F1D10);

//typedef void(*_EnableShadedBackgroundColor)(BSGFxShaderFXTarget* target, NiColor* color, float brightness);
//RelocAddr<_EnableShadedBackgroundColor> EnableShadedBackgroundColor(0x20F1C90);

ScreenArcherMenu::ScreenArcherMenu() : GameMenuBase()
{
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

	if ((*g_inputDeviceMgr)->IsGamepadEnabled())
		flags &= ~IMenu::kFlag_UsesCursor;

	UInt32 movieFlags = 3;
	UInt32 extendedFlags = 3;

	if (CALL_MEMBER_FN((*g_scaleformManager), LoadMovie)(this, movie, SAM_MENU_NAME, SAM_MENU_ROOT, movieFlags))
	{
		stage.SetMember("menuFlags", &GFxValue(flags));
		stage.SetMember("movieFlags", &GFxValue(movieFlags));
		stage.SetMember("extendedFlags", &GFxValue(extendedFlags));

		CreateBaseShaderTarget(filterHolder, stage);
		filterHolder->SetFilterColor(false);
		//BSGFxShaderFXTargetSetUseAlphaForDropShadow(filterHolder, true);
		//(*g_colorUpdateDispatcher)->eventDispatcher.AddEventSink(filterHolder);
		shaderFXObjects.Push(filterHolder);
	}
}

ScreenArcherMenu::~ScreenArcherMenu() 
{

}

void ScreenArcherMenu::RegisterFunctions()
{
	RegisterNativeFunction("PlaySound", 0);
	RegisterNativeFunction("OpenMenu", 1);
	RegisterNativeFunction("CloseMenu", 2);
}

void ScreenArcherMenu::Invoke(Args* args)
{
	switch (args->optionID)
	{
	case 0:
	{
		if (args->numArgs >= 1)
		{
			if (args->args[0].IsString())
				PlayUISound(args->args[0].GetString());
		}
		break;
	}
	case 1:
	{
		if (args->numArgs >= 1)
		{
			if (args->args[0].IsString())
			{
				BSFixedString menuName(args->args[0].GetString());
				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Open);
			}
		}
	}
	
	case 2:
	{
		if (args->numArgs >= 1)
		{
			if (args->args[0].IsString())
			{
				BSFixedString menuName(args->args[0].GetString());
				CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuName, kMessage_Close);
			}
		}
	}
	break;
	default:
		break;
	}
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

ScreenArcherMenu::BoneDisplay::NodeMarker* ScreenArcherMenu::PushNodeMarker(NiAVObject* node) {
	auto& emplaced = boneDisplay.nodes.emplace_back(BoneDisplay::NodeMarker(node));
	movie->movieRoot->CreateObject(&emplaced.marker, "NodeMarker", &GFxValue(node->m_name.c_str()), 1);
	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &emplaced.marker, 1);
	movie->movieRoot->Invoke("root1.Menu_mc.AddNodeMarker", nullptr, &emplaced.marker, 1);

	//emplaced.target = new BSGFxShaderFXTarget(&emplaced.marker);
	//BSGFxShaderFXTargetSetUseAlphaForDropShadow(emplaced.target, true);
	//EnableHUDColor(emplaced.target);

	return &emplaced;
}

void ScreenArcherMenu::PushBoneMarker(BoneDisplay::NodeMarker* start, BoneDisplay::NodeMarker* end) {
	auto& emplaced = boneDisplay.bones.emplace_back(BoneDisplay::BoneMarker(start, end));
	movie->movieRoot->CreateObject(&emplaced.marker, "BoneMarker", nullptr, 0);
	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &emplaced.marker, 1);

	//emplaced.target = new BSGFxShaderFXTarget(&emplaced.marker);
	//BSGFxShaderFXTargetSetUseAlphaForDropShadow(emplaced.target, true);
	//EnableHUDColor(emplaced.target);
}

//void ScreenArcherMenu::PushAxisMarker(NiColor& color, NiTransform& transform) {
//	auto& emplaced = boneDisplay.axes.emplace_back(BoneDisplay::AxisMarker(transform));
//	movie->movieRoot->CreateObject(&emplaced.marker, "AxisMarker", nullptr, 0);
//	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &emplaced.marker, 1);
//
//	emplaced.target = new BSGFxShaderFXTarget(&emplaced.marker);
//	emplaced.target->EnableColorMultipliers(&color, 2.0f);
//}

//void ScreenArcherMenu::InitRotateTool() {
//	boneDisplay.rotateTool = std::make_unique<BoneDisplay::RotateTool>();
//
//	movie->movieRoot->CreateObject(&boneDisplay.rotateTool->tool, "RotateTool", nullptr, 0);
//	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &boneDisplay.rotateTool->tool, 1);
//
//	//boneDisplay.rotateTool->targets[0] = new BSGFxShaderFXTarget(&boneDisplay.rotateTool->tool);
//	//EnableHUDColor(boneDisplay.rotateTool->targets[0]);
//
//	boneDisplay.rotateTool->tool.GetMember("xAxis", &boneDisplay.rotateTool->axis[0]);
//	boneDisplay.rotateTool->tool.GetMember("yAxis", &boneDisplay.rotateTool->axis[1]);
//	boneDisplay.rotateTool->tool.GetMember("zAxis", &boneDisplay.rotateTool->axis[2]);
//
//	NiColor color = GetNiColor(1.0f, 0.0f, 0.0f);
//	boneDisplay.rotateTool->targets[0] = new BSGFxShaderFXTarget(&boneDisplay.rotateTool->axis[0]);
//	boneDisplay.rotateTool->targets[0]->EnableColorMultipliers(&color, 2.0f);
//
//	color = GetNiColor(0.0f, 1.0f, 0.0f);
//	boneDisplay.rotateTool->targets[1] = new BSGFxShaderFXTarget(&boneDisplay.rotateTool->axis[1]);
//	boneDisplay.rotateTool->targets[1]->EnableColorMultipliers(&color, 2.0f);
//
//	color = GetNiColor(0.0f, 0.0f, 1.0f);
//	boneDisplay.rotateTool->targets[2] = new BSGFxShaderFXTarget(&boneDisplay.rotateTool->axis[2]);
//	boneDisplay.rotateTool->targets[2]->EnableColorMultipliers(&color, 2.0f);
//}

//void ScreenArcherMenu::PushRotateMarker(SInt32 axis, NiColor& color, NiTransform& transform) {
//	auto& emplaced = boneDisplay.rotates.emplace_back(BoneDisplay::RotateMarker(transform));
//	movie->movieRoot->CreateObject(&emplaced.marker, "RotateMarker", &GFxValue(axis), 1);
//	movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &emplaced.marker, 1);
//
//	emplaced.target = new BSGFxShaderFXTarget(&emplaced.marker);
//	emplaced.target->EnableColorMultipliers(&color, 2.0f);
//}

void ScreenArcherMenu::VisitNodes(SAF::BSFixedStringSet& set, NiAVObject* parent, BoneDisplay::NodeMarker* start)
{
	NiPointer<NiNode> node(parent->GetAsNiNode());

	if (node) {
		for (UInt32 i = 0; i < node->m_children.m_emptyRunStart; i++) {
			NiPointer<NiAVObject> object(node->m_children.m_data[i]);
			if (object) {

				//if child is found, add markers and recursive call with new parent
				if (set.count(object->m_name)) {
					BoneDisplay::NodeMarker* end = PushNodeMarker(object);
					PushBoneMarker(start, end);
					VisitNodes(set, object, end);
				}

				//Else keep searching for children
				else {
					VisitNodes(set, object, start);
				}
			}
		}
	}
}

void ScreenArcherMenu::EnableBoneDisplay(SAF::ActorAdjustmentsPtr actorAdjustments)
{
	std::lock_guard lock(boneDisplay.mutex);

	if (boneDisplay.enabled)
		return;

	boneDisplay.actor = actorAdjustments;

	boneDisplay.enabled = true;
	boneDisplay.Reserve(actorAdjustments->nodeSets->baseStrings.size() + 1);

	boneDisplay.rootMarker = PushNodeMarker(actorAdjustments->root);
	//Clear root name to prevent selection
	boneDisplay.rootMarker->marker.SetMember("boneName", &GFxValue());

	VisitNodes(boneDisplay.actor->nodeSets->baseStrings, boneDisplay.actor->root, boneDisplay.rootMarker);
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

void RemoveShaderTarget(BSTArray<BSGFxShaderFXTarget*>& targets, BSGFxShaderFXTarget* target)
{
	for (UInt64 i = targets.count - 1; i >= 0; --i) {
		if (target == targets.entries[i]) {
			targets.Remove(i);
			return;
		}
	}
}

void ScreenArcherMenu::DisableBoneDisplay()
{
	std::lock_guard lock(boneDisplay.mutex);

	if (!boneDisplay.enabled)
		return;

	//DisableAxisDisplay();
	//movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &boneDisplay.rotateTool->tool, 1);
	//DisableRotateDisplay();

	//we're shrinking the shader objects array so go backwards

	//for (auto it = boneDisplay.rotates.rbegin(); it != boneDisplay.rotates.rend(); ++it) {
	//	movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
	//}

	for (auto it = boneDisplay.bones.rbegin(); it != boneDisplay.bones.rend(); ++it) {
		//RemoveShaderTarget(shaderFXObjects, it->target);
		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
	}

	for (auto it = boneDisplay.nodes.rbegin(); it != boneDisplay.nodes.rend(); ++it) {
		//RemoveShaderTarget(shaderFXObjects, it->target);
		it->marker.Invoke("destroy", nullptr, nullptr, 0);
		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
	}

	movie->movieRoot->Invoke("root1.Menu_mc.ClearNodeMarkers", nullptr, nullptr, 0);

	boneDisplay.Clear();
	boneDisplay.enabled = false;
}

//void ScreenArcherMenu::EnableAxisDisplay()
//{
//	NiColor red = GetNiColor(1.0f, 0.0f, 0.0f);
//	NiColor green = GetNiColor(0.0f, 1.0f, 0.0f);
//	NiColor blue = GetNiColor(0.0f, 0.0f, 1.0f);
//
//	NiTransform redTransform{ SAF::MatrixIdentity(), NiPoint3(2.0f, 0.0f, 0.0f), 1.0f };
//	NiTransform greenTransform{ SAF::MatrixIdentity(), NiPoint3(0.0f, 2.0f, 0.0f), 1.0f };
//	NiTransform blueTransform{ SAF::MatrixIdentity(), NiPoint3(0.0f, 0.0f, 2.0f), 1.0f };
//
//	boneDisplay.axes.reserve(3);
//	PushAxisMarker(red, redTransform);
//	PushAxisMarker(green, greenTransform);
//	PushAxisMarker(blue, blueTransform);
//}
//
//void ScreenArcherMenu::DisableAxisDisplay()
//{
//	for (auto it = boneDisplay.axes.rbegin(); it != boneDisplay.axes.rend(); ++it) {
//		//RemoveShaderTarget(shaderFXObjects, it->target);
//		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
//	}
//
//	boneDisplay.axes.clear();
//}

//void ScreenArcherMenu::EnableRotateDisplay()
//{
//	NiColor red = GetNiColor(1.0f, 0.0f, 0.0f);
//	NiColor green = GetNiColor(0.0f, 1.0f, 0.0f);
//	NiColor blue = GetNiColor(0.0f, 0.0f, 1.0f);
//
//	NiTransform redTransform { 
//		NiMatrix43 { 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0 },
//		NiPoint3(),
//		1 
//	};
//
//	NiTransform greenTransform = SAF::TransformIdentity();
//
//	NiTransform blueTransform { 
//		NiMatrix43{ 1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0 },
//		NiPoint3(),
//		1
//	};
//
//	boneDisplay.rotates.reserve(3);
//	PushRotateMarker(kAxisX, red, redTransform);
//	PushRotateMarker(kAxisY, green, greenTransform);
//	PushRotateMarker(kAxisZ, blue, blueTransform);
//}
//
//void ScreenArcherMenu::DisableRotateDisplay()
//{
//	for (auto it = boneDisplay.rotates.rbegin(); it != boneDisplay.rotates.rend(); ++it) {
//		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &it->marker, 1);
//	}
//
//	boneDisplay.rotates.clear();
//}

void ScreenArcherMenu::EnableRotateDisplay() {
	if (!boneDisplay.rotateMarker) {
		boneDisplay.rotateMarker = std::make_unique<BoneDisplay::RotateMarker>();
		movie->movieRoot->CreateObject(&boneDisplay.rotateMarker->marker, "RotateTool", nullptr, 0);
		movie->movieRoot->Invoke("root1.Menu_mc.addChild", nullptr, &boneDisplay.rotateMarker->marker, 1);

		auto hudColor = (NiColor*)gameHudColor.GetUIntPtr();
		GFxValue color[] = {
			GFxValue(hudColor->r == 0 ? 1.0 : 1.0 / hudColor->r),
			GFxValue(hudColor->g == 0 ? 1.0 : 1.0 / hudColor->g),
			GFxValue(hudColor->b == 0 ? 1.0 : 1.0 / hudColor->b)
		};
		boneDisplay.rotateMarker->marker.Invoke("setColor", nullptr, color, 3);

		//boneDisplay.rotateMarker->target = new BSGFxShaderFXTarget(&boneDisplay.rotateMarker->marker);
	}
}

void ScreenArcherMenu::DisableRotateDisplay() {
	if (boneDisplay.rotateMarker) {
		movie->movieRoot->Invoke("root1.Menu_mc.removeChild", nullptr, &boneDisplay.rotateMarker->marker, 1);
		boneDisplay.rotateMarker = nullptr;
	}
}

//const char* debugTexts[] = {
//	"root1.Menu_mc.debug1.text1.text",
//	"root1.Menu_mc.debug1.text2.text",
//	"root1.Menu_mc.debug1.text3.text",
//	"root1.Menu_mc.debug1.text4.text",
//};
//
//void ScreenArcherMenu::DrawDebug(SInt32 i, const char* text)
//{
//	GFxValue textValue;
//	movie->movieRoot->CreateString(&textValue, text);
//	movie->movieRoot->SetVariable(debugTexts[i], &textValue);
//}

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
		it.marker.SetMember("visible", &GFxValue(it.visible));

		if (it.visible) {
			GFxValue::DisplayInfo displayInfo;
			it.marker.GetDisplayInfo(&displayInfo);
			displayInfo.SetPosition(SAM_X + (SAM_WIDTH * outPos.x), SAM_Y + (SAM_HEIGHT * (1 - outPos.y)));
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

		it.marker.SetMember("visible", &GFxValue(boneVisible));
	}

	if (rotateVisible) {

		bool visible = !!selectedNode;
		NiPoint3 outPos;
		if (visible) {
			WorldToScreen_Internal(&selectedNode->node->m_worldTransform.pos, &outPos);
			if (outPos.z < 0.0)
				visible = false;
		}

		rotateMarker->marker.SetMember("visible", &GFxValue(visible));

		if (visible) {
			GFxValue::DisplayInfo info;
			rotateMarker->marker.GetDisplayInfo(&info);
			info.SetPosition(SAM_X + (SAM_WIDTH * outPos.x), SAM_Y + (SAM_HEIGHT * (1 - outPos.y)));
			rotateMarker->marker.SetDisplayInfo(&info);
		}

		//for (auto& it : rotates) {

		//	it.marker.SetMember("visible", &GFxValue(visible));
		//	if (visible)
		//	{
		//		NiTransform result = SAF::MultiplyNiTransform(selectedNode->node->m_worldTransform, it.transform);

				//GFxMatrix2x4 matrix {
				//	result.rot.arr[0] * result.scale,
				//	result.rot.arr[4] * result.scale,
				//	0,
				//	(float)info._x,
				//	result.rot.arr[1] * result.scale,
				//	result.rot.arr[5] * result.scale,
				//	0,
				//	(float)info._y
				//};

				//SetDisplayMatrix(nullptr, it.marker.objectInterface, &matrix);

		//		GFxValue matrix[] = {
		//			GFxValue(result.rot.arr[0] * result.scale),
		//			GFxValue(result.rot.arr[1] * result.scale),
		//			GFxValue(result.rot.arr[4] * result.scale),
		//			GFxValue(result.rot.arr[5] * result.scale),
		//			GFxValue(info._x),
		//			GFxValue(info._y)
		//		};

		//		it.marker.Invoke("setTo", nullptr, matrix, sizeof(matrix) / sizeof(GFxValue));
		//	}
		//}

		//rotateTool->tool.SetMember("visible", &GFxValue(true));

		//GFxValue::DisplayInfo info;
		//selectedNode->marker.GetDisplayInfo(&info);

		//auto& rot = selectedNode->node->m_worldTransform.rot.arr;
		//float scale = selectedNode->node->m_worldTransform.scale;
		//GFxValue transform[] = {
		//	GFxValue(rot[0] * scale),
		//	GFxValue(rot[1] * scale),
		//	GFxValue(rot[2] * scale),
		//	GFxValue(rot[4] * scale),
		//	GFxValue(rot[5] * scale),
		//	GFxValue(rot[6] * scale),
		//	GFxValue(rot[8] * scale),
		//	GFxValue(rot[9] * scale),
		//	GFxValue(rot[10] * scale),
		//	GFxValue(info._x),
		//	GFxValue(info._y),
		//	GFxValue(info._z),
		//};

		//rotateTool->tool.Invoke("setTransform", nullptr, transform, sizeof(transform) / sizeof(GFxValue));

		//for (auto& it : axes) {
		//	NiTransform world = SAF::MultiplyNiTransform(selectedNode->node->m_worldTransform, it.transform);
		//	NiPoint3 outPos;
		//	WorldToScreen_Internal(&world.pos, &outPos);

		//	bool axisVisible = outPos.z >= 0.0;
		//	it.marker.SetMember("visible", &GFxValue(axisVisible));

		//	if (axisVisible)
		//		SetRotatedDisplayInfo(it.marker, boneInfo._x, SAM_X + (SAM_WIDTH * outPos.x), boneInfo._y, SAM_Y + (SAM_HEIGHT * (1 - outPos.y)));
		//}
	}
}

//void ScreenArcherMenu::UpdateDebug() {
//
//	auto freeState = GetFreeCameraState();
//	if (freeState) {
//		std::stringstream r;
//		r << "Rot: ";
//		r << freeState->yaw;
//		r << " ";
//		r << freeState->pitch;
//		r << " ";
//		r << GetCameraRoll();
//		DrawDebug(0, r.str().c_str());
//	}
//
//	if (selected.refr) {
//
//		auto playerCam = *g_playerCamera;
//		auto& pos = playerCam->cameraNode->m_worldTransform.pos;
//
//		float x, y, z;
//		SAF::MatrixToEulerYPR(playerCam->cameraNode->m_worldTransform.rot, x, y, z);
//		std::stringstream r;
//		r << "Rot: ";
//		r << x;
//		r << " ";
//		r << y;
//		r << " ";
//		r << z;
//		DrawDebug(1, r.str().c_str());
//
//		NiMatrix3ToEulerAnglesZXY2(&playerCam->cameraNode->m_worldTransform.rot, &x, &y, &z);
//		std::stringstream r2;
//		r2 << "Rot: ";
//		r2 << x;
//		r2 << " ";
//		r2 << y;
//		r2 << " ";
//		r2 << z;
//		DrawDebug(2, r2.str().c_str());
//
//		NiMatrix3ToEulerAnglesXZY2(&playerCam->cameraNode->m_worldTransform.rot, &x, &y, &z);
//		std::stringstream r3;
//		r3 << "Rot: ";
//		r3 << x;
//		r3 << " ";
//		r3 << y;
//		r3 << " ";
//		r3 << z;
//		DrawDebug(3, r3.str().c_str());
//	}
//}

void ScreenArcherMenu::AdvanceMovie(float unk0, void* unk1)
{
	if (boneDisplay.enabled)
		boneDisplay.Update();

	//UpdateDebug();

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

	//if (boneDisplay.selectedNode) {
		//NiColor white = GetNiColor(1.0f, 1.0f, 1.0f);
		//nodeIt->target->EnableColorMultipliers(&white, 1.0f);
	//}
	//else {
		//EnableAxisDisplay();
		//EnableRotateDisplay();
	//}
	
	//SetShaderColor(nodeIt->target, 1.0f, 0.0f, 0.0f);
	boneDisplay.selectedNode = nodeIt._Ptr;

	//auto boneIt = std::find_if(boneDisplay.bones.begin(), boneDisplay.bones.end(), [&nodeIt](const BoneDisplay::BoneMarker& marker) {
	//	return marker.start == nodeIt._Ptr;
	//});

	//dw if this fails
	//if (boneIt != boneDisplay.bones.end()) {
		//SetShaderColor(boneIt->target, 1.0f, 0.0f, 0.0f);
	//	boneDisplay.selectedBone = boneIt._Ptr;
	//}
	//else {
	//	boneDisplay.selectedBone = nullptr;
	//}

	return true;
}

void ScreenArcherMenu::UnselectNode() {
	std::lock_guard lock(boneDisplay.mutex);

	boneDisplay.selectedNode = nullptr;

	//NiColor white = GetNiColor(1.0f, 1.0f, 1.0f);

	//if (boneDisplay.selectedNode) {
		//boneDisplay.selectedNode->target->EnableColorMultipliers(&white, 1.0f);
	//	boneDisplay.selectedNode = nullptr;
	//}

	//if (boneDisplay.selectedBone) {
		//boneDisplay.selectedBone->target->EnableColorMultipliers(&white, 1.0f);
	//	boneDisplay.selectedBone = nullptr;
	//}

	//DisableAxisDisplay();
	//DisableRotateDisplay();
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

	//auto actorAdjustments = saf->GetActorAdjustments(selected.refr->formID);
	//if (!actorAdjustments)
	//	return result.SetError(SKELETON_ERROR);

	//auto it = actorAdjustments->poseMap.find(name);
	//if (it == actorAdjustments->poseMap.end())
	//	return result.SetError("Node not found");

	//auto camera = GetFreeCameraState();
	//if (!camera)
	//	return result.SetError(CAMERA_ERROR);

	//SetCameraTransform(camera, it->second->m_worldTransform);
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

//void OverNodeMarker(GFxResult& result, const char* name) 
//{
//	auto wrapped = samManager.GetWrapped();
//	if (!wrapped.menu)
//		return result.SetError(MENU_MISSING_ERROR);
//	auto menu = (ScreenArcherMenu*)wrapped.menu;
//
//	menu->boneDisplay.hovers.push_back(name);
//	wrapped.GetRoot()->Invoke("root1.Menu_mc.ShowNotification", nullptr, &GFxValue(name), 1);
//}
//
//void OutNodeMarker(GFxResult& result, const char* name) 
//{
//	auto wrapped = samManager.GetWrapped();
//	if (!wrapped.menu)
//		return result.SetError(MENU_MISSING_ERROR);
//	auto menu = (ScreenArcherMenu*)wrapped.menu;
//
//	menu->boneDisplay.hovers.remove(name);
//
//	const char* notif = (menu->boneDisplay.hovers.size() ? menu->boneDisplay.hovers.back() : "");
//	wrapped.GetRoot()->Invoke("root1.Menu_mc.ShowNotification", nullptr, &GFxValue(notif), 1);
//}

typedef void(*_ScaleformRefCountImplAddRef)(IMenu* menu);
RelocAddr<_ScaleformRefCountImplAddRef> ScaleformRefCountImplAddRef(0x210EBF0);

typedef void(*_ScaleformRefCountImplRelease)(IMenu* menu);
RelocAddr<_ScaleformRefCountImplRelease> ScaleformRefCountImplRelease(0x210EC90);

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

	if (!root->SetVariable(visiblePath, &GFxValue(visible)))
		_Log("Failed to set visibility of menu: ", menuName.c_str());
}

//RelocPtr <BSReadWriteLock> g_menuStackLock(0x65774B0);
//
//void RemoveDuplicateIMenusFromStack(IMenu* menu)
//{
//	BSWriteLocker locker(g_menuStackLock);
//
//	auto& stack = (*g_ui)->menuStack;
//
//	int removeIndex;
//	int foundIndex;
//
//	//loop until every duplicate is removed
//	do {
//		removeIndex = -1;
//		foundIndex = -1;
//
//		//find index of duplicate
//		for (int i = 0; i < stack.count; ++i) {
//			if (stack.entries[i] == menu) {
//				if (foundIndex != -1) {
//					removeIndex = i;
//					break;
//				}
//				foundIndex = i;
//			}
//		}
//
//		//remove duplicate if found
//		if (removeIndex != -1) {
//			_DMESSAGE("Duplicate menu on stack found");
//			stack.Remove(removeIndex);
//		}
//
//	} while (removeIndex != -1);
//}

IMenuWrapper SamManager::StoreMenu() {
	std::lock_guard lock(mutex);

	BSReadLocker tableLock(g_menuTableLock);

	auto tableItem = (*g_ui)->menuTable.Find(&BSFixedString(SAM_MENU_NAME));
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

					if (result)
						CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(menuStr, kMessage_Close);
				}
			}
		}
		else {
			if ((*g_ui)->IsMenuRegistered(menuStr)) {
				//should make this a set or something
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
		//RemoveDuplicateIMenusFromStack(wrapped.menu);
		CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(BSFixedString(SAM_MENU_NAME), kMessage_Close);
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
	CALL_MEMBER_FN(*g_uiMessageManager, SendUIMessage)(BSFixedString(SAM_MENU_NAME), kMessage_Close);
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
	root->Invoke("root1.Menu_mc.PopMenuTo", &ret, &GFxValue(name), 1);
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

	root->SetVariable("root1.Menu_mc.visible", &GFxValue(isVisible));
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
			data.SetMember("title", &GFxValue(name));
		}
		else {
			data.SetMember("title", &GFxValue(" "));
		}
	}
	else {
		data.SetMember("title", &GFxValue("No Target"));
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
	(*g_inputMgr)->AllowTextInput(true);
	LockFfc(true);

	TESObjectREFR* refr = GetRefr();
	selected.Update(refr);
	UpdateNonActorRefr();

	GFxValue data;
	root->CreateObject(&data);

	data.SetMember("menuName", &GFxValue(storedName.c_str()));

	GetMenuTarget(data);

	GFxValue saved;
	if (LoadData(root, &saved))
		data.SetMember("saved", &saved);

	GFxValue widescreen(GetMenuOption(kOptionWidescreen));
	data.SetMember("widescreen", &widescreen);

	GFxValue alignment(GetMenuOption(kOptionAlignment));
	data.SetMember("swap", &alignment);

	auto globalJson = GetCachedMenu("Global");
	if (globalJson) {
		GFxValue global;
		JsonToGFx(root, &global, *globalJson);
		data.SetMember("global", &global);
	}

	root->Invoke("root1.Menu_mc.MenuOpened", nullptr, &data, 1);
	root->SetVariable("root1.Menu_mc.visible", &GFxValue(true));

	return true;
}

bool SamManager::OnMenuClose() {
	(*g_inputMgr)->AllowTextInput(false);
	SetInputEnableLayer(false);
	LockFfc(false);
	selected.Clear();
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

	if (selected.refr != refr && GetMenuOption(kOptionHotswap))
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