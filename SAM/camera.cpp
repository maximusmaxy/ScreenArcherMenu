#include "camera.h"

#include "f4se/NiNodes.h"
#include "f4se/Serialization.h"

#include "SAF/util.h"
#include "SAF/io.h"
#include "SAF/conversions.h"
using namespace SAF;

#include <json/json.h>

#include "constants.h"
#include "compatibility.h"
#include "sam.h"

RelocPtr<float> cameraFOV(0x05AC8D28);

CameraSaveState cameraSaveStates[CAM_SAVE_STATE_SLOTS];

NiNode* GetCameraNode()
{
	PlayerCamera* camera = *g_playerCamera;
	if (!camera)
		return nullptr;

	return camera->cameraNode;
}

TESCameraState* GetCurrentCameraState()
{
	PlayerCamera* camera = *g_playerCamera;
	if (!camera)
		return nullptr;

	return camera->cameraState;
}

FreeCameraState* GetFreeCameraState()
{
	PlayerCamera* camera = *g_playerCamera;
	if (!camera)
		return nullptr;

	if (camera->cameraState == camera->cameraStates[PlayerCamera::kCameraState_Free])
		return (FreeCameraState*)camera->cameraState;

	return nullptr;
}

void GetCamera(GFxResult& result)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return result.SetError(CAMERA_ERROR);

	result.CreateValues();

	result.PushValue(state->x);
	result.PushValue(state->y);
	result.PushValue(state->z);
	result.PushValue(state->yaw * RADIAN_TO_DEGREE);
	float adjustedPitch = (state->pitch < MATH_PI) ? state->pitch : (MATH_PI * -2) + state->pitch;
	result.PushValue(adjustedPitch * RADIAN_TO_DEGREE);
	result.PushValue(GetCameraRoll() * RADIAN_TO_DEGREE);
	result.PushValue(*cameraFOV);
	result.PushValue((SInt32)0);
	result.PushValue((SInt32)1);
	result.PushValue((SInt32)2);
	result.PushValue((SInt32)0);
	result.PushValue((SInt32)1);
	result.PushValue((SInt32)2);
}

enum {
	kCameraX = 0,
	kCameraY,
	kCameraZ,
	kCameraYaw,
	kCameraPitch,
	kCameraRoll,
	kCameraFOV
};

void SetCamera(GFxResult& result, int index, float value) 
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return result.SetError(CAMERA_ERROR);

	switch (index) {
	case kCameraX: state->x += value; break;
	case kCameraY: state->y += value; break;
	case kCameraZ: state->z += value; break;
	case kCameraYaw: state->yaw = value * DEGREE_TO_RADIAN; break;
	case kCameraPitch:
	{
		float adjustedPitch = state->pitch = value * DEGREE_TO_RADIAN;
		if (adjustedPitch < 0)
			adjustedPitch += (360 * DEGREE_TO_RADIAN);
		state->pitch = adjustedPitch;
		break;
	}
	case kCameraRoll: SetCameraRoll(value * DEGREE_TO_RADIAN); break;
	case kCameraFOV: *cameraFOV = value; break;
	}
}

bool SaveCameraState(SInt32 index)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	if (index < 0 || index >= CAM_SAVE_STATE_SLOTS)
		return false;

	cameraSaveStates[index].enabled = true;
	cameraSaveStates[index].x = state->x;
	cameraSaveStates[index].y = state->y;
	cameraSaveStates[index].z = state->z;
	cameraSaveStates[index].yaw = state->yaw;
	cameraSaveStates[index].pitch = state->pitch;
	cameraSaveStates[index].roll = GetCameraRoll();
	cameraSaveStates[index].fov = *cameraFOV;

	return true;
}

bool LoadCameraState(SInt32 index)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	if (index < 0 || index >= CAM_SAVE_STATE_SLOTS || !cameraSaveStates[index].enabled)
		return false;

	state->x = cameraSaveStates[index].x;
	state->y = cameraSaveStates[index].y;
	state->z = cameraSaveStates[index].z;
	state->yaw = cameraSaveStates[index].yaw;
	state->pitch = cameraSaveStates[index].pitch;
	SetCameraRoll(cameraSaveStates[index].roll);
	*cameraFOV = cameraSaveStates[index].fov;

	return true;
}

