#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/TrackballCamera.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

class PBRWithIBL : public ITestSuite
{
public:
	PBRWithIBL();
	~PBRWithIBL();

	const_cstr									GetName() const override;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	struct SceneData
	{
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
		floral::vec4f							CameraPos;
	};

	struct IBLBakeSceneData
	{
		floral::mat4x4f							WVP;
		floral::vec4f							CameraPos;
	};

	struct LightData
	{
		floral::vec4f							LightDirection;
		floral::vec4f							LightIntensity;
	};

	struct SurfaceData
	{
		floral::vec4f							BaseColor;
		floral::vec4f							Attributes;
	};

	struct PrefilterConfigs
	{
		floral::vec2f							roughness;
	};

private:
	SceneData									m_SceneData;
	IBLBakeSceneData							m_IBLSceneData;
	LightData									m_LightData;
	SurfaceData									m_SurfaceData;
	PrefilterConfigs							m_PrefilterConfigs;
	floral::camera_view_t						m_View;
	floral::camera_persp_t						m_Projection;

	TrackballCamera								m_CameraMotion;

private:
	insigne::vb_handle_t						m_VB;
	insigne::vb_handle_t						m_SphereVB;
	insigne::ib_handle_t						m_IB;
	insigne::ib_handle_t						m_SphereIB;
	insigne::ub_handle_t						m_SceneUB;
	insigne::ub_handle_t						m_IBLBakeSceneUB;
	insigne::ub_handle_t						m_LightUB;
	insigne::ub_handle_t						m_SurfaceUB;
	insigne::ub_handle_t						m_PrefilterUB;

	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;

	insigne::texture_handle_t					m_CubeMapTex;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;
	insigne::shader_handle_t					m_CubeMapShader;
	insigne::material_desc_t					m_CubeMapMaterial;
	insigne::shader_handle_t					m_PMREMShader;
	insigne::material_desc_t					m_PMREMMaterial;

	insigne::framebuffer_handle_t				m_PostFXBuffer;
	insigne::framebuffer_handle_t				m_SpecularFB;
	insigne::shader_handle_t					m_ToneMapShader;
	insigne::material_desc_t					m_ToneMapMaterial;

	bool										m_NeedBakePMREM;

private:
	LinearArena*								m_MemoryArena;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
