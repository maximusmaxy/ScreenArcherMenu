#include "lights.h"

#include "f4se/GameData.h"
#include "f4se/GameForms.h"
#include "f4se/GameTypes.h"
#include "f4se/GameHandle.h"
#include "f4se/NiTypes.h"
#include "f4se/NiNodes.h"
#include "f4se/NiObjects.h"
#include "f4se/NiProperties.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameRTTI.h"

#include "f4se_common/Relocation.h"
#include "f4se/Serialization.h"
using namespace Serialization;

#include "common/IFileStream.h"

#include "constants.h"
#include "sam.h"
#include "papyrus.h"
#include "positioning.h"
#include "io.h"
#include "forms.h"

#include "SAF/util.h"
#include "SAF/conversions.h"
#include "SAF/io.h"
using namespace SAF;

LightManager lightManager;

//false
//typedef void (*_DisableInternal)(TESObjectREFR* refr, bool unk2);
//RelocAddr< _DisableInternal> DisableInternal(0x4E4420);

//true
//typedef void (*_MarkForDeleteInternal)(TESObjectREFR* refr, bool unk2);
//RelocAddr<_MarkForDeleteInternal> MarkForDeleteInternal(0x4115C0);

//void MenuLight::SetPos(char axis, float value)
//{
//	TranslationParam p1;
//	TranslationParam p2;
//
//	p1.axis = axis;
//	p2.value = value;
//	UpdateTranslationInternal(*unkTranslation, 0x1007, lightRefr, p1, p2);
//}
//
//void MenuLight::SetRot(char axis, float value)
//{
//	TranslationParam p1;
//	TranslationParam p2;
//
//	p1.axis = axis;
//	p2.value = value;
//	UpdateTranslationInternal(*unkTranslation, 0x1009, lightRefr, p1, p2);
//}

enum {
	kLightDistance = 0,
	kLightRotation,
	kLightHeight,
	kLightX,
	kLightY,
};

enum {
	kLightSettingsX = 0,
	kLightSettingsY,
	kLightSettingsZ,
	kLightSettingsRotation
};

TESObjectREFR* MenuLight::GetRefr()
{
	if (!lightRefr || !formId)
		return nullptr;

	TESForm* lookupForm = LookupFormByID(formId);
	if (lookupForm != lightRefr)
		return nullptr;

	if (lookupForm->flags & TESForm::kFlag_IsDeleted)
		return nullptr;

	return lightRefr;
}

std::pair<std::string, std::string>* GetLightPair(UInt32 formId)
{
	for (auto& categories : lightsMenuCache) {
		for (auto& light : categories.second) {
			UInt32 menuId = HexStringToUInt32(light.first.c_str());
			if (menuId == formId) {
				return &light;
			}
		}
	}

	return nullptr;
}

float DistanceBetweenPoints(NiPoint3& a, NiPoint3& b)
{
	float xDif = b.x - a.x;
	float yDif = b.y - a.y;
	return std::sqrt((xDif * xDif) + (yDif * yDif));
}

//TODO convert to unit circle https://www.mathsisfun.com/geometry/unit-circle.html
float CurrentRotation(NiPoint3& a, NiPoint3& b, float distance)
{
	if (distance == 0.0f)
		return 0.0f;

	//find the 2 sets of radians between 0 and 360 for each axis and see where they intersect
	float asin1 = std::asin((b.x - a.x) / distance);
	float asin2 = MATH_PI - asin1;
	if (asin1 < 0.0)
		asin1 += DOUBLE_PI;

	float acos1 = std::acos((b.y - a.y) / distance);
	float acos2 = DOUBLE_PI - acos1;

	if (FloatEqual(asin1, acos1) || FloatEqual(asin1, acos2))
		return asin1;
	else if (FloatEqual(asin2, acos1) || FloatEqual(asin2, acos2))
		return asin2;
	
	return 0.0f;
}

