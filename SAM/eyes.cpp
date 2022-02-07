#include "eyes.h"

#include "f4se/GameObjects.h"
#include "f4se/GameTypes.h"
#include "f4se/NiNodes.h"
#include "f4se/BSGeometry.h"
#include "f4se/GameRTTI.h"

#include "sam.h"
#include "mfg.h"

NiAVObject* GetEyeNode(TESObjectREFR* refr)
{
	if (!refr) {
		return nullptr;
	}

	TESNPC* npc = (TESNPC*)refr->baseForm;
	BGSHeadPart* eyePart = npc->GetHeadPartByType(BGSHeadPart::kTypeEyes);
	if (!eyePart) {
		_DMESSAGE("Failed to find eye head part");
		return nullptr;
	}
	BSFixedString eyeName = eyePart->partName;

	NiNode* rootNode = refr->GetActorRootNode(false);
	NiAVObject* eyeNode = rootNode->GetObjectByName(&eyeName);
	if (!eyeNode) {
		_DMESSAGE("Failed to find eye node");
		return nullptr;
	}
	return eyeNode;
}

BSShaderMaterial* GetEyeMaterial()
{
	if (!selected.eyeNode) {
		_DMESSAGE("Eye node not found");
		return nullptr;
	}
	BSGeometry* eyeGeometry = selected.eyeNode->GetAsBSGeometry();
	NiPointer<NiProperty> niProperty = eyeGeometry->shaderProperty;
	BSShaderProperty* shaderProperty = (BSShaderProperty*)niProperty.get();
	if (!shaderProperty) {
		_DMESSAGE("Eye shader not found");
		return nullptr;
	}
	BSShaderMaterial* eyeMaterial = shaderProperty->shaderMaterial;

	return eyeMaterial;
}

bool GetEyecoords(float* ret)
{
	BSShaderMaterial* eyeMaterial = GetEyeMaterial();

	if (!eyeMaterial) return false;

	float u, v;
	eyeMaterial->GetOffsetUV(&u, &v);
	ret[0] = u;
	ret[1] = v;

	return true;
}

//RelocAddr<EyeUpdateData> eyeUpdateData(0x5A5D430);
bool eyeMeshUpdated = false;

void SetEyecoords(float x, float y)
{
	if (eyeMeshUpdated) {
		selected.eyeNode = GetEyeNode(selected.refr);
		eyeMeshUpdated = false;
	}

	if (!selected.eyeNode)
		return;

	BSGeometry* eyeGeometry = selected.eyeNode->GetAsBSGeometry();
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

BGSHeadPart* GetHeadPartByID(UInt32 id) {
	TESForm* eyeForm = LookupFormByID(id);
	BGSHeadPart* crossedEyePart = DYNAMIC_CAST(eyeForm, TESForm, BGSHeadPart);
	return crossedEyePart;
}

BGSHeadPart* ChangeEyeMesh(TESObjectREFR* refr, BGSHeadPart* eyePart) {
	TESNPC* npc = (TESNPC*)refr->baseForm;
	BGSHeadPart* previousEyes = npc->GetHeadPartByType(BGSHeadPart::kTypeEyes);
	BGSTextureSet* texture = previousEyes->textureSet;
	npc->ChangeHeadPart(eyePart, false, false);
	BGSHeadPart* newEyes = npc->GetHeadPartByType(BGSHeadPart::kTypeEyes);
	newEyes->textureSet = texture;
	npc->MarkChanged(0x800);
	(*g_player)->middleProcess->unk08->unk496 = Actor::MiddleProcess::Data08::kDirtyHeadParts;
	CALL_MEMBER_FN(*g_player, QueueUpdate)(false, 0, true, 0);
	eyeMeshUpdated = true;
	return previousEyes;
}