#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/FreeCamera.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

class PBR : public ITestSuite
{
public:
	PBR();
	~PBR();

	const_cstr									GetName() const override;

	void										OnInitialize(floral::filesystem<FreelistArena>* i_fs, const FontRenderer* i_fontRenderer) override;
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

private:
	SceneData									m_SceneData;
	LightData									m_LightData;
	SurfaceData									m_SurfaceData;

	FreeCamera									m_CameraMotion;

private:
	insigne::vb_handle_t						m_VB;
	insigne::vb_handle_t						m_SphereVB;
	insigne::ib_handle_t						m_IB;
	insigne::ib_handle_t						m_SphereIB;
	insigne::ub_handle_t						m_SceneUB;
	insigne::ub_handle_t						m_LightUB;
	insigne::ub_handle_t						m_SurfaceUB;

	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;
	insigne::shader_handle_t					m_PBRShader;
	insigne::material_desc_t					m_PBRMaterial;

	insigne::framebuffer_handle_t				m_PostFXBuffer;
	insigne::shader_handle_t					m_ToneMapShader;
	insigne::material_desc_t					m_ToneMapMaterial;

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
