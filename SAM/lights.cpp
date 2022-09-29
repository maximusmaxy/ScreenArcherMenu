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

#include "sam.h"
#include "papyrus.h"
#include "positioning.h"

#include "SAF/util.h"
#include "SAF/conversions.h"
#include "SAF/io.h"
using namespace SAF;

LightManager lightManager;
std::unordered_map<UInt32, const char*> lightModMap;

//out, actor, form, 1, 0, 0, false, false
typedef UInt32* (*_PlaceAtMeInternal)(UInt64* out, TESObjectREFR* actor, TESForm* form, int unk4, int unk5, int unk6, bool unk7, bool unk8);
RelocAddr<_PlaceAtMeInternal> PlaceAtMeInternal(0x5121D0);

typedef void (*_GetREFRFromHandle)(UInt32* handle, NiPointer<TESObjectREFR>& refr);
RelocAddr<_GetREFRFromHandle> GetREFRFromHandle(0xAC90);

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
			UInt32 menuId = std::stoul(light.first, nullptr, 16);
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
	if (id < 0 || id >= lights.size())
		return nullptr;

	if (lights[id].GetRefr())
		return &lights[id];

	return nullptr;
}

void LightManager::ForEach(const std::function<void(MenuLight*)>& functor)
{
	for (auto& light : lights) {
		if (light.GetRefr()) {
			functor(&light);
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
	lights.push_back(light);
}

void LightManager::Erase(UInt32 id)
{
	MenuLight* light = GetLight(id);
	if (light)
		light->Erase();

	lights.erase(lights.begin() + id);
}

void LightManager::Clear()
{
	lights.clear();
	pos = NiPoint3();
	rot = 0.0f;
}

void LightManager::ValidateLights()
{
	auto it = lights.begin();

	while (it != lights.end())
	{
		if (it->GetRefr()) {
			++it;
		}
		else {
			it = lights.erase(it);
		}
	}
}

void LightManager::EraseAll()
{
	ForEach([](MenuLight* light) {
		light->Erase();
	});
	lights.clear();
}

bool LightManager::HasRefr(TESObjectREFR* refr)
{
	for (auto& light : lights) {
		TESObjectREFR* lightRefr = light.GetRefr();
		if (lightRefr == refr)
			return true;
	}
	return false;
}

bool LightManager::GetVisible()
{
	for (auto& light : lights) {
		TESObjectREFR* lightRefr = light.GetRefr();
		if (lightRefr && !light.GetVisible())
			return false;
	}
	return true;
}

void GetLightSelectGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	lightManager.ValidateLights();

	lightManager.ForEach([&](MenuLight* light) {
		result->PushBack(&GFxValue(light->name.c_str()));
	});
}

void GetLightEditGFx(GFxMovieRoot* root, GFxValue* result, UInt32 id)
{
	root->CreateArray(result);

	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return;

	result->PushBack(&GFxValue(light->distance));
	result->PushBack(&GFxValue(light->rotation * RADIAN_TO_DEGREE));
	result->PushBack(&GFxValue(light->height));
	result->PushBack(&GFxValue(light->xOffset * RADIAN_TO_DEGREE));
	result->PushBack(&GFxValue(light->yOffset * RADIAN_TO_DEGREE));
}

void GetLightCategoriesGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	for (auto& category : lightsMenuCache) {
		GFxValue name(category.first.c_str());
		result->PushBack(&name);
	}
}

void GetLightObjectsGFx(GFxMovieRoot* root, GFxValue* result, UInt32 categoryId)
{
	root->CreateArray(result);

	if (categoryId < 0 || categoryId >= lightsMenuCache.size())
		return;

	for (auto& light : lightsMenuCache[categoryId].second) {
		result->PushBack(&GFxValue(light.second.c_str()));
	}
}

void GetLightSettingsGFx(GFxMovieRoot* root, GFxValue* result)
{
	root->CreateArray(result);

	result->PushBack(&GFxValue(lightManager.pos.x));
	result->PushBack(&GFxValue(lightManager.pos.y));
	result->PushBack(&GFxValue(lightManager.pos.z));
	result->PushBack(&GFxValue(lightManager.rot * RADIAN_TO_DEGREE));
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

	UInt64 out;
	UInt32* handle = PlaceAtMeInternal(&out, refr, form, 1, 0, 0, false, false);

	NiPointer<TESObjectREFR> lightRefr;
	GetREFRFromHandle(handle, lightRefr);

	if (!lightRefr)
		return light;

	light.lightRefr = lightRefr;
	light.formId = lightRefr->formID;

	return light;
}

MenuLight CreateLightFromMenu(UInt32 categoryId, UInt32 lightId)
{
	UInt32 formId = std::stoul(lightsMenuCache[categoryId].second[lightId].first, nullptr, 16);

	MenuLight light = CreateLightFromId(formId);
	if (light.formId == 0)
		return light;
	
	light.name = lightsMenuCache[categoryId].second[lightId].second;

	return light;
}