void MenuLight::Initialize()
{
	TESObjectREFR* target = selected.refr;
	if (!target)
		target = *g_player;

	TESObjectREFR* refr = GetRefr();
	if (!refr)
		return;

	//adjust for global position
	NiPoint3 pos = target->pos + lightManager.pos;

	distance = DistanceBetweenPoints(pos, refr->pos);
	//player relative
	//rotation = Modulo(CurrentRotation(pos, refr->pos, distance) - target->rot.z - lightManager.rot, DOUBLE_PI);
	//world relative
	rotation = Modulo(CurrentRotation(pos, refr->pos, distance) - lightManager.rot, DOUBLE_PI);
	height = refr->pos.z - pos.z;

	//Convert to RPY for correct order of rotations
	NiPoint3 rot = YPRToRPY(refr->rot);

	//player relative
	//xOffset = Modulo(-rotation - rot.x - HALF_PI - target->rot.z - lightManager.rot, DOUBLE_PI);
	//world relative
	xOffset = Modulo(-rotation - rot.x - HALF_PI - lightManager.rot, DOUBLE_PI);
	if (xOffset >= MATH_PI)
		xOffset -= DOUBLE_PI;
	float pitch = distance == 0.0f ? 0.0f : std::atan((height - 100) / distance);
	yOffset = pitch - rot.y;
}

void MenuLight::Update()
{
	TESObjectREFR* target = selected.refr;
	if (!target)
		target = *g_player;

	//player relative
	//float adjustedRot = Modulo(rotation + lightManager.rot + target->rot.z, DOUBLE_PI);
	//world relative
	float adjustedRot = Modulo(rotation + lightManager.rot, DOUBLE_PI);

	NiPoint3 pos(
		std::sin(adjustedRot) * distance + target->pos.x + lightManager.pos.x,
		std::cos(adjustedRot) * distance + target->pos.y + lightManager.pos.y,
		target->pos.z + height + lightManager.pos.z
	);

	//Do rotations in RPY for correct order and convert back YPR
	NiPoint3 rot(
		Modulo(-adjustedRot - HALF_PI, DOUBLE_PI) - xOffset,
		std::atan((height - 100) / distance) - yOffset,
		0.0f
	);

	NiPoint3 convertedRot = RPYToYPR(rot);

	MoveTo(pos, convertedRot);
}

//void MenuLight::MoveTo(NiPoint3& pos, NiPoint3& rot)
//{
//	TESObjectREFR* target = selected.refr;
//	if (!target)
//		target = *g_player;
//
//	UInt32 nullHandle = *g_invalidRefHandle;
//	TESObjectCELL* selectedCell = target->parentCell;
//	TESWorldSpace* selectedWorldspace = CALL_MEMBER_FN(target, GetWorldspace)();
//
//	//Only move to the players cell
//	TESObjectREFR* player = *g_player;
//	TESObjectCELL* playerCell = player->parentCell;
//	TESWorldSpace* playerWorldspace = CALL_MEMBER_FN(player, GetWorldspace)();
//
//	if (selectedCell != playerCell || selectedWorldspace != playerWorldspace)
//		return;
//
//	MoveRefrToPosition(lightRefr, &nullHandle, selectedCell, selectedWorldspace, &pos, &rot);
//}

void MenuLight::MoveTo(NiPoint3& pos, NiPoint3& rot)
{
	//This uses the same method as the console commands setpos/setangle. The internal MoveRefrToPosition was causing 
	//crashes when a light was moved inside the player (Collision related?)
	TranslationParam p1;
	TranslationParam p2;

	p1.axis = 'X';
	p2.value = pos.x;
	UpdateTranslationInternal(*unkTranslation, 0x1007, lightRefr, p1, p2);
	p1.axis = 'Y';
	p2.value = pos.y;
	UpdateTranslationInternal(*unkTranslation, 0x1007, lightRefr, p1, p2);
	p1.axis = 'Z';
	p2.value = pos.z;
	UpdateTranslationInternal(*unkTranslation, 0x1007, lightRefr, p1, p2);

	p1.axis = 'X';
	p2.value = rot.x * RADIAN_TO_DEGREE;
	UpdateTranslationInternal(*unkTranslation, 0x1009, lightRefr, p1, p2);
	p1.axis = 'Y';
	p2.value = rot.y * RADIAN_TO_DEGREE;
	UpdateTranslationInternal(*unkTranslation, 0x1009, lightRefr, p1, p2);
	p1.axis = 'Z';
	p2.value = rot.z * RADIAN_TO_DEGREE;
	UpdateTranslationInternal(*unkTranslation, 0x1009, lightRefr, p1, p2);
}

