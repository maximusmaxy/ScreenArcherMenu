#pragma once

#include "f4se/GameReferences.h"
#include "f4se/ScaleformMovie.h"
#include "f4se/ScaleformValue.h"
#include "f4se/PluginAPI.h"

#include "gfx.h"

#include <vector>
#include <unordered_map>

class MenuLight
{
public:
	UInt32 formId;
	TESObjectREFR* lightRefr;
	std::string name;
	float distance{};
	float rotation{};
	float height{};
	float xOffset{};
	float yOffset{};

	MenuLight() : formId(0), lightRefr(nullptr) {}
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
};

typedef std::vector<MenuLight> LightList;
typedef std::unordered_map<UInt32, LightList> MenuLightCache;
typedef std::vector<std::pair<UInt32, LightList>> MenuLightCacheList;

class LightManager
{
public:
	LightList* lights;
	NiPoint3 pos;
	float rot;

	MenuLightCache lightCache;
	LightList tempLights;

	MenuLight* GetLight(UInt32 id);
	void ForEach(const std::function<void(MenuLight*)>& functor);
	void ForEachWithIndex(const std::function<void(MenuLight*,SInt32)>& functor);

	LightList* GetLightList();
	void UpdateLightList();

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

void GetLightSelect(GFxResult& result);
void SetLightSelect(GFxResult& result, SInt32 index);
void GetLightEdit(GFxResult& result, UInt32 selectedLight);
void GetLightCategories(GFxResult& result);
void GetLightForms(GFxResult& result, SInt32 categoryId);
void GetLightSettings(GFxResult& result);

void CreateLight(GFxResult& result, UInt32 formId);
void AddLight(GFxResult& result);
void EditLight(GFxResult& result, UInt32 type, float value, SInt32 selectedLight);
void RenameLight(GFxResult& result, const char* name, SInt32 selectedLight);
void SwapLight(GFxResult& result, UInt32 formId, SInt32 selectedLight);
bool GetLightVisible(SInt32 selectedLight);
bool ToggleLightVisible(SInt32 selectedLight);
void DeleteLight(GFxResult& result, SInt32 selectedLight);
void ResetLight(GFxResult& result, SInt32 selectedLight);

void EditLightSettings(GFxResult& result, UInt32 type, float value);
void UpdateAllLights();
bool GetAllLightsVisible();
bool ToggleAllLightsVisible();
void DeleteAllLights();
void ResetLightSettings();

bool SaveLightsJson(const char* filename);
bool LoadLightsFile(const char* filename);
bool LoadLightsPath(const char* path);

void SerializeLights(const F4SESerializationInterface* ifc, UInt32 version);
void DeserializeLights(const F4SESerializationInterface* ifc, UInt32 version);
void RevertLights();