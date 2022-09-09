#pragma once

#include "f4se/GameCamera.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/PluginAPI.h"

#define CAM_SAVE_STATE_SLOTS 3

class FreeCameraState : public TESCameraState
{
public:
	float x;
	float y;
	float z;
	float pitch;
	float yaw;
};

struct CameraSaveState
{
	bool enabled = false;
	float x;
	float y;
	float z;
	float yaw;
	float pitch;
	float roll;
	float fov;
};

FreeCameraState* GetFreeCameraState();

void GetCameraGFx(GFxMovieRoot* root, GFxValue* result);
bool SetCameraPos(float x, float y, float z);
bool SetCameraRot(float x, float y, float z);
void SetFOV(float fov);
bool SaveCamera(int index);
bool LoadCamera(int index);

void SerializeCamera(const F4SESerializationInterface* ifc);
void DeserializeCamera(const F4SESerializationInterface* ifc);
void RevertCamera();