#include "camera.h"

#include "f4se/NiNodes.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "SAF/util.h"
#include "SAF/io.h"
#include "SAF/conversions.h"
using namespace SAF;

#include "json/json.h"

#include "constants.h"
#include "compatibility.h"

RelocPtr<float> cameraFOV(0x05AC8D28);

CameraSaveState cameraSaveStates[CAM_SAVE_STATE_SLOTS];

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
	result.PushValue(0);
	result.PushValue(1);
	result.PushValue(2);
	result.PushValue(0);
	result.PushValue(1);
	result.PushValue(2);
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

bool SaveCameraState(int index)
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

bool LoadCameraState(int index)
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