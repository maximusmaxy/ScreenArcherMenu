#include "eyes.h"

#include "f4se/GameObjects.h"
#include "f4se/GameTypes.h"
#include "f4se/NiNodes.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameRTTI.h"

NiAVObject* GetEyeNode(TESObjectREFR* refr)
{
	if (!refr) return nullptr;

	TESNPC* npc = (TESNPC*)refr->baseForm;
	BGSHeadPart* eyePart = npc->GetHeadPartByType(BGSHeadPart::kTypeEyes);
	if (!eyePart) return nullptr;
	BSFixedString eyeName = eyePart->partName;

	NiNode* rootNode = refr->GetActorRootNode(false);
	NiAVObject* eyeNode = rootNode->GetObjectByName(&eyeName);
	if (!eyeNode) return nullptr;

	return eyeNode;
}

BSShaderMaterial* GetEyeMaterial(NiAVObject* eyeNode)
{
	if (!eyeNode) return nullptr;

	BSGeometry* eyeGeometry = eyeNode->GetAsBSGeometry();
	NiPointer<NiProperty> niProperty = eyeGeometry->shaderProperty;
	BSShaderProperty* shaderProperty = (BSShaderProperty*)niProperty.get();
	if (!shaderProperty) return nullptr;

	BSShaderMaterial* eyeMaterial = shaderProperty->shaderMaterial;
	return eyeMaterial;
}

bool GetEyecoords(NiAVObject* eyeNode, float* ret)
{
	if (!eyeNode) return false;

	BSShaderMaterial* eyeMaterial = GetEyeMaterial(eyeNode);
	if (!eyeMaterial) return false;

	float u, v;
	eyeMaterial->GetOffsetUV(&u, &v);
	ret[0] = u;
	ret[1] = v;

	return true;
}

bool GetEyecoords(TESObjectREFR* refr, float* ret)
{
	NiAVObject* eyeNode = GetEyeNode(refr);
	if (!eyeNode) return false;

	return GetEyecoords(eyeNode, ret);
}

void SetEyecoords(NiAVObject* eyeNode, float x, float y)
{
	if (!eyeNode) return;

	BSGeometry* eyeGeometry = eyeNode->GetAsBSGeometry();
	NiPointer<NiProperty> niProperty = eyeGeometry->shaderProperty;
	BSShaderProperty* shaderProperty = (BSShaderProperty*)niProperty.get();

	if (!shaderProperty)
		return;

	//Setting this value makes sure the eye uv map updates
	//It only needs to be done once but haven't figured out how to check if it's already been applied
	//so we do it every time to make sure
	shaderProperty->iLastRenderPassState = 0x7FFFFFFF;

	BSShaderMaterial* eyeMaterial = shaderProperty->shaderMaterial;

	if (!eyeMaterial)
		return;

	eyeMaterial->SetOffsetUV(x, y);
}

void SetEyecoords(TESObjectREFR* refr, float x, float y)
{
	if (!refr) return;

	NiAVObject* eyeNode = GetEyeNode(refr);
	if (!eyeNode) return;

	SetEyecoords(eyeNode, x, y);
}