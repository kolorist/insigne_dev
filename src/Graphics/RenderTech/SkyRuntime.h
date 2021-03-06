#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Graphics/InsigneHelpers.h"
#include "Graphics/MaterialLoader.h"
#include "Graphics/PostFXChain.h"

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

	struct ObjectSceneData
	{
		floral::mat4x4f							viewProjectionMatrix;
		floral::mat4x4f							transformMatrix;
		floral::vec4f							cameraPosition;
	};

	struct ConvolutionSceneData
	{
		floral::mat4x4f							viewProjectionMatrix;
		f32										roughness;
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
	ObjectSceneData								m_ObjectSceneData;
	ConvolutionSceneData						m_ConvolutionSceneData;
	TextureInfoData								m_TextureInfoData;
	AtmosphereData								m_AtmosphereData;
	ConfigsData									m_ConfigsData;
	floral::vec3f								m_LookAt;
	floral::vec3f								m_CamPos;
	f32											m_AspectRatio;
	f32											m_FovY;
	floral::mat4x4f								m_ProjectionMatrix;
	floral::mat4x4f								m_ViewProjectionMatrix;
	f32											m_SunZenith; // in rad
	f32											m_SunAzimuth; // in rad

	helpers::SurfaceGPU							m_Quad;
	helpers::SurfaceGPU							m_Surface;
	helpers::SurfaceGPU							m_HelmetSurfaceGPU;

	mat_loader::MaterialShaderPair				m_MSPair;
	mat_loader::MaterialShaderPair				m_SphereMSPair;
	mat_loader::MaterialShaderPair				m_ConvoMSPair;
	mat_loader::MaterialShaderPair				m_HelmetMSPair;
	insigne::texture_handle_t					m_TransmittanceTexture;
	insigne::texture_handle_t					m_ScatteringTexture;
	insigne::texture_handle_t					m_IrradianceTexture;
	insigne::texture_handle_t					m_SplitSumTexture;

	insigne::ub_handle_t						m_SceneUB;
	insigne::ub_handle_t						m_ConvolutionUB;
	insigne::ub_handle_t						m_ObjectSceneUB;
	insigne::ub_handle_t						m_TextureInfoUB;
	insigne::ub_handle_t						m_AtmosphereUB;
	insigne::ub_handle_t						m_ConfigsUB;

	insigne::framebuffer_handle_t				m_PreConvoFB;
	insigne::framebuffer_handle_t				m_ConvolutionFB;

	bool										m_PreConvoDone;
	bool										m_ConvoDone;

	pfx_chain::PostFXChain<LinearArena, FreelistArena>	m_PostFXChain;

private:
	helich::memory_region<LinearArena>			m_TexDataArenaRegion;
	LinearArena									m_TexDataArena;

	FreelistArena*								m_MemoryArena;
	LinearArena*								m_PostFXArena;
	LinearArena*								m_ModelDataArena;
	LinearArena*								m_MaterialDataArena;
};

// ------------------------------------------------------------------
}
}
