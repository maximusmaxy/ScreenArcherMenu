#pragma once

#include "f4se/GameCamera.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/PluginAPI.h"

#include "gfx.h"

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

void GetCamera(GFxResult& result);
void SetCamera(GFxResult& result, int index, float value);
bool SaveCameraState(int index);
bool LoadCameraState(int index);

bool SaveCameraFile(const char* file);
bool LoadCameraFile(const char* file);
bool LoadCameraPath(const char* path);
void LoadCameraGFx(GFxResult& result, const char* path);

void SerializeCamera(const F4SESerializationInterface* ifc, UInt32 version);
void DeserializeCamera(const F4SESerializationInterface* ifc, UInt32 version);
void RevertCamera();