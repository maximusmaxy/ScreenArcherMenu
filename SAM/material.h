#pragma once

#include <fstream>

struct MaterialColor
{
	float r;
	float g;
	float b;
};

class Material
{
public:
	//base
	UInt32 type;
	UInt32 version;

	bool tileU;
	bool tileV;
	float uOffset;
	float vOffset;
	float uScale;
	float vScale;

	float alpha;
	UInt32 blendMode;
	UInt8 alphaTestRef;
	bool alphaTest;

	bool zBufferWrite;
	bool zBufferTest;
	bool screenSpaceReflections;
	bool wetnessControlScreenSpaceReflections;
	bool decal;
	bool twoSided;
	bool decalNoFade;
	bool nonOccluder;

	bool refraction;
	bool refractionFalloff;
	float refractionPower;

	bool environmentMapping;
	float environmentMappingMaskScale;
	bool depthBias;

	bool grayscaleToPaletteColor;

	UInt8 maskWrites;

	//bgsm
	std::string diffuseTexture;
	std::string normalTexture;
	std::string smoothSpecTexture;
	std::string grayscaleTexture;
	std::string glowTexture;
	std::string wrinklesTexture;
	std::string specularTexture;
	std::string lightingTexture;
	std::string flowTexture;
	std::string distanceFieldAlphaTexture;
	std::string envmapTexture;
	std::string innerLayerTexture;
	std::string displacementTexture;

	bool enableEditorAlphaRef;

	bool translucency;
	bool translucencyThickObject;
	bool translucencyMixAlbedoWithSubsurfaceColor;
	MaterialColor translucencySubsurfaceColor;
	float translucencyTransmissiveScale;
	float translucencyTurbulence;
	bool rimLighting;
	float rimPower;
	float backlightPower;
	bool subsurfaceLighting;
	float subsurfaceLightingRolloff;

	bool specularEnabled;
	MaterialColor specularColor;
	float specularMult;
	float smoothness;
	float fresnelPower;
	float wetnessControlSpecScale;
	float wetnessControlSpecPowerScale;
	float wetnessControlSpecMinvar;
	float wetnessControlEnvmapScale;
	float wetnessControlFresnelPower;
	float wetnessControlMetalness;

	bool pbr;
	bool customPorosity;
	float porosityValue;

	std::string rootMaterialPath;

	bool anisoLighting;
	bool emitEnabled;
	MaterialColor emittanceColor;
	float emittanceMult;
	bool modelSpaceNormals;
	bool externalEmittance;
	float lumEmittance;
	bool useAdaptiveEmissive;
	float adaptiveEmissiveExposureOffset;
	float adaptiveEmissiveFinalExposureMin;
	float adaptiveEmissiveFinalExposureMax;

	bool backLighting;
	bool recieveShadows;
	bool hideSecret;
	bool castShadows;
	bool dissolveFade;
	bool assumeShadowmask;

	bool glowMap;
	bool environmentMappingWindow;
	bool environmentMappingEye;

	bool hair;
	MaterialColor hairTintColor;

	bool tree;
	bool facegen;
	bool skinTint;
	bool tessellate;

	float displacementTextureBias;
	float displacementTextureScale;
	float tessellationPnScale;
	float tessellationBaseFactor;
	float tessellationFadeDistance;

	float grayscaleToPaletteScale;

	bool skewSpecularAlpha;

	bool terrain;
	UInt32 unkInt1;
	float terrainThresholdFalloff;
	float terrainTilingDistance;
	float terrainRotationAngle;

	//bgem
	//

	bool ReadBase(std::ifstream& stream);
	bool ReadBgsm(std::ifstream& stream);
	bool ReadBgem(std::ifstream& stream);

	bool WriteBase(std::ofstream& stream);
	bool WriteBgsm(std::ofstream& stream);
	bool WriteBgem(std::ofstream& stream);

	bool Load(const char* path);
	bool Save(const char* path);
};