bool MenuLight::GetVisible()
{
	NiNode* root = lightRefr->GetObjectRootNode();
	if (!root)
		return false;

	//TODO: Should rework this so it isn't hard coded to work only with SOE lights
	static const BSFixedString markerStr("Marker");
	NiAVObject* marker = root->GetObjectByName(&markerStr);
	if (!marker)
		return false;

	BSTriShape* shape = marker->GetAsBSTriShape();
	if (!shape)
		return false;

	NiPointer<NiProperty> niProperty = shape->effectState;
	NiAlphaProperty* alphaProperty = (NiAlphaProperty*)niProperty.get();
	if (!alphaProperty)
		return false;

	return alphaProperty->alphaThreshold == 0;
}

void MenuLight::SetVisible(bool visible)
{
	static BSFixedString show("Show");
	static BSFixedString hide("Hide");

	BSFixedString* animation = visible ? &show : &hide;

	PapyrusPlayGamebryoAnimation(lightRefr, animation);
}

bool MenuLight::ToggleVisible()
{
	bool visible = !GetVisible();
	SetVisible(visible);
	return visible;
}

void MenuLight::Rename(const char* newName)
{
	name = newName;
}

bool MenuLight::IsNameUpdated()
{
	auto pair = GetLightPair(lightRefr->baseForm->formID);
	if (!pair)
		return false;

	//stricmp is false if equal
	return (_stricmp(pair->second.c_str(), name.c_str()));
}

void MenuLight::Erase()
{
	PapyrusDelete(lightRefr);
}

MenuLight* LightManager::GetLight(UInt32 id)
{
	if (!lights || id < 0 || id >= lights->size())
		return nullptr;

	if ((*lights)[id].GetRefr())
		return &(*lights)[id];

	return nullptr;
}

void LightManager::ForEach(const std::function<void(MenuLight*)>& functor)
{
	if (lights) {

		for (auto& light : *lights) {
			if (light.GetRefr()) {
				functor(&light);
			}
		}
	}
}

void LightManager::ForEachWithIndex(const std::function<void(MenuLight*,SInt32)>& functor)
{
	if (lights) {
		for (SInt32 i = 0; i < lights->size(); ++i) {
			auto& light = (*lights)[i];
			if (light.GetRefr()) {
				functor(&light, i);
			}
		}
	}
}

//Gets the light list for the current worldspace/cell pair
LightList* LightManager::GetLightList()
{
	TESObjectREFR* player = *g_player;
	if (!player)
		return nullptr;

	TESObjectCELL* cell = player->parentCell;
	if (!cell)
		return nullptr;

	return &lightCache[cell->formID];
}

void LightManager::UpdateLightList()
{
	LightList* previousLights = lights;
	lights = GetLightList();

	//If list not found just use a temp list, they will eventually get lost but it's better than nothing
	if (!lights) {
		lights = &tempLights;

		//If it has updated, clear the list
		if (lights != previousLights) {
			lights->clear();
		}
	}

	//if list is updated, get the current references for each form id, and remove if invalid
	if (lights != previousLights) {
		auto it = lights->begin();
		while (it != lights->end()) {
			TESForm* form = LookupFormByID(it->formId);
			if (form) {
				it->lightRefr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
				++it;
			}
			else {
				it = lights->erase(it);
			}
		}
	}
}

void LightManager::Update()
{
	ForEach([&](MenuLight* light) {
		light->Update();
	});
}

void LightManager::Push(MenuLight light)
{
	if (lights)
		lights->push_back(light);
}

void LightManager::Erase(UInt32 id)
{
	MenuLight* light = GetLight(id);
	if (light)
		light->Erase();
	
	if (lights)
		lights->erase(lights->begin() + id);
}

void LightManager::Clear()
{
	lights = nullptr;
	lightCache.clear();
	tempLights.clear();
	pos = NiPoint3();
	rot = 0.0f;
}

void LightManager::ValidateLights()
{
	if (!lights)
		return;

	auto it = lights->begin();

	while (it != lights->end())
	{
		//if we can get the reference, it's valid
		if (it->GetRefr()) {
			++it;
		}
		else {
			it = lights->erase(it);
		}
	}
}

