#pragma once

#include "f4se/GameCamera.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/PluginAPI.h"
#include "f4se/NiTypes.h"

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
bool SaveCameraState(SInt32 index);
bool LoadCameraState(SInt32 index);

bool SaveCameraFile(const char* file);
bool LoadCameraFile(const char* file);
bool LoadCameraPath(const char* path);
void LoadCameraGFx(GFxResult& result, const char* path);

void SerializeCamera(const F4SESerializationInterface* ifc, UInt32 version);
void DeserializeCamera(const F4SESerializationInterface* ifc, UInt32 version);
void RevertCamera();

NiTransform GetCameraTransform(FreeCameraState* state);
void SetCameraTransform(FreeCameraState* state, NiTransform& transform);

void RotateCameraAroundTransform(NiTransform& transform, float x, float y);
void PanCamera(float x, float y);

void UpdateCameraRotation(GFxResult& result, float x, float y);
void UpdateCameraPan(GFxResult& result, float x, float y);
void UpdateCameraZoom(GFxResult& result, float x);