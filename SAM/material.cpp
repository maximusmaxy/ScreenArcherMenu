#include "material.h"

#include "SAF/util.h"
#include "io.h"

void ReadMaterialString(std::ifstream& stream, std::string& str)
{
	UInt32 size = Read<UInt32>(stream);
	str.resize(size - 1);
	stream.read(str.data(), size);
}

void ReadColor(std::ifstream& stream, MaterialColor& color)
{
	color.r = Read<float>(stream);
	color.g = Read<float>(stream);
	color.b = Read<float>(stream);
}

enum {
	kBlendModeUnknown = 0,
	kBlendModeNone,
	kBlendModeStandard,
	kBlendModeAdditive,
	kBlendModeMultiplicative,

	kBlendModeError = -1
};

UInt32 GetBlendMode(bool a, UInt32 b, UInt32 c)
{
	if (!a && b == 6 && c == 7) {
		return kBlendModeUnknown;
	}
	else if (!a && b == 0 && c == 0) {
		return kBlendModeNone;
	}
	else if (a && b == 6 && c == 7) {
		return kBlendModeStandard;
	}
	else if (a && b == 6 && c == 0) {
		return kBlendModeAdditive;
	}
	else if (a && b == 4 && c == 1) {
		return kBlendModeMultiplicative;
	}

	return kBlendModeError;
}

bool Material::ReadBase(std::ifstream& stream)
{
	type = Read<UInt32>(stream);
	version = Read<UInt32>(stream);

	auto tileFlags = Read<UInt32>(stream);
	tileU = (tileFlags & 2);
	tileV = (tileFlags & 1);
	uOffset = Read<float>(stream);
	vOffset = Read<float>(stream);
	uScale = Read<float>(stream);
	vScale = Read<float>(stream);

	alpha = Read<float>(stream);
	auto blend1 = Read<bool>(stream);
	auto blend2 = Read<UInt32>(stream);
	auto blend3 = Read<UInt32>(stream);
	blendMode = GetBlendMode(blend1, blend2, blend3);
	if (blendMode == kBlendModeError) {
		_DMESSAGE("Failed to read material blend mode");
		return false;
	}
	alphaTestRef = Read<UInt8>(stream);
	alphaTest = Read<bool>(stream);

	zBufferWrite = Read<bool>(stream);
	zBufferTest = Read<bool>(stream);
	screenSpaceReflections = Read<bool>(stream);
	wetnessControlScreenSpaceReflections = Read<bool>(stream);
	decal = Read<bool>(stream);
	twoSided = Read<bool>(stream);
	decalNoFade = Read<bool>(stream);
	nonOccluder = Read<bool>(stream);

	refraction = Read<bool>(stream);
	refractionFalloff = Read<bool>(stream);
	refractionPower = Read<float>(stream);

	if (version < 10) {
		environmentMapping = Read<bool>(stream);
		environmentMappingMaskScale = Read<float>(stream);
	}
	else {
		depthBias = Read<bool>(stream);
	}

	grayscaleToPaletteColor = Read<bool>(stream);

	if (version >= 6) {
		maskWrites = Read<UInt8>(stream);
	}
}