void LightManager::EraseAll()
{
	ForEach([](MenuLight* light) {
		light->Erase();
	});

	if (lights) 
		lights->clear();
}

bool LightManager::HasRefr(TESObjectREFR* refr)
{
	for (auto& light : *lights) {
		TESObjectREFR* lightRefr = light.GetRefr();
		if (lightRefr == refr)
			return true;
	}
	return false;
}

bool LightManager::GetVisible()
{
	for (auto& light : *lights) {
		TESObjectREFR* lightRefr = light.GetRefr();
		if (lightRefr && !light.GetVisible())
			return false;
	}
	return true;
}

void GetLightSelect(GFxResult& result)
{
	result.CreateMenuItems();

	lightManager.UpdateLightList();
	lightManager.ValidateLights();

	//TODO using the index is unsafe because a light might get deleted while this menu is open. Need better method
	lightManager.ForEachWithIndex([&](MenuLight* light, SInt32 i) {
		result.PushItem(light->name.c_str(), i);
	});

	SInt32 size = lightManager.lights->size();

	result.PushItem(LIGHTS_ADD_NEW, size);
	result.PushItem(LIGHTS_ADD_CONSOLE, size + 1);
	result.PushItem(LIGHTS_GLOBAL_SETTINGS, size + 2);
}

void SetLightSelect(GFxResult& result, SInt32 index)
{
	SInt32 size = lightManager.lights->size();
	if (index < size)
		samManager.PushMenu("LightEdit");
	else if (index == size)
		samManager.PushMenu("LightAddCategories");
	else if (index == size + 1)
	{
		AddLight(result);
		samManager.UpdateMenu();
	}
	else if (index == size + 2)
		samManager.PushMenu("LightGlobal");
	else
		result.SetError(LIGHT_INDEX_ERROR);
}

void GetLightEdit(GFxResult& result, UInt32 selectedLight)
{
	MenuLight* light = lightManager.GetLight(selectedLight);
	if (!light)
		return result.SetError("Light index out of range");

	result.CreateValues();

	result.PushValue(light->distance);
	result.PushValue(light->rotation * RADIAN_TO_DEGREE);
	result.PushValue(light->height);
	result.PushValue(light->xOffset * RADIAN_TO_DEGREE);
	result.PushValue(light->yOffset * RADIAN_TO_DEGREE);
}

void GetLightCategories(GFxResult& result)
{
	result.CreateMenuItems();

	for (SInt32 i = 0; i < lightsMenuCache.size(); ++i) {
		result.PushItem(lightsMenuCache[i].first.c_str(), i);
	}
}

void GetLightForms(GFxResult& result, SInt32 categoryIndex)
{
	if (categoryIndex < 0 || categoryIndex >= lightsMenuCache.size())
		return result.SetError("Light category index out of range");

	result.CreateMenuItems();

	for (auto& kvp : lightsMenuCache[categoryIndex].second) {
		UInt32 formId = HexStringToUInt32(kvp.first.c_str());
		result.PushItem(kvp.second.c_str(), formId);
	}
}

void GetLightSettings(GFxResult& result)
{
	result.CreateValues();
	result.PushValue(lightManager.pos.x);
	result.PushValue(lightManager.pos.y);
	result.PushValue(lightManager.pos.z);
	result.PushValue(lightManager.rot * RADIAN_TO_DEGREE);
}

MenuLight CreateLightFromId(UInt32 formId)
{
	MenuLight light;

	TESObjectREFR* refr = selected.refr;
	if (!refr)
		refr = *g_player;

	TESForm* form = LookupFormByID(formId);
	if (!form)
		return light;

	TESObjectREFR* out;
	UInt32* handle = PlaceAtMeInternal(&out, refr, form, 1, 0, 0, false, false);

	NiPointer<TESObjectREFR> lightRefr;
	GetREFRFromHandle(handle, lightRefr);

	if (!lightRefr)
		return light;

	light.lightRefr = lightRefr;
	light.formId = lightRefr->formID;

	return light;
}

MenuLight CreateLightFromMenu(UInt32 formId)
{
	MenuLight light = CreateLightFromId(formId);
	if (light.formId == 0)
		return light;

	auto pair = GetLightPair(formId);
	if (!pair)
		return light;
	
	light.name = pair->second;

	return light;
}

