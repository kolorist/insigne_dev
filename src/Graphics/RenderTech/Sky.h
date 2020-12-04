#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{
// ------------------------------------------------------------------

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

// ------------------------------------------------------------------

class Sky : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "sky";

public:
	Sky();
	~Sky();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	f32**										AllocateTexture3D(const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);
	f32*										AllocateTexture2D(const s32 i_w, const s32 i_h, const s32 i_channel);
	void										WriteCacheTex2D(const_cstr i_cacheFileName, f32* i_data, const s32 i_w, const s32 i_h, const s32 i_channel);
	void										WriteCacheTex3D(const_cstr i_cacheFileName, f32** i_data, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);
	void										WriteCache(const_cstr i_cacheFileName, voidptr i_data, const ssize i_size);
	f32*										LoadCacheTex2D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_channel);
	f32**										LoadCacheTex3D(const_cstr i_cacheFileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);
	voidptr										LoadCache(const_cstr i_cacheFileName, const ssize i_size, const voidptr i_buffer = nullptr);

	void										_DebugWriteHDR3D(const_cstr i_fileName, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel, f32** i_data);

private:
	Atmosphere									m_Atmosphere;

private:
	LinearArena*								m_DataArena;
	helich::memory_region<LinearArena>			m_TexDataArenaRegion;
	LinearArena									m_TexDataArena;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;
};

// ------------------------------------------------------------------
}
}