bool Material::ReadBgsm(std::ifstream& stream)
{
	ReadMaterialString(stream, diffuseTexture);
	ReadMaterialString(stream, normalTexture);
	ReadMaterialString(stream, smoothSpecTexture);
	ReadMaterialString(stream, grayscaleTexture);

	if (version > 2) {
		ReadMaterialString(stream, glowTexture);
		ReadMaterialString(stream, wrinklesTexture);
		ReadMaterialString(stream, specularTexture);
		ReadMaterialString(stream, lightingTexture);
		ReadMaterialString(stream, flowTexture);
		
		if (version >= 17) {
			ReadMaterialString(stream, distanceFieldAlphaTexture);
		}
	}
	else {
		ReadMaterialString(stream, envmapTexture);
		ReadMaterialString(stream, glowTexture);
		ReadMaterialString(stream, innerLayerTexture);
		ReadMaterialString(stream, wrinklesTexture);
		ReadMaterialString(stream, displacementTexture);
	}

	enableEditorAlphaRef = Read<bool>(stream);
	
	if (version >= 8) {
		translucency = Read<bool>(stream);
		translucencyThickObject = Read<bool>(stream);
		translucencyMixAlbedoWithSubsurfaceColor = Read<bool>(stream);
		ReadColor(stream, translucencySubsurfaceColor);
		translucencyTransmissiveScale = Read<float>(stream);
		translucencyTurbulence = Read<float>(stream);
	}
	else {
		rimLighting = Read<bool>(stream);
		rimPower = Read<float>(stream);
		backlightPower = Read<float>(stream);
		subsurfaceLighting = Read<bool>(stream);
		subsurfaceLightingRolloff = Read<float>(stream);
	}

	specularEnabled = Read<bool>(stream);
	ReadColor(stream, specularColor);
	specularMult = Read<float>(stream);
	smoothness = Read<float>(stream);

	fresnelPower = Read<float>(stream);
	wetnessControlSpecScale = Read<float>(stream);
	wetnessControlSpecPowerScale = Read<float>(stream);
	wetnessControlSpecMinvar = Read<float>(stream);

	if (version < 10)
		wetnessControlEnvmapScale = Read<float>(stream);

	wetnessControlFresnelPower = Read<float>(stream);
	wetnessControlMetalness = Read<float>(stream);

	if (version > 2) {
		pbr = Read<bool>(stream);
		if (version >= 9) {
			customPorosity = Read<bool>(stream);
			porosityValue = Read<float>(stream);
		}
	}

	ReadMaterialString(stream, rootMaterialPath);

	anisoLighting = Read<bool>(stream);
	emitEnabled = Read<bool>(stream);

	if (emitEnabled)
		ReadColor(stream, emittanceColor);

	emittanceMult = Read<float>(stream);
	modelSpaceNormals = Read<bool>(stream);
	externalEmittance = Read<bool>(stream);

	if (version >= 12)
		lumEmittance = Read<float>(stream);

	if (version >= 13) {
		useAdaptiveEmissive = Read<bool>(stream);
		adaptiveEmissiveExposureOffset = Read<float>(stream);
		adaptiveEmissiveFinalExposureMin = Read<float>(stream);
		adaptiveEmissiveFinalExposureMin = Read<float>(stream);
	}

	if (version < 8)
		backLighting = Read<bool>(stream);

	recieveShadows = Read<bool>(stream);
	hideSecret = Read<bool>(stream);
	castShadows = Read<bool>(stream);
	dissolveFade = Read<bool>(stream);
	assumeShadowmask = Read<bool>(stream);

	glowMap = Read<bool>(stream);
	
	if (version < 7) {
		environmentMappingWindow = Read<bool>(stream);
		environmentMappingEye = Read<bool>(stream);
	}

	hair = Read<bool>(stream);
	ReadColor(stream, hairTintColor);

	tree = Read<bool>(stream);
	facegen = Read<bool>(stream);
	skinTint = Read<bool>(stream);
	tessellate = Read<bool>(stream);

	if (version < 3) {
		displacementTextureBias = Read<float>(stream);
		displacementTextureScale = Read<float>(stream);
		tessellationPnScale = Read<float>(stream);
		tessellationBaseFactor = Read<float>(stream);
		tessellationFadeDistance = Read<float>(stream);
	}

	grayscaleToPaletteScale = Read<float>(stream);

	if (version >= 1)
		skewSpecularAlpha = Read<bool>(stream);

	if (version >= 3) {
		terrain = Read<bool>(stream);

		if (terrain) {
			if (version == 3)
				unkInt1 = Read<UInt32>(stream);

			terrainThresholdFalloff = Read<float>(stream);
			terrainTilingDistance = Read<float>(stream);
			terrainRotationAngle = Read<float>(stream);
		}
	}

	return true;
}

bool Material::ReadBgem(std::ifstream& stream)
{
	//

	return true;
}