void CreateLight(GFxResult& result, UInt32 formId)
{
	MenuLight light = CreateLightFromMenu(formId);
	if (!light.formId)
		return result.SetError(LIGHT_FORM_ERROR);

	light.distance = 100;
	light.rotation = 0;
	light.height = 100;
	light.xOffset = 0;
	light.yOffset = 0;
	light.Update();

	lightManager.Push(light);
}

void AddLight(GFxResult& result)
{
	if (!selectedNonActor.refr)
		return result.SetError("Console target could not be found");

	if (selectedNonActor.refr->baseForm->GetFormType() != kFormType_LIGH)
		return result.SetError("Console target is not a light");

	if (lightManager.HasRefr(selectedNonActor.refr))
		return result.SetError("Light has already been added");

	auto pair = GetLightPair(selectedNonActor.refr->baseForm->formID);
	if (!pair)
		return result.SetError("This light is not manageable by SAM");

	MenuLight light(selectedNonActor.refr, pair->second);
	light.Initialize();

	lightManager.Push(light);

	return;
}

void EditLight(GFxResult& result, UInt32 type, float value, SInt32 selectedLight)
{
	MenuLight* light = lightManager.GetLight(selectedLight);
	if (!light)
		return result.SetError(LIGHT_INDEX_ERROR);

	switch (type)
	{
	case kLightDistance: 
		light->distance += value;
		if (light->distance < 0)
			light->distance = 0.0f;
		break;
	case kLightRotation:
		light->rotation = Modulo(light->rotation + (value * DEGREE_TO_RADIAN), 360.0f * DEGREE_TO_RADIAN); 
		break;
	case kLightHeight: light->height += value; break;
	case kLightX: light->xOffset = value * DEGREE_TO_RADIAN; break;
	case kLightY: light->yOffset = value * DEGREE_TO_RADIAN; break;
	}
	
	light->Update();
}

void ResetLight(GFxResult& result, SInt32 selectedLight)
{
	MenuLight* light = lightManager.GetLight(selectedLight);
	if (!light)
		return result.SetError(LIGHT_INDEX_ERROR);

	light->distance = 100;
	light->rotation = 0;
	light->height = 100;
	light->xOffset = 0;
	light->yOffset = 0;

	light->Update();
}

void RenameLight(GFxResult& result, const char* name, SInt32 selectedLight)
{
	MenuLight* light = lightManager.GetLight(selectedLight);
	if (!light)
		return result.SetError(LIGHT_INDEX_ERROR);

	light->Rename(name);
}

void SwapLight(GFxResult& result, UInt32 formId, SInt32 selectedLight)
{
	MenuLight* light = lightManager.GetLight(selectedLight);
	if (!light)
		return result.SetError(LIGHT_INDEX_ERROR);

	MenuLight newLight = CreateLightFromMenu(formId);
	if (!newLight.formId)
		return result.SetError(LIGHT_FORM_ERROR);

	NiPoint3 pos = light->lightRefr->pos;
	NiPoint3 rot = light->lightRefr->rot;

	//Only keep name if updated otherwise use default
	if (light->IsNameUpdated()) {
		light->name = newLight.name;
	}

	light->Erase();
	light->lightRefr = newLight.lightRefr;
	light->formId = newLight.formId;
	light->MoveTo(pos, rot);
}

bool GetLightVisible(SInt32 id)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return true;

	return light->GetVisible();
}

bool ToggleLightVisible(SInt32 id)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return true;

	return light->ToggleVisible();
}

void EditLightSettings(GFxResult& result, UInt32 type, float value)
{
	switch (type) {
	case kLightSettingsX: lightManager.pos.x += value; break;
	case kLightSettingsY: lightManager.pos.y += value; break;
	case kLightSettingsZ: lightManager.pos.z += value; break;
	case kLightSettingsRotation: 
		lightManager.rot = Modulo(lightManager.rot + (value * DEGREE_TO_RADIAN), 360.0f * DEGREE_TO_RADIAN); 
		break;
	}

	lightManager.Update();
}

void ResetLightSettings()
{
	lightManager.pos.x = 0;
	lightManager.pos.y = 0;
	lightManager.pos.z = 0;
	lightManager.rot = 0;
	lightManager.Update();
}