bool SaveCameraFile(const char* file)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	Json::Value value;
	value["version"] = 1;

	char buffer[FLOAT_BUFFER_LEN];

	Json::Value camera;
	WriteJsonFloat(camera, "x", state->x, buffer, "%.06f");
	WriteJsonFloat(camera, "y", state->y, buffer, "%.06f");
	WriteJsonFloat(camera, "z", state->z, buffer, "%.06f");
	WriteJsonFloat(camera, "yaw", state->yaw, buffer, "%.06f");
	WriteJsonFloat(camera, "pitch", state->pitch, buffer, "%.06f");
	WriteJsonFloat(camera, "roll", GetCameraRoll(), buffer, "%.06f");
	WriteJsonFloat(camera, "fov", *cameraFOV, buffer, "%.06f");

	//Try storing the cell, dw if it doesn't actually work
	TESObjectREFR* player = *g_player;
	if (player) {
		TESObjectCELL* parentCell = player->parentCell;
		if (parentCell) {
			const char* mod = GetModName(parentCell->formID);
			if (mod) {
				Json::Value cell;
				cell["mod"] = mod;
				cell["id"] = UInt32ToHexString(GetBaseId(parentCell->formID));
				camera["cell"] = cell;
			}
		}
	}

	value["camera"] = camera;

	std::string path = GetPathWithExtension(CAMERA_PATH, file, ".json");

	return SAF::WriteJsonFile(path.c_str(), value);
}

bool LoadCameraFile(const char* filename)
{
	std::string path = GetPathWithExtension(CAMERA_PATH, filename, ".json");

	return LoadCameraPath(path.c_str());
}

bool LoadCameraPath(const char* path)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	Json::Value value;
	if (!SAF::ReadJsonFile(path, value))
		return false;

	Json::Value camera = value.get("camera", Json::Value());
	if (!camera.isObject())
		return false;

	TESObjectREFR* player = *g_player;

	//verify cell if available
	Json::Value cell = camera.get("cell", Json::Value());
	if (cell.isObject()) {
		Json::Value mod = cell.get("mod", Json::Value());
		Json::Value id = cell.get("id", Json::Value());
		if (mod.isString() && id.isString()) {
			UInt32 formId = GetFormId(mod.asCString(), HexStringToUInt32(id.asCString()));
			if (player) {
				TESObjectCELL* parentCell = player->parentCell;
				if (parentCell) {
					if (formId != parentCell->formID)
						return false;
				}
			}
		}
	}

	//use player as default to prevent void outs
	float x, y, z;
	if (player) {
		x = player->pos.x;
		y = player->pos.y;
		z = player->pos.z;
	}
	else {
		x = y = z = 0.0f;
	}

	state->x = ReadJsonFloat(camera, "x", x);
	state->y = ReadJsonFloat(camera, "y", y);
	state->z = ReadJsonFloat(camera, "z", z);
	state->yaw = ReadJsonFloat(camera, "yaw", 0.0f);
	state->pitch = ReadJsonFloat(camera, "pitch", 0.0f);
	SetCameraRoll(ReadJsonFloat(camera, "roll", 0.0f));
	*cameraFOV = ReadJsonFloat(camera, "fov", 70.0f);

	return true;
}

