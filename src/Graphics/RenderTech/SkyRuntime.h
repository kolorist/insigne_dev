#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/MaterialLoader.h"

#include "Memory/MemorySystem.h"

#include "precomputed_sky.h"

namespace stone
{
namespace tech
{
// ------------------------------------------------------------------

class SkyRuntime : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "sky runtime";

private:
	struct SceneData
	{
		floral::mat4x4f							modelFromView;
		floral::mat4x4f							viewFromClip;
	};

	struct TextureInfoData
	{
		s32										transmittanceTextureWidth;	// N
		s32										transmittanceTextureHeight;	// N

		s32										scatteringTextureRSize;		// N
		s32										scatteringTextureMuSize;	// N
		s32										scatteringTextureMuSSize;	// N
		s32										scatteringTextureNuSize;	// N

		s32										scatteringTextureWidth;		// N
		s32										scatteringTextureHeight;	// N
		s32										scatteringTextureDepth;		// N

		s32										irrandianceTextureWidth;	// N
		s32										irrandianceTextureHeight;	// N
	};

	struct AtmosphereData
	{
		floral::vec4f							solarIrradiance;		// 4N
		floral::vec4f							rayleighScattering;		// 4N
		floral::vec3f							mieScattering;			// 4N, must be a vec3 because the next element's base alignment is N
		f32										sunAngularRadius;		// N
		f32										bottomRadius;			// N
		f32										topRadius;				// N
		f32										miePhaseFunctionG;		// N
		f32										muSMin;					// N
	};

	struct ConfigsData
	{
		floral::vec4f							camera;
		floral::vec4f							whitePoint;
		floral::vec4f							earthCenter;
		floral::vec4f							sunDirection;
		floral::vec2f							sunSize;
		f32										exposure;
		f32										unitLengthInMeters;
	};

public:
	SkyRuntime();
	~SkyRuntime();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	insigne::texture_handle_t					LoadRawHDRTexture2D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_channel);
	insigne::texture_handle_t					LoadRawHDRTexture3D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel);

private:
	SkyFixedConfigs								m_SkyFixedConfigs;

	SceneData									m_SceneData;
	TextureInfoData								m_TextureInfoData;
	AtmosphereData								m_AtmosphereData;
	ConfigsData									m_ConfigsData;
	f32											m_ViewDistance; // in meters
	f32											m_ViewZenith; // in rad
	f32											m_ViewAzimuth; // in rad
	f32											m_SunZenith; // in rad
	f32											m_SunAzimuth; // in rad

	helpers::SurfaceGPU							m_Quad;

	mat_loader::MaterialShaderPair				m_MSPair;
	insigne::texture_handle_t					m_TransmittanceTexture;
	insigne::texture_handle_t					m_ScatteringTexture;
	insigne::texture_handle_t					m_IrradianceTexture;

	insigne::ub_handle_t						m_SceneUB;
	insigne::ub_handle_t						m_TextureInfoUB;
	insigne::ub_handle_t						m_AtmosphereUB;
	insigne::ub_handle_t						m_ConfigsUB;

private:
	helich::memory_region<LinearArena>			m_TexDataArenaRegion;
	LinearArena									m_TexDataArena;

	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
};

// ------------------------------------------------------------------
}
}