void RecalculateLight(SInt32 selectedLight)
{
	MenuLight* light = lightManager.GetLight(selectedLight);
	if (!light)
		return;

	light->Initialize();
}

void DeleteLight(GFxResult& result, SInt32 selectedLight)
{
	lightManager.Erase(selectedLight);
}

void UpdateAllLights()
{
	lightManager.ForEach([](MenuLight* light) {
		light->Update();
	});
}

bool GetAllLightsVisible()
{
	return lightManager.GetVisible();
}

bool ToggleAllLightsVisible()
{
	bool visible = !lightManager.GetVisible();

	lightManager.ForEach([&](MenuLight* light) {
		light->SetVisible(visible);
	});

	return visible;
}

void DeleteAllLights()
{
	lightManager.EraseAll();
}

bool SaveLightsJson(const char* filename)
{
	Json::Value value;
	Json::Value lights(Json::ValueType::arrayValue);
	value["name"] = filename;
	value["version"] = 1;

	char buffer[FLOAT_BUFFER_LEN];
	WriteJsonFloat(value, "x", lightManager.pos.x, buffer, "%.06f");
	WriteJsonFloat(value, "y", lightManager.pos.y, buffer, "%.06f");
	WriteJsonFloat(value, "z", lightManager.pos.z, buffer, "%.06f");
	WriteJsonFloat(value, "rotation", lightManager.rot, buffer, "%.06f");

	lightManager.ForEach([&](MenuLight* light) {
		Json::Value lightValue;
		lightValue["name"] = light->name;
		lightValue["mod"] = GetModName(light->lightRefr->baseForm->formID);
		lightValue["id"] = UInt32ToHexString(GetBaseId(light->lightRefr->baseForm->formID));
		WriteJsonFloat(lightValue, "distance", light->distance, buffer, "%.06f");
		WriteJsonFloat(lightValue, "rotation", light->rotation, buffer, "%.06f");
		WriteJsonFloat(lightValue, "height", light->height, buffer, "%.06f");
		WriteJsonFloat(lightValue, "xoffset", light->xOffset, buffer, "%.06f");
		WriteJsonFloat(lightValue, "yoffset", light->yOffset, buffer, "%.06f");
		lights.append(lightValue);
	});

	value["lights"] = lights;

	std::string path = GetPathWithExtension(LIGHTS_PATH, filename, ".json");

	return WriteJsonFile(path.c_str(), value);
}

bool LoadLightsFile(const char* filename)
{
	std::string path = GetPathWithExtension(LIGHTS_PATH, filename, ".json");

	return LoadLightsPath(path.c_str());
}

bool LoadLightsPath(const char* path)
{
	if (!selected.refr)
		return false;

	Json::Value value;
	if (!ReadJsonFile(path, value))
		return false;

	lightManager.EraseAll();

	lightManager.pos.x = ReadJsonFloat(value, "x", 0.0f);
	lightManager.pos.y = ReadJsonFloat(value, "y", 0.0f);
	lightManager.pos.z = ReadJsonFloat(value, "z", 0.0f);
	lightManager.rot = ReadJsonFloat(value, "rotation", 0.0f);

	Json::Value lights = value["lights"];

	for (auto& light : lights)
	{
		std::string mod = light.get("mod", "").asString();
		std::string id = light.get("id", "").asString();

		UInt32 formId = GetFormId(mod.c_str(), id.c_str());
		MenuLight menuLight = CreateLightFromId(formId);
		
		if (menuLight.formId != 0) {
			menuLight.name = light.get("name", "Light").asString();
			menuLight.distance = ReadJsonFloat(light, "distance", 100.0f);
			menuLight.rotation = ReadJsonFloat(light, "rotation", 0.0f);
			menuLight.height = ReadJsonFloat(light, "height", 100.0f);
			menuLight.xOffset = ReadJsonFloat(light, "xoffset", 0.0f);
			menuLight.yOffset = ReadJsonFloat(light, "yoffset", 0.0f);
			menuLight.Update();
			lightManager.Push(menuLight);
		}
	}

	return true;
}

/*
* LIGH Version 1
* 
* X
* Y
* Z
* Rotation
* Size
*   formId
*   name
*   distance
*   rotation
*   height
*   xoffset
*   yoffset
* 
* LIGH Version 2 - Updated to include the cell map
* 
* X
* Y
* Z
* Rotation
* 
* Cell Size
*	Cell Id
*   Lightlist Size
*     formId
*     name
*     distance
*     rotation
*     height
*     xoffset
*     yoffset
*/

