#pragma once

#include "f4se/GameReferences.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/PluginAPI.h"

#include <vector>
#include <unordered_map>

class MenuLight
{
public:
	UInt32 formId;
	TESObjectREFR* lightRefr;
	std::string name;
	float distance;
	float rotation;
	float height;
	float xOffset;
	float yOffset;

	MenuLight() : formId(0) {}
	MenuLight(TESObjectREFR* refr) : lightRefr(refr), formId(refr->formID) {}
	MenuLight(TESObjectREFR* refr, std::string name) : lightRefr(refr), formId(refr->formID), name(name) {}

	TESObjectREFR* GetRefr();
	void Initialize();
	void Update();

	bool GetVisible();
	void SetVisible(bool visible);
	bool ToggleVisible();
	void Rename(const char* name);
	bool IsNameUpdated();
	void Erase();
	void MoveTo(NiPoint3& pos, NiPoint3& rot);
	void UpdatePosition(NiPoint3& pos, NiPoint3& rot);
};

class LightManager
{
public:
	std::vector<MenuLight> lights;
	NiPoint3 pos;
	float rot;

	MenuLight* GetLight(UInt32 id);
	void ForEach(const std::function<void(MenuLight*)>& functor);

	void Update();
	void Push(MenuLight light);
	void Erase(UInt32 id);
	void Clear();

	bool HasRefr(TESObjectREFR* refr);
	bool GetVisible();
	void ValidateLights();
	void EraseAll();
};

extern LightManager lightManager;
extern std::unordered_map<UInt32, const char*> lightModMap;

void GetLightSelectGFx(GFxMovieRoot* root, GFxValue* result);
void GetLightEditGFx(GFxMovieRoot* root, GFxValue* result, UInt32 id);
void GetLightCategoriesGFx(GFxMovieRoot* root, GFxValue* result);
void GetLightObjectsGFx(GFxMovieRoot* root, GFxValue* result, UInt32 categoryId);
void GetLightSettingsGFx(GFxMovieRoot* root, GFxValue* result);

bool CreateLight(UInt32 categoryId, UInt32 lightId);
void AddLight();
void EditLight(UInt32 id, UInt32 type, float value);
void RenameLight(UInt32 id, const char* name);
bool SwapLight(UInt32 id, UInt32 categoryId, UInt32 lightId);
bool GetLightVisible(UInt32 id);
bool ToggleLightVisible(UInt32 id);
void DeleteLight(UInt32 id);
void ResetLight(UInt32 id);

void EditLightSettings(UInt32 type, float value);
void UpdateAllLights();
bool GetAllLightsVisible();
bool ToggleAllLightsVisible();
void DeleteAllLights();
void ResetLightSettings();

void SaveLightsJson(const char* filename);
void LoadLightsJson(const char* filename);

void SerializeLights(const F4SESerializationInterface* ifc, UInt32 version);
void DeserializeLights(const F4SESerializationInterface* ifc, UInt32 version);
void RevertLights();