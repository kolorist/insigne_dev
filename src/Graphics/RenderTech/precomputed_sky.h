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

//-------------------------------------------------------------------
}
