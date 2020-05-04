#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include <refrain2.h>

#include "Graphics/TestSuite.h"
#include "Graphics/TrackballCamera.h"
#include "Graphics/MaterialLoader.h"
#include "Graphics/PostFXChain.h"
#include "Graphics/InsigneHelpers.h"

#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{
// -------------------------------------------------------------------

enum class ProjectionScheme
{
	LightProbe = 0,
	HStrip,
	Equirectangular,
	Invalid
};

// -------------------------------------------------------------------

class SHCalculator : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "sh calculator";

public:
	SHCalculator();
	~SHCalculator();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	void										_ComputeSH();
	void										_ComputeDebugSH(const u32 i_faceIdx);

	void										_LoadHDRImage(const_cstr i_fileName);
	void										_LoadMaterial(mat_loader::MaterialShaderPair* o_msPair, const floral::path& i_path);

private:
	struct SHComputeData
	{
		LinearArena* LocalMemoryArena;
		f32* InputTexture;
		f32* OutputRadianceTex;
		f32* OutputIrradianceTex;
		floral::vec3f OutputCoeffs[9];
		u32 Resolution;
		s32 Projection;
		u32 DebugFaceIndex;
	};

	SHComputeData								m_SHComputeTaskData;
	static refrain2::Task						ComputeSHCoeffs(voidptr i_data);
	static refrain2::Task						ComputeDebugSHCoeffs(voidptr i_data);

private:

	struct SceneData
	{
		floral::mat4x4f							viewProjectionMatrix;
		floral::vec4f							cameraPosition;
		floral::vec4f							SH[9];
	};

	struct IBLBakeSceneData
	{
		floral::mat4x4f							WVP;
		floral::vec4f							CameraPos;
	};

	struct PrefilterConfigs
	{
		floral::vec2f							roughness;
	};

	struct PreviewConfigs
	{
		floral::vec2f							texLod;
	};

private:
	TrackballCamera								m_CameraMotion;
	SceneData									m_SceneData;

	IBLBakeSceneData							m_IBLBakeSceneData;
	PrefilterConfigs							m_PrefilterConfigs;
	PreviewConfigs								m_PreviewConfigs;
	PreviewConfigs								m_PreviewSpecConfigs;
	floral::camera_view_t						m_IBLBakeView;
	floral::camera_persp_t						m_IBLBakeProjection;

private:
	FreelistArena*								m_TemporalArena;
	FreelistArena*								m_MemoryArena;
	LinearArena*								m_MaterialDataArena;
	LinearArena*								m_PostFXArena;

private:
	mat_loader::MaterialShaderPair				m_PMREMBakeMSPair;

	helpers::SurfaceGPU							m_PMREMSkybox;

	pfx_chain::PostFXChain<LinearArena, FreelistArena>	m_PostFXChain;

	ProjectionScheme							m_CurrentProjectionScheme;

	// previewing for imgui
	bool										m_ImgLoaded;
	insigne::texture_handle_t					m_PreviewTexture[3];
	insigne::texture_handle_t					m_CurrentPreviewTexture;
	floral::vec3f								m_MinHDR;
	floral::vec3f								m_MaxHDR;

	// previewing for probe and skybox
	helpers::SurfaceGPU							m_Sphere;
	helpers::SurfaceGPU							m_Skysphere;
	mat_loader::MaterialShaderPair				m_SHPreviewMSPair;
	mat_loader::MaterialShaderPair				m_SHSkyPreviewMSPair;
	mat_loader::MaterialShaderPair				m_PreviewMSPair[3];
	mat_loader::MaterialShaderPair				m_SkyPreviewMSPair[3];
	mat_loader::MaterialShaderPair*				m_CurrentPreviewMSPair;
	mat_loader::MaterialShaderPair*				m_CurrentSkyPreviewMSPair;
	insigne::texture_handle_t					m_Current3DPreviewTexture;

	// sh calculation
	insigne::texture_handle_t					m_SHPreviewRadianceTex;
	insigne::texture_handle_t					m_SHPreviewIrradianceTex;
	f32*										m_SHInputTexData;
	f32*										m_SHRadTexData;
	f32*										m_SHIrrTexData;
	bool										m_ComputingSH;
	bool										m_SHReady;
	std::atomic<u32>							m_Counter;

	// pmrem calculation
	mat_loader::MaterialShaderPair				m_PMREMPreviewMSPair;
	mat_loader::MaterialShaderPair				m_PMREMSkyPreviewMSPair;
	bool										m_NeedBakePMREM;
	bool										m_PMREMReady;

	// for processing
	insigne::texture_handle_t					m_InputTexture[3];

	// ub(s)
	insigne::ub_handle_t						m_UB;
	insigne::ub_handle_t						m_IBLBakeSceneUB;
	insigne::ub_handle_t						m_PrefilterUB;
	insigne::ub_handle_t						m_PreviewUB;
	insigne::ub_handle_t						m_PreviewPMREMUB;
	insigne::framebuffer_handle_t				m_SpecularFB;

	insigne::texture_handle_t					m_CurrentInputTexture;

	insigne::material_desc_t*					m_CurrentPreviewMat;

	f32*										m_SpecImgData;
	u64											m_SpecPromisedFrame;
	bool										m_IsCapturingSpecData;

	floral::vec3f								m_CamPos;
};

// -------------------------------------------------------------------
}
}
