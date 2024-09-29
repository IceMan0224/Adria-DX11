#pragma once

namespace adria
{
	#define DECLARE_CBUFFER_SLOT(name, slot) static constexpr uint16 CBUFFER_SLOT_##name = slot
	#define DECLARE_TEXTURE_SLOT(name, slot) static constexpr uint16 TEXTURE_SLOT_##name = slot

	DECLARE_CBUFFER_SLOT(FRAME, 0);
	DECLARE_CBUFFER_SLOT(OBJECT, 1);
	DECLARE_CBUFFER_SLOT(LIGHT, 2);
	DECLARE_CBUFFER_SLOT(SHADOW, 3);
	DECLARE_CBUFFER_SLOT(MATERIAL, 4);
	DECLARE_CBUFFER_SLOT(POSTPROCESS, 5);
	DECLARE_CBUFFER_SLOT(COMPUTE, 6);
	DECLARE_CBUFFER_SLOT(WEATHER, 7);
	DECLARE_CBUFFER_SLOT(VOXEL, 8);
	DECLARE_CBUFFER_SLOT(TERRAIN, 9);

	DECLARE_TEXTURE_SLOT(DIFFUSE, 0);
	DECLARE_TEXTURE_SLOT(SPECULAR, 1);
	DECLARE_TEXTURE_SLOT(ROUGHNESS_METALLIC, 1);
	DECLARE_TEXTURE_SLOT(NORMAL, 2);
	DECLARE_TEXTURE_SLOT(EMISSIVE, 3);
	DECLARE_TEXTURE_SLOT(CUBEMAP, 0);
	DECLARE_TEXTURE_SLOT(SHADOW, 4);
	DECLARE_TEXTURE_SLOT(SHADOWCUBE, 5);
	DECLARE_TEXTURE_SLOT(SHADOWARRAY, 6);

	DECLARE_TEXTURE_SLOT(GRASS, 0);
	DECLARE_TEXTURE_SLOT(BASE, 1);
	DECLARE_TEXTURE_SLOT(ROCK, 2);
	DECLARE_TEXTURE_SLOT(SAND, 3);
	DECLARE_TEXTURE_SLOT(LAYER, 4);

	enum ShaderId : uint8
	{
		VS_Sky,
		PS_Skybox,
		PS_HosekWilkieSky,
		PS_UniformSky,
		VS_Texture,
		PS_Texture,
		VS_Solid,
		PS_Solid,
		VS_Sun,
		VS_Billboard,
		VS_Decal,
		PS_Decal,
		PS_DecalsModifyNormals,
		VS_GBufferPBR,
		PS_GBufferPBR,
		PS_GBufferPBR_Mask,
		VS_GBufferTerrain,
		PS_GBufferTerrain,
		VS_FullscreenQuad,
		PS_AmbientPBR,
		PS_AmbientPBR_AO,
		PS_AmbientPBR_IBL,
		PS_AmbientPBR_AO_IBL,
		PS_DeferredLighting,
		PS_ClusterLighting,
		PS_ToneMap_Reinhard,
		PS_ToneMap_Linear,
		PS_ToneMap_Hable,
		PS_ToneMap_TonyMcMapface,
		PS_FXAA,
		PS_TAA,
		PS_CopyTextures,
		PS_AddTextures,
		PS_SSAO,
		PS_HBAO,
		PS_SSR,
		PS_FilmEffects,
		VS_LensFlare,
		GS_LensFlare,
		PS_LensFlare,
		PS_GodRays,
		PS_DepthOfField,
		VS_Bokeh,
		GS_Bokeh,
		PS_Bokeh,
		PS_VolumetricClouds,
		PS_MotionVectors,
		PS_MotionBlur,
		PS_Fog,
		VS_Shadow,
		PS_Shadow,
		VS_ShadowTransparent,
		PS_ShadowTransparent,
		PS_VolumetricLight_Directional,
		PS_VolumetricLight_Spot,
		PS_VolumetricLight_Point,
		PS_VolumetricLight_DirectionalWithCascades,
		CS_BlurHorizontal,
		CS_BlurVertical,
		CS_BokehGeneration,
		CS_BloomExtract,
		CS_BloomCombine,
		CS_OceanInitialSpectrum,
		CS_OceanPhase,
		CS_OceanSpectrum,
		CS_OceanFFT_Horizontal,
		CS_OceanFFT_Vertical,
		CS_OceanNormalMap,
		CS_TiledLighting,
		CS_ClusterBuilding,
		CS_ClusterCulling,
		VS_Ocean,
		PS_Ocean,
		VS_OceanLOD,
		HS_OceanLOD,
		DS_OceanLOD,
		VS_Foliage,
		PS_Foliage,
		CS_Picker,
		VS_Particle,
		PS_Particle,
		CS_ParticleInitDeadList,
		CS_ParticleReset,
		CS_ParticleEmit,
		CS_ParticleSimulate,
		CS_ParticleBitonicSortStep,
		CS_ParticleSort512,
		CS_ParticleSortInner512,
		CS_ParticleInitSortArgs,
		ShaderId_Count
	};

