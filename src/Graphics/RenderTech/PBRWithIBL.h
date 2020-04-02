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
	const insigne::texture_handle_t _Load2DTexture(const floral::path& i_path);

private:
	struct SceneData
	{
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
		floral::vec4f							CameraPos;
	};

	struct LightData
	{
		floral::vec4f							worldLightDirection;			// 4N
		floral::vec3f							worldLightIntensity;			// this vec3 will be merged with the s32 below
		s32										pointLightsCount;				// results in a 4N basic unit machine
		floral::vec4f							pointLightPositions[4];			// 4N * 4
		floral::vec4f							pointLightAttributes[4];		// 4N * 4
	};

	struct SHData
	{
		floral::vec4f							SH[9];
	};

private:
	SceneData									m_SceneData;
	LightData									m_LightData;
	SHData										m_SHData;

	TrackballCamera								m_CameraMotion;

private:
	insigne::vb_handle_t						m_VB;
	insigne::vb_handle_t						m_SphereVB;
	insigne::ib_handle_t						m_IB;
	insigne::ib_handle_t						m_SphereIB;
	insigne::ub_handle_t						m_SceneUB;
	insigne::ub_handle_t						m_LightUB;
	insigne::ub_handle_t						m_SurfaceUB;
	insigne::ub_handle_t						m_SHUB;

	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;

	insigne::texture_handle_t					m_CubeMapTex;
	insigne::texture_handle_t					m_AlbedoTex;
	insigne::texture_handle_t					m_AttributeTex;
	insigne::texture_handle_t					m_NormalTex;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;
	insigne::shader_handle_t					m_CubeMapShader;
	insigne::material_desc_t					m_CubeMapMaterial;

	insigne::shader_handle_t					m_BRDFShader;
	insigne::material_desc_t					m_BRDFMaterial;

	insigne::shader_handle_t					m_PBRShader;
	insigne::material_desc_t					m_PBRMaterial;

	insigne::framebuffer_handle_t				m_PostFXBuffer;
	insigne::framebuffer_handle_t				m_BrdfFB;

	insigne::shader_handle_t					m_ToneMapShader;
	insigne::material_desc_t					m_ToneMapMaterial;

	bool										m_PBRReady;
	bool										m_BakingBRDF;
	bool										m_LoadingTextures;

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
