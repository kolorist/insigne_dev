#pragma once
#include <floral/stdaliases.h>
#include <floral/gpds/vec.h>

namespace stone
{
//-------------------------------------------------------------------
#pragma pack(push)
#pragma pack(1)

struct BakedDataInfos
{
	s32											transmittanceTextureWidth;
	s32											transmittanceTextureHeight;

	s32											scatteringTextureRSize;
	s32											scatteringTextureMuSize;
	s32											scatteringTextureMuSSize;
	s32											scatteringTextureNuSize;

	s32											scatteringTextureWidth;
	s32											scatteringTextureHeight;
	s32											scatteringTextureDepth;

	s32											irrandianceTextureWidth;
	s32											irrandianceTextureHeight;
};

struct SkyFixedConfigs
{
	floral::vec3f								solarIrradiance;
	floral::vec3f								rayleighScattering;
	floral::vec3f								mieScattering;
	f32											sunAngularRadius;
	f32											bottomRadius;
	f32											topRadius;
	f32											miePhaseFunctionG;
	f32											muSMin;
	f32											unitLengthInMeters;
};

#pragma pack(pop)

struct DensityProfileLayer
{
	f32											Width;

	f32											ExpTerm;
	f32											ExpScale;
	f32											LinearTerm;
	f32											ConstantTerm;
};

struct DensityProfile
{
	DensityProfileLayer							Layers[2];
};

struct Atmosphere
{
	f32											TopRadius;
	f32											BottomRadius;
	f32											MuSMin;
	floral::vec3f								SolarIrradiance;
	f32											SunAngularRadius;

	floral::vec3f								RayleighScattering;
	DensityProfile								RayleighDensity;
	floral::vec3f								MieScattering;
	floral::vec3f								MieExtinction;
	DensityProfile								MieDensity;
	f32											MiePhaseFunctionG;
	floral::vec3f								AbsorptionExtinction;
	DensityProfile								AbsorptionDensity;

	floral::vec3f								GroundAlbedo;
};

//-------------------------------------------------------------------

void											initialize_atmosphere(Atmosphere* o_atmosphere, BakedDataInfos* o_textureInfo, SkyFixedConfigs* o_skyConfigs);
void											generate_transmittance_texture(const Atmosphere& i_atmosphere, f32* o_texture);
void											generate_direct_irradiance_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture, f32* o_texture);
void											generate_single_scattering_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
													f32** o_deltaRayleighScatteringTexture, f32** o_deltaMieScatteringTexture, f32** o_scatteringTexture,
													const s32 i_depth = -1);
void											generate_scattering_density_texture(const Atmosphere& i_atmosphere, f32* i_transmittanceTexture,
													f32** i_deltaRayleighScatteringTexture, f32** i_deltaMieScatteringTexture, f32** i_deltaMultipleScatteringTexture, f32* i_deltaIrradianceTexture,
													f32** o_deltaScatteringDensityTexture, const s32 i_scatteringOrder, const s32 i_depth);

void											generate_indirect_irradiance_texture(const Atmosphere& i_atmosphere,
													f32** i_deltaRayleighScatteringTexture, f32** i_deltaMieScatteringTexture, f32** i_deltaMultipleScatteringTexture,
													f32* o_deltaIrradianceTexture, f32* o_irradianceTexture, const s32 i_scatteringOrder);

void											generate_multiple_scattering_texture(const Atmosphere& i_atmosphere,
													f32* i_transmittanceTexture, f32** i_deltaScatteringDensityTexture,
													f32** o_deltaMultipleScatteringTexture, f32** o_scatteringTexture, const s32 i_depth);

}