bool Material::Load(const char* path)
{
	std::ifstream stream;
	stream.open(path, std::ios::in | std::ios::binary);
	if (stream.fail()) {
		_Log("Failed to read material file: ", path);
		return false;
	}

	if (!ReadBase(stream))
		return false;

	if (type == 'MSGB') {
		if (!ReadBgsm(stream))
			return false;
	}
	else if (type == 'MEGB') {
		if (!ReadBgem(stream))
			return false;
	}
	else {
		_Log("Failed to read material type: ", path);
		return false;
	}
	
	return true;
}

void WriteMaterialString(std::ofstream& stream, std::string& str)
{
	Write<UInt32>(stream, str.size() + 1);
	stream.write(str.data(), str.size() + 1);
}

void WriteColor(std::ofstream& stream, MaterialColor& color)
{
	Write(stream, color.r);
	Write(stream, color.g);
	Write(stream, color.b);
}

bool SetBlendMode(UInt32 blendMode, bool* a, UInt32* b, UInt32* c)
{
	switch (blendMode) {
	case kBlendModeUnknown:
		*a = false;
		*b = 6;
		*c = 7;
		break;
	case kBlendModeNone:
		*a = false;
		*b = 0;
		*c = 0;
		break;
	case kBlendModeStandard:
		*a = true;
		*b = 6;
		*c = 7;
		break;
	case kBlendModeAdditive:
		*a = true;
		*b = 6;
		*c = 0;
		break;
	case kBlendModeMultiplicative:
		*a = true;
		*b = 4;
		*c = 1;
		break;
	case kBlendModeError:
		return false;
	}

	return true;
}

bool Material::WriteBase(std::ofstream& stream)
{
	Write(stream, type);
	Write(stream, version);
	UInt32 tileFlags = 0;
	if (tileU)
		tileFlags |= 2;
	if (tileV)
		tileFlags |= 1;
	Write(stream, tileFlags);
	Write(stream, uOffset);
	Write(stream, vOffset);
	Write(stream, uScale);
	Write(stream, vScale);
	Write(stream, alpha);

	bool a;
	UInt32 b;
	UInt32 c;
	if (!SetBlendMode(blendMode, &a, &b, &c)) {
		_DMESSAGE("Failed to set material blend mode");
		return false;
	}
	Write(stream, a);
	Write(stream, b);
	Write(stream, c);

	Write(stream, alphaTestRef);
	Write(stream, alphaTest);
	
	Write(stream, zBufferWrite);
	Write(stream, zBufferTest);
	Write(stream, screenSpaceReflections);
	Write(stream, wetnessControlScreenSpaceReflections);
	Write(stream, decal);
	Write(stream, twoSided);
	Write(stream, decalNoFade);
	Write(stream, nonOccluder);

	Write(stream, refraction);
	Write(stream, refractionFalloff);
	Write(stream, refractionPower);

	if (version < 10)
	{
		Write(stream, environmentMapping);
		Write(stream, environmentMappingMaskScale);
	}
	else {
		Write(stream, depthBias);
	}

	if (version >= 6)
		Write(stream, maskWrites);

	return true;
}

