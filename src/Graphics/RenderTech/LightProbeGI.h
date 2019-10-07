#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/FreeCamera.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/RenderTech/SHProbeBaker.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

class LightProbeGI : public ITestSuite
{
public:
	LightProbeGI();
	~LightProbeGI();

	const_cstr									GetName() const override;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	void										_RenderSceneAlbedo(const floral::mat4x4f& i_wvp);
	void										_InitializeSHBaker(const floral::simple_callback<void>& i_renderCb);
	void										_UpdateSHBaker();

private:
	struct SceneData
	{
		floral::mat4x4f							WVP;
	};

	struct ProbeData
	{
		floral::mat4x4f							XForm;
		floral::vec4f							CoEffs[9];	// 3 bands
	};

	struct WorldData
	{
		floral::vec4f							BBMinCorner;
		floral::vec4f							BBDimension;
	};

	struct SHCoeffs
	{
		floral::vec4f							CoEffs[9];	// 3 bands
	};

	struct SHData
	{
		SHCoeffs								Probes[64];
	};

private:
	bool										m_SHBakingStarted;
	bool										m_SHReady;
	bool										m_DrawSHProbes;

	SceneData									m_SceneData;
	WorldData									m_WorldData;
	SHData*										m_SHData;
	FreeCamera									m_CameraMotion;
	SHProbeBaker								m_SHBaker;

	floral::fixed_array<floral::vec3f, LinearArena>	m_SHPositions;
	floral::fixed_array<VertexPNC, LinearArena>	m_VerticesData;
	floral::fixed_array<s32, LinearArena>		m_IndicesData;

private:
	insigne::vb_handle_t						m_VB;
	insigne::vb_handle_t						m_ProbeVB;
	insigne::ib_handle_t						m_IB;
	insigne::ib_handle_t						m_ProbeIB;
	insigne::ub_handle_t						m_UB;
	insigne::ub_handle_t						m_SHUB;
	insigne::ub_handle_t						m_ProbeUB;
	insigne::ub_handle_t						m_AlbedoUB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

	insigne::shader_handle_t					m_AlbedoShader;
	insigne::material_desc_t					m_AlbedoMaterial;

	insigne::shader_handle_t					m_ProbeShader;
	insigne::material_desc_t					m_ProbeMaterial;

	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;
	insigne::framebuffer_handle_t				m_PostFXBuffer;
	insigne::shader_handle_t					m_ToneMapShader;
	insigne::material_desc_t					m_ToneMapMaterial;

private:
	LinearArena*								m_ResourceArena;
	FreelistArena*								m_TemporalArena;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
