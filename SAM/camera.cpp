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

void GetCameraGFx(GFxResult& result)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return result.SetError(CAMERA_ERROR);

	result.CreateValues();

	result.PushValue(&GFxValue(state->x));
	result.PushValue(&GFxValue(state->y));
	result.PushValue(&GFxValue(state->z));
	result.PushValue(&GFxValue(state->yaw * RADIAN_TO_DEGREE));
	float adjustedPitch = (state->pitch < MATH_PI) ? state->pitch : (MATH_PI * -2) + state->pitch;
	result.PushValue(&GFxValue(adjustedPitch * RADIAN_TO_DEGREE));
	result.PushValue(&GFxValue(GetCameraRoll() * RADIAN_TO_DEGREE));
	result.PushValue(&GFxValue(*cameraFOV));
	result.PushValue(&GFxValue(0));
	result.PushValue(&GFxValue(1));
	result.PushValue(&GFxValue(2));
	result.PushValue(&GFxValue(0));
	result.PushValue(&GFxValue(1));
	result.PushValue(&GFxValue(2));
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
		float adjustedPitch = state->pitch += value * DEGREE_TO_RADIAN;
		if (adjustedPitch < 0)
			adjustedPitch += 360;
		state->pitch = adjustedPitch;
		break;
	}
	case kCameraRoll: SetCameraRoll(value * DEGREE_TO_RADIAN); break;
	case kCameraFOV: *cameraFOV = value; break;
	}
}

bool SaveCamera(int index)
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

bool LoadCamera(int index)
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

bool SaveCameraPath(const char* path)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	TESObjectREFR* player = *g_player;
	if (!player)
		return false;

	TESObjectCELL* cell = player->parentCell;
	if (!cell)
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
	const char* mod = GetModName(cell->formID);
	if (mod) {
		int form = GetBaseId(cell->formID);
		camera["mod"] = mod;
		camera["form"] = form;
	}

	value["camera"] = camera;

	return SAF::WriteJsonFile(path, value);
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

	//verify cell if available
	Json::Value mod = value.get("mod", Json::Value());
	if (!mod.isNull()) {
		UInt32 formId = GetFormId(mod.asCString(), value.get("form", 0).asInt());
		
		TESObjectREFR* player = *g_player;
		if (player) {
			TESObjectCELL* cell = player->parentCell;
			if (cell) {
				if (formId != cell->formID)
					return false;
			}
		}
	}

	state->x = value.get("x", 0.0f).asFloat();
	state->y = value.get("y", 0.0f).asFloat();
	state->z = value.get("z", 0.0f).asFloat();
	state->yaw = value.get("yaw", 0.0f).asFloat();
	state->pitch = value.get("pitch", 0.0f).asFloat();
	SetCameraRoll(value.get("roll", 0.0f).asFloat());
	*cameraFOV = value.get("fov", 0.0f).asFloat();

	return true;
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