	enum class ShaderProgram : uint8 
	{
		Skybox,
		UniformColorSky,
		HosekWilkieSky,
		Texture,
		Solid,
		Sun,
		Billboard,
		GBufferPBR,
		GBufferPBR_Mask,
		GBuffer_Terrain,
		AmbientPBR,
		AmbientPBR_AO,
		AmbientPBR_IBL,
		AmbientPBR_AO_IBL,
		LightingPBR,
		ClusterLightingPBR,
		ToneMap_Reinhard,
		ToneMap_Hable,
		ToneMap_Linear,
		ToneMap_TonyMcMapface,
		FXAA,
		TAA,
		Copy,
		Add,
		DepthMap,
		DepthMap_Transparent,
		Volumetric_Directional,
		Volumetric_DirectionalCascades,
		Volumetric_Spot,
		Volumetric_Point,
		Volumetric_Clouds,
		SSAO,
		SSR,
		HBAO,
		GodRays,
		MotionBlur,
		FilmEffects,
		DOF,
		Fog,
		Ocean,
		VoxelGI,
		MotionVectors,
		GBuffer_Foliage,
		Particles,
		Decals,
		Decals_ModifyNormals,
		Blur_Horizontal,
		Blur_Vertical,
		BloomExtract,
		BloomCombine,
		BokehGenerate,
		OceanInitialSpectrum,
		OceanSpectrum,
		OceanFFT_Horizontal,
		OceanFFT_Vertical,
		OceanNormalMap,
		OceanPhase,
		TiledLighting,
		ClusterBuilding,
		ClusterCulling,
		VoxelCopy,
		VoxelSecondBounce,
		Picker,
		ParticleInitDeadList,
		ParticleReset,
		ParticleEmit,
		ParticleSimulate,
		ParticleBitonicSortStep,
		ParticleSort512,
		ParticleSortInner512,
		ParticleSortInitArgs,
		LensFlare,
		BokehDraw,
		Voxelize,
		VoxelizeDebug,
		OceanLOD,
		Unknown
	};

	enum class ToneMap : uint8
	{
		Reinhard,
		Hable,
		Linear,
		TonyMcMapface
	};

	enum class LightType : int32
	{
		Directional,
		Point,
		Spot
	};

	enum class FogType : int32
	{
		Exponential, 
		ExponentialHeight
	};

	enum class AmbientOcclusion : uint8
	{
		None,
		SSAO,
		HBAO
	};

	enum class BokehType : uint8
	{
		Hex,
		Oct,
		Circle,
		Cross
	};

	enum class SkyType : uint8
	{
		UniformColor,
		Skybox,
		HosekWilkie
	};

	enum class BlendState
	{
		None,
		AlphaToCoverage,
		AdditiveBlend,
		AlphaBlend
	};

	enum class DepthState
	{
		None
	};

	enum class RasterizerState
	{
		None,
		NoCulling
	};

	enum AntiAliasing : uint8
	{
		AntiAliasing_None = 0x0,
		AntiAliasing_FXAA = 0x1,
		AntiAliasing_TAA = 0x2
	};

	enum GBufferSlot : uint8
	{
		GBufferSlot_NormalMetallic,
		GBufferSlot_DiffuseRoughness,
		GBufferSlot_Emissive,
		GBufferSlot_Count
	};

	enum class DecalType : uint8 
	{
		Project_XY,
		Project_YZ,
		Project_XZ
	};

	enum class MaterialAlphaMode : uint8
	{
		Opaque,
		Blend,
		Mask
	};
}