//convert maps into vectors for serialization, clearing out anything that is empty
MenuLightCacheList GetValidLights()
{
	MenuLightCacheList lightCacheList;

	for (auto& cell : lightManager.lightCache)
	{
		if (cell.second.size()) {
			lightCacheList.push_back(cell);
		}
	}

	return lightCacheList;
}

void SerializeLights(const F4SESerializationInterface* ifc, UInt32 version)
{
	lightManager.ValidateLights();

	auto lightLists = GetValidLights();

	if (version == LIGHTS_SERIALIZE_VERSION) {
		WriteData<float>(ifc, &lightManager.pos.x);
		WriteData<float>(ifc, &lightManager.pos.y);
		WriteData<float>(ifc, &lightManager.pos.z);
		WriteData<float>(ifc, &lightManager.rot);

		UInt32 cellSize = lightLists.size();
		WriteData<UInt32>(ifc, &cellSize);

		for (int c = 0; c < cellSize; ++c) {
			UInt32 cellId = lightLists[c].first;
			WriteData<UInt32>(ifc, &cellId);

			UInt32 lightListSize = lightLists[c].second.size();
			WriteData<UInt32>(ifc, &lightListSize);
			for (int i = 0; i < lightListSize; ++i) {

				auto light = &lightLists[c].second[i];
				WriteData<UInt32>(ifc, &light->formId);
				WriteData<std::string>(ifc, &light->name);
				WriteData<float>(ifc, &light->distance);
				WriteData<float>(ifc, &light->rotation);
				WriteData<float>(ifc, &light->height);
				WriteData<float>(ifc, &light->xOffset);
				WriteData<float>(ifc, &light->yOffset);
			}
		}
	}
}

void DeserializeLights(const F4SESerializationInterface* ifc, UInt32 version)
{
	ReadData<float>(ifc, &lightManager.pos.x);
	ReadData<float>(ifc, &lightManager.pos.y);
	ReadData<float>(ifc, &lightManager.pos.z);
	ReadData<float>(ifc, &lightManager.rot);

	switch (version)
	{
	case 1:
	{
		LightList* lightList = lightManager.GetLightList();

		UInt32 size;
		ReadData<UInt32>(ifc, &size);

		for (int i = 0; i < size; ++i)
		{
			MenuLight light;
			UInt32 oldId;

			ReadData<UInt32>(ifc, &oldId);
			ReadData<std::string>(ifc, &light.name);
			ReadData<float>(ifc, &light.distance);
			ReadData<float>(ifc, &light.rotation);
			ReadData<float>(ifc, &light.height);
			ReadData<float>(ifc, &light.xOffset);
			ReadData<float>(ifc, &light.yOffset);
			if (lightList && ifc->ResolveFormId(oldId, &light.formId)) {
				lightList->push_back(light);
			}
		}
		break;
	}
	case 2:
	{
		UInt32 cellSize;
		ReadData<UInt32>(ifc, &cellSize);
		for (int c = 0; c < cellSize; ++c) 
		{
			UInt32 oldCellId, cellId;
			ReadData<UInt32>(ifc, &oldCellId);
			bool cellResolved = ifc->ResolveFormId(oldCellId, &cellId);

			UInt32 lightListSize;
			ReadData<UInt32>(ifc, &lightListSize);
			for (int i = 0; i < lightListSize; ++i) {
				MenuLight light;
				UInt32 oldId;

				ReadData<UInt32>(ifc, &oldId);
				ReadData<std::string>(ifc, &light.name);
				ReadData<float>(ifc, &light.distance);
				ReadData<float>(ifc, &light.rotation);
				ReadData<float>(ifc, &light.height);
				ReadData<float>(ifc, &light.xOffset);
				ReadData<float>(ifc, &light.yOffset);

				//if cell and light form ids resolve successfully
				if (cellResolved && ifc->ResolveFormId(oldId, &light.formId)) {
					lightManager.lightCache[cellId].push_back(light);
				}
			}
		}
		break;
	}
	}
}

void RevertLights()
{
	lightManager.Clear();
}