void LoadCameraGFx(GFxResult& result, const char* path) {
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return result.SetError(CAMERA_ERROR);

	Json::Value value;
	if (!SAF::ReadJsonFile(path, value))
		return result.SetError("Failed to read camera json file");

	Json::Value camera = value.get("camera", Json::Value());
	if (!camera.isObject())
		return result.SetError("Failed to read camera json file");

	TESObjectREFR* player = *g_player;

	//verify cell if available
	Json::Value cell = camera.get("cell", Json::Value());
	if (cell.isObject()) {
		Json::Value mod = cell.get("mod", Json::Value());
		Json::Value id = cell.get("id", Json::Value());
		if (mod.isString() && id.isString()) {
			UInt32 formId = GetFormId(mod.asCString(), HexStringToUInt32(id.asCString()));
			if (player) {
				TESObjectCELL* parentCell = player->parentCell;
				if (parentCell) {
					if (formId != parentCell->formID)
						return result.SetError("Cannot load this camera state from this cell");
				}
			}
		}
	}

	//use player as default to prevent void outs
	float x, y, z;
	if (player) {
		x = player->pos.x;
		y = player->pos.y;
		z = player->pos.z;
	}
	else {
		x = y = z = 0.0f;
	}

	state->x = ReadJsonFloat(camera, "x", x);
	state->y = ReadJsonFloat(camera, "y", y);
	state->z = ReadJsonFloat(camera, "z", z);
	state->yaw = ReadJsonFloat(camera, "yaw", 0.0f);
	state->pitch = ReadJsonFloat(camera, "pitch", 0.0f);
	SetCameraRoll(ReadJsonFloat(camera, "roll", 0.0f));
	*cameraFOV = ReadJsonFloat(camera, "fov", 70.0f);
}

void SerializeCamera(const F4SESerializationInterface* ifc, UInt32 version)
{
	using namespace Serialization;

	if (version == CAM_SERIALIZE_VERSION) {
		UInt32 size = CAM_SAVE_STATE_SLOTS;
		WriteData<UInt32>(ifc, &size);

		for (int i = 0; i < size; ++i)
		{
			WriteData<bool>(ifc, &cameraSaveStates[i].enabled);
			WriteData<float>(ifc, &cameraSaveStates[i].x);
			WriteData<float>(ifc, &cameraSaveStates[i].y);
			WriteData<float>(ifc, &cameraSaveStates[i].z);
			WriteData<float>(ifc, &cameraSaveStates[i].yaw);
			WriteData<float>(ifc, &cameraSaveStates[i].pitch);
			WriteData<float>(ifc, &cameraSaveStates[i].roll);
			WriteData<float>(ifc, &cameraSaveStates[i].fov);
		}
	}
}

void DeserializeCamera(const F4SESerializationInterface* ifc, UInt32 version)
{
	using namespace Serialization;

	if (version == CAM_SERIALIZE_VERSION)
	{
		UInt32 size;
		ReadData<UInt32>(ifc, &size);

		for (int i = 0; i < size; ++i)
		{
			ReadData<bool>(ifc, &cameraSaveStates[i].enabled);
			ReadData<float>(ifc, &cameraSaveStates[i].x);
			ReadData<float>(ifc, &cameraSaveStates[i].y);
			ReadData<float>(ifc, &cameraSaveStates[i].z);
			ReadData<float>(ifc, &cameraSaveStates[i].yaw);
			ReadData<float>(ifc, &cameraSaveStates[i].pitch);
			ReadData<float>(ifc, &cameraSaveStates[i].roll);
			ReadData<float>(ifc, &cameraSaveStates[i].fov);
		}
	}
}

void RevertCamera()
{
	for (int i = 0; i < CAM_SAVE_STATE_SLOTS; ++i)
	{
		cameraSaveStates[i].enabled = false;
	}
}

RelocAddr<_NiMatrix3FromEulerAnglesZXY> NiMatrix3FromEulerAnglesZXY(0x1B92840);
RelocAddr<_NiMatrix3ToEulerAnglesZXY> NiMatrix3ToEulerAnglesZXY(0x1B926F0);

NiTransform GetCameraTransform(FreeCameraState* state) 
{
	NiTransform result;

	NiMatrix3FromEulerAnglesZXY(&result.rot, state->pitch, state->yaw, GetCameraRoll());

	result.pos.x = state->x;
	result.pos.y = state->y;
	result.pos.z = state->z;
	result.scale = 1.0f;

	return result;
}

void SetCameraTransform(FreeCameraState* state, NiTransform& transform) 
{
	float outRoll;
	NiMatrix3ToEulerAnglesZXY(&transform.rot, &state->pitch, &state->yaw, &outRoll);
	SetCameraRoll(outRoll);

	state->x = transform.pos.x;
	state->y = transform.pos.y;
	state->z = transform.pos.z;
}

