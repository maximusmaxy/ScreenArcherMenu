#include "camera.h"

#include "f4se/NiNodes.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "saf/conversions.h"
using namespace SAF;

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

void GetCameraGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return;

	result->PushBack(&GFxValue(state->x));
	result->PushBack(&GFxValue(state->y));
	result->PushBack(&GFxValue(state->z));
	result->PushBack(&GFxValue(state->yaw * RADIAN_TO_DEGREE));
	float adjustedPitch = (state->pitch < MATH_PI) ? state->pitch : (MATH_PI * -2) + state->pitch;
	result->PushBack(&GFxValue(adjustedPitch * RADIAN_TO_DEGREE));
	result->PushBack(&GFxValue(GetCameraRoll() * RADIAN_TO_DEGREE));
	result->PushBack(&GFxValue(*cameraFOV));

	for (UInt32 i = 0; i < CAM_SAVE_STATE_SLOTS; ++i)
	{
		result->PushBack(&GFxValue(i));
	}

	for (UInt32 i = 0; i < CAM_SAVE_STATE_SLOTS; ++i)
	{
		result->PushBack(&GFxValue(i));
	}
}

bool SetCameraPos(float x, float y, float z)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	state->x = x;
	state->y = y;
	state->z = z;

	return true;
}

bool SetCameraRot(float yaw, float pitch, float roll)
{
	FreeCameraState* state = GetFreeCameraState();
	if (!state)
		return false;

	state->yaw = yaw * DEGREE_TO_RADIAN;
	float adjustedPitch = (pitch >= 0) ? pitch : pitch + 360;
	state->pitch = adjustedPitch * DEGREE_TO_RADIAN;
	SetCameraRoll(roll * DEGREE_TO_RADIAN);

	return true;
}

void SetFOV(float fov)
{
	*cameraFOV = fov;
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