bool CreateLight(UInt32 categoryId, UInt32 lightId)
{
	MenuLight light = CreateLightFromMenu(categoryId, lightId);
	if (!light.formId)
		return false;

	light.distance = 100;
	light.rotation = 0;
	light.height = 100;
	light.xOffset = 0;
	light.yOffset = 0;
	light.Update();

	lightManager.Push(light);

	return true;
}

void AddLight()
{
	if (!selectedNonActor.refr)
		return;

	if (selectedNonActor.refr->baseForm->GetFormType() != kFormType_LIGH)
		return;

	//Don't add if already exists
	if (lightManager.HasRefr(selectedNonActor.refr))
		return;

	auto pair = GetLightPair(selectedNonActor.refr->baseForm->formID);
	if (!pair)
		return;

	MenuLight light(selectedNonActor.refr, pair->second);
	light.Initialize();

	lightManager.Push(light);

	return;
}

void EditLight(UInt32 id, UInt32 type, float value)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return;

	switch (type)
	{
	case kLightDistance: light->distance = value; break;
	case kLightRotation: light->rotation = value * DEGREE_TO_RADIAN; break;
	case kLightHeight: light->height = value; break;
	case kLightX: light->xOffset = value * DEGREE_TO_RADIAN; break;
	case kLightY: light->yOffset = value * DEGREE_TO_RADIAN; break;
	}
	
	light->Update();
}

void ResetLight(UInt32 id)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return;

	light->distance = 100;
	light->rotation = 0;
	light->height = 100;
	light->xOffset = 0;
	light->yOffset = 0;

	light->Update();
}

void RenameLight(UInt32 id, const char* name)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return;

	light->Rename(name);
}

bool SwapLight(UInt32 id, UInt32 categoryId, UInt32 lightId)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return false;

	MenuLight newLight = CreateLightFromMenu(categoryId, lightId);
	if (!newLight.formId)
		return false;

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

	return true;
}

bool GetLightVisible(UInt32 id)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return true;

	return light->GetVisible();
}

bool ToggleLightVisible(UInt32 id)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return true;

	return light->ToggleVisible();
}