void UpdateCameraRotation(GFxResult& result, float x, float y)
{
	if (!selected.refr)
		return result.SetError(CONSOLE_ERROR);

	auto camera = GetFreeCameraState();
	if (!camera)
		return result.SetError(CAMERA_ERROR);

	NiPoint3 pos = GetCameraPivot();

	x *= DEGREE_TO_RADIAN;
	y *= DEGREE_TO_RADIAN;

	double xDif = camera->x - pos.x;
	double yDif = camera->y - pos.y;
	double zDif = camera->z - pos.z;
	double distance = std::sqrt((xDif * xDif) + (yDif * yDif));

	double xAngle;

	if (distance != 0) {
		if (yDif < 0.0) {
			xAngle = std::asin(-xDif / distance) + MATH_PI;
		}
		else {
			xAngle = std::asin(xDif / distance);
		}

		xAngle += x;

		camera->x = std::sin(xAngle) * distance + pos.x;
		camera->y = std::cos(xAngle) * distance + pos.y;
	}
	else {
		xAngle = camera->yaw + MATH_PI;
	}

	if (xAngle < 0)
		xAngle += (MATH_PI * 2);

	camera->yaw += x;

	double yAngle = camera->yaw;
	double yAngleOffset = (xAngle + MATH_PI) - camera->yaw;
	double yDistance = std::cos(yAngleOffset) * distance;

	if (yDistance != 0) {
		double hypotenuse = std::sqrt(yDistance * yDistance + zDif * zDif);
		double pAngle = std::atan(zDif / yDistance);
		double vAngle = pAngle + y;

		//check y rotation in bounds
		if (!(y > 0 && (vAngle > MATH_PI * 0.5)) &&
			!(y < 0 && (vAngle < MATH_PI * -0.5))) {

			camera->z = std::sin(vAngle) * hypotenuse + pos.z;

			double pDistance = std::cos(pAngle) * hypotenuse;
			double vDistance = std::cos(vAngle) * hypotenuse;
			double vDif = pDistance - vDistance;

			camera->x += std::sin(yAngle) * vDif;
			camera->y += std::cos(yAngle) * vDif;

			camera->pitch += y;
		}
	}
	else {
		double pitch = camera->pitch + y;
		if (pitch > MATH_PI)
			pitch -= (MATH_PI * 2);

		if (!(y > 0 && pitch > MATH_PI * 0.5) &&
			!(y < 0 && pitch < MATH_PI * - 0.5))
			camera->pitch = pitch;
	}

	samManager.Invoke("root1.Menu_mc.RefreshCamera", nullptr, nullptr, 0);
}

void UpdateCameraPan(GFxResult& result, float x, float y)
{
	auto camera = GetFreeCameraState();
	if (!camera)
		return result.SetError(CAMERA_ERROR);

	NiTransform t = {
		SAF::MatrixIdentity(),
		NiPoint3 {-x, 0, y},
		1
	};

	t = SAF::MultiplyNiTransform((*g_playerCamera)->cameraNode->m_worldTransform, t);

	camera->x = t.pos.x;
	camera->y = t.pos.y;
	camera->z = t.pos.z;

	samManager.Invoke("root1.Menu_mc.RefreshCamera", nullptr, nullptr, 0);
}

void UpdateCameraZoom(GFxResult& result, float x)
{
	auto camera = GetFreeCameraState();
	if (!camera)
		return result.SetError(CAMERA_ERROR);

	NiTransform t = {
		SAF::MatrixIdentity(),
		NiPoint3 {0, x, 0},
		1
	};

	t = SAF::MultiplyNiTransform((*g_playerCamera)->cameraNode->m_worldTransform, t);

	camera->x = t.pos.x;
	camera->y = t.pos.y;
	camera->z = t.pos.z;

	samManager.Invoke("root1.Menu_mc.RefreshCamera", nullptr, nullptr, 0);
}