bool Material::WriteBgsm(std::ofstream& stream)
{
	WriteMaterialString(stream, diffuseTexture);
	WriteMaterialString(stream, normalTexture);
	WriteMaterialString(stream, smoothSpecTexture);
	WriteMaterialString(stream, grayscaleTexture);

	if (version > 2) {
		WriteMaterialString(stream, glowTexture);
		WriteMaterialString(stream, wrinklesTexture);
		WriteMaterialString(stream, specularTexture);
		WriteMaterialString(stream, lightingTexture);
		WriteMaterialString(stream, flowTexture);

		if (version >= 17) {
			WriteMaterialString(stream, distanceFieldAlphaTexture);
		}
	}
	else {
		WriteMaterialString(stream, envmapTexture);
		WriteMaterialString(stream, glowTexture);
		WriteMaterialString(stream, innerLayerTexture);
		WriteMaterialString(stream, wrinklesTexture);
		WriteMaterialString(stream, displacementTexture);
	}

	Write(stream, enableEditorAlphaRef);

	if (version >= 8) {
		Write(stream, translucency);
		Write(stream, translucencyThickObject);
		Write(stream, translucencyMixAlbedoWithSubsurfaceColor);
		WriteColor(stream, translucencySubsurfaceColor);
		Write(stream, translucencyTransmissiveScale);
		Write(stream, translucencyTurbulence);
	}
	else {
		Write(stream, rimLighting);
		Write(stream, rimPower);
		Write(stream, backlightPower);
		Write(stream, subsurfaceLighting);
		Write(stream, subsurfaceLightingRolloff);
	}

	Write(stream, specularEnabled);
	WriteColor(stream, specularColor);
	Write(stream, specularMult);
	Write(stream, smoothness);

	Write(stream, fresnelPower);
	Write(stream, wetnessControlSpecScale);
	Write(stream, wetnessControlSpecPowerScale);
	Write(stream, wetnessControlSpecMinvar);

	if (version < 10)
		Write(stream, wetnessControlEnvmapScale);

	Write(stream, wetnessControlFresnelPower);
	Write(stream, wetnessControlMetalness);

	if (version > 2)
	{
		Write(stream, pbr);

		if (version >= 9) {
			Write(stream, customPorosity);
			Write(stream, porosityValue);
		}
	}

	WriteMaterialString(stream, rootMaterialPath);

	Write(stream, anisoLighting);
	Write(stream, emitEnabled);

	if (emitEnabled)
		WriteColor(stream, emittanceColor);

	Write(stream, emittanceMult);
	Write(stream, modelSpaceNormals);
	Write(stream, externalEmittance);

	if (version >= 12)
		Write(stream, lumEmittance);

	if (version >= 13) {
		Write(stream, useAdaptiveEmissive);
		Write(stream, adaptiveEmissiveExposureOffset);
		Write(stream, adaptiveEmissiveFinalExposureMin);
		Write(stream, adaptiveEmissiveFinalExposureMax);
	}

	if (version < 8)
		Write(stream, backLighting);

	Write(stream, recieveShadows);
	Write(stream, hideSecret);
	Write(stream, castShadows);
	Write(stream, dissolveFade);
	Write(stream, assumeShadowmask);
	Write(stream, glowMap);

	if (version < 7) {
		Write(stream, environmentMappingWindow);
		Write(stream, environmentMappingEye);
	}

	Write(stream, hair);
	WriteColor(stream, hairTintColor);

	Write(stream, tree);
	Write(stream, facegen);
	Write(stream, skinTint);
	Write(stream, tessellate);

	if (version < 3)
	{
		Write(stream, displacementTextureBias);
		Write(stream, displacementTextureScale);
		Write(stream, tessellationPnScale);
		Write(stream, tessellationBaseFactor);
		Write(stream, tessellationFadeDistance);
	}

	Write(stream, grayscaleToPaletteScale);
	
	if (version >= 1)
		Write(stream, skewSpecularAlpha);

	if (version >= 3)
	{
		Write(stream, terrain);
		if (terrain)
		{
			if (version == 3)
				Write(stream, unkInt1);

			Write(stream, terrainThresholdFalloff);
			Write(stream, terrainTilingDistance);
			Write(stream, terrainRotationAngle);
		}
	}

	return true;
}

bool Material::WriteBgem(std::ofstream& stream)
{
	//

	return true;
}

bool Material::Save(const char* path)
{
	SAF::OutStreamWrapper wrapper(path);
	if (wrapper.fail)
		return false;

	if (!WriteBase(wrapper.stream))
		return false;

	if (type == 'MSGB') {
		if (!WriteBgsm(wrapper.stream))
			return false;
	}
	else if (type == 'MEGB') {
		if (!WriteBgem(wrapper.stream))
			return false;
	}
	else {
		_Log("Failed to read material type: ", path);
		return false;
	}

	return true;
}