void EditLightSettings(UInt32 type, float value)
{
	switch (type) {
	case kLightSettingsX: lightManager.pos.x = value; break;
	case kLightSettingsY: lightManager.pos.y = value; break;
	case kLightSettingsZ: lightManager.pos.z = value; break;
	case kLightSettingsRotation: lightManager.rot = value * DEGREE_TO_RADIAN; break;
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

void RecalculateLight(UInt32 id)
{
	MenuLight* light = lightManager.GetLight(id);
	if (!light)
		return;

	light->Initialize();
}

void DeleteLight(UInt32 id)
{
	lightManager.Erase(id);
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

#define LIGHTS_PATH "Data\\F4SE\\Plugins\\SAM\\Lights\\"

void SaveLightsJson(const char* filename)
{
	IFileStream file;

	std::string path = LIGHTS_PATH;
	path += filename;
	path += ".json";

	IFileStream::MakeAllDirs(path.c_str());

	if (!file.Create(path.c_str())) {
		_DMESSAGE("Failed to create file");
		return;
	}

	Json::Value value;
	Json::Value lights(Json::ValueType::arrayValue);
	value["name"] = filename;
	value["version"] = 1;

	char buffer[32];
	WriteJsonFloat(value["x"], lightManager.pos.x, "%.06f");
	WriteJsonFloat(value["y"], lightManager.pos.y, "%.06f");
	WriteJsonFloat(value["z"], lightManager.pos.z, "%.06f");
	WriteJsonFloat(value["rotation"], lightManager.rot, "%.06f");

	lightManager.ForEach([&](MenuLight* light) {
		Json::Value lightValue;
		lightValue["name"] = light->name;
		lightValue["mod"] = lightModMap[GetModId(light->lightRefr->baseForm->formID)];
		lightValue["id"] = HexToString(GetBaseId(light->lightRefr->baseForm->formID));
		WriteJsonFloat(lightValue["distance"], light->distance, "%.06f");
		WriteJsonFloat(lightValue["rotation"], light->rotation, "%.06f");
		WriteJsonFloat(lightValue["height"], light->height, "%.06f");
		WriteJsonFloat(lightValue["xoffset"], light->xOffset, "%.06f");
		WriteJsonFloat(lightValue["yoffset"], light->yOffset, "%.06f");
		lights.append(lightValue);
	});

	value["lights"] = lights;

	Json::StyledWriter writer;
	Json::String jsonString = writer.write(value);
	file.WriteBuf(jsonString.c_str(), jsonString.size() - 1);
	file.Close();
}

void LoadLightsJson(const char* filename)
{
	if (!selected.refr)
		return;

	IFileStream file;

	std::string path = LIGHTS_PATH;
	path += filename;
	path += ".json";

	if (!file.Open(path.c_str()))
		return;

	std::string jsonString;
	ReadAll(&file, &jsonString);
	file.Close();

	Json::Reader reader;
	Json::Value value;
	
	if (!reader.parse(jsonString, value))
		return;

	lightManager.EraseAll();

	lightManager.pos.x = ReadJsonFloat(value["x"]);
	lightManager.pos.y = ReadJsonFloat(value["y"]);
	lightManager.pos.z = ReadJsonFloat(value["z"]);
	lightManager.rot = ReadJsonFloat(value["rotation"]);

	Json::Value lights = value["lights"];

	for (auto& light : lights)
	{
		UInt32 formId = GetFormId(light["mod"].asString(), light["id"].asString());
		MenuLight menuLight = CreateLightFromId(formId);
		
		if (menuLight.formId != 0) {
			menuLight.name = light["name"].asString();
			menuLight.distance = ReadJsonFloat(light["distance"]);
			menuLight.rotation = ReadJsonFloat(light["rotation"]);
			menuLight.height = ReadJsonFloat(light["height"]);
			menuLight.xOffset = ReadJsonFloat(light["xoffset"]);
			menuLight.yOffset = ReadJsonFloat(light["yoffset"]);
			menuLight.Update();
			lightManager.Push(menuLight);
		}
	}
}

/*
* LIGH Version 1
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
*/

void SerializeLights(const F4SESerializationInterface* ifc, UInt32 version)
{
	lightManager.ValidateLights();

	if (version == 1) {
		WriteData<float>(ifc, &lightManager.pos.x);
		WriteData<float>(ifc, &lightManager.pos.y);
		WriteData<float>(ifc, &lightManager.pos.z);
		WriteData<float>(ifc, &lightManager.rot);

		UInt32 size = lightManager.lights.size();
		WriteData<UInt32>(ifc, &size);

		for (int i = 0; i < size; ++i) {
			MenuLight* light = lightManager.GetLight(i);
			if (light) {
				WriteData<UInt32>(ifc, &light->formId);
				WriteData<std::string>(ifc, &light->name);
				WriteData<float>(ifc, &light->distance);
				WriteData<float>(ifc, &light->rotation);
				WriteData<float>(ifc, &light->height);
				WriteData<float>(ifc, &light->xOffset);
				WriteData<float>(ifc, &light->yOffset);
			}
			else {
				//if for some reason it fails after validation ??? just write anyway
				UInt32 zero = 0;
				WriteData<UInt32>(ifc, &zero);
				std::string empty;
				WriteData<std::string>(ifc, &empty);
				float zerof = 0.0f;
				WriteData<float>(ifc, &zerof);
				WriteData<float>(ifc, &zerof);
				WriteData<float>(ifc, &zerof);
				WriteData<float>(ifc, &zerof);
				WriteData<float>(ifc, &zerof);
			}
		}
	}
}

void DeserializeLights(const F4SESerializationInterface* ifc, UInt32 version)
{
	if (version == 1)
	{
		ReadData<float>(ifc, &lightManager.pos.x);
		ReadData<float>(ifc, &lightManager.pos.y);
		ReadData<float>(ifc, &lightManager.pos.z);
		ReadData<float>(ifc, &lightManager.rot);
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
			if (ifc->ResolveFormId(oldId, &light.formId)) {
				TESForm* form = LookupFormByID(light.formId);
				if (form) {
					light.lightRefr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
					if (light.lightRefr) {
						lightManager.Push(light);
					}
				}
			}
		}
	}
}

void RevertLights()
{
	lightManager.Clear();
}

//void ExportLights()
//{
//	IFileStream file;
//	std::string path = "Data\\F4SE\\Plugins\\SAM\\lights.txt";
//	IFileStream::MakeAllDirs(path.c_str());
//
//	if (!file.Create(path.c_str())) {
//		_DMESSAGE("Failed to create file");
//		return;
//	}
//
//	std::stringstream stream;
//	for (auto& category : lightsMenuCache) {
//		stream << "Category\t" << category.first << std::endl;
//		for (auto& light : category.second) {
//			UInt32 formId = std::stoul(light.first, nullptr, 16); 
//			TESForm* form = LookupFormByID(formId);
//			if (form && form->GetFullName()) {
//				std::string fullname(form->GetFullName());
//				int firstof = fullname.find_first_of("-");
//				std::string afterHyphen = fullname.substr(firstof + 2);
//				std::string key = HexToString(formId & 0xFFFFFF);
//				stream << key << "\t" << afterHyphen << std::endl;
//			}
//			else {
//				stream << light.first << "\t" << "Error" << std::endl;
//			}
//		}
//	}
//
//	file.WriteString(stream.str().c_str());
//	file.Close();
//}