#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include <refrain2.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Graphics/TrackballCamera.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

class SHCalculator : public ITestSuite
{
public:
	SHCalculator();
	~SHCalculator();

	const_cstr									GetName() const override;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	void										_ComputeSH();
	void										_ComputeDebugSH(const u32 i_faceIdx);

	void										_LoadHDRImage(const_cstr i_fileName);

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
		floral::mat4x4f							WVP;
		floral::vec4f							SH[9];
	};

private:
	TrackballCamera								m_CameraMotion;
	SceneData									m_SceneData;

private:
	FreelistArena*								m_TemporalArena;

private:
	insigne::vb_handle_t						m_ProbeVB;
	insigne::ib_handle_t						m_ProbeIB;
	insigne::ub_handle_t						m_UB;
	insigne::shader_handle_t					m_ProbeShader;
	insigne::material_desc_t					m_ProbeMaterial;

	insigne::shader_handle_t					m_PreviewShader[3];
	insigne::material_desc_t					m_PreviewMaterial[3];

	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;
	insigne::framebuffer_handle_t				m_PostFXBuffer;
	insigne::shader_handle_t					m_ToneMapShader;
	insigne::material_desc_t					m_ToneMapMaterial;

	insigne::texture_handle_t					m_Texture[3];
	floral::vec2i								m_InputTextureSize;
	insigne::texture_handle_t					m_CurrentInputTexture;
	insigne::texture_handle_t					m_CurrentPreviewTexture;
	insigne::texture_handle_t					m_CurrentInputTexture3D;
	insigne::texture_handle_t					m_RadianceTexture;
	insigne::texture_handle_t					m_IrradianceTexture;
	f32*										m_TextureData;
	f32*										m_RadTextureData;
	f32*										m_IrrTextureData;
	u32											m_TextureResolution;
	s32											m_Projection;
	floral::vec3f								m_MinHDR;
	floral::vec3f								m_MaxHDR;

	bool										m_ImgLoaded;
	bool										m_ComputingSH;
	bool										m_SHReady;
	bool										m_SpecReady;
	insigne::material_desc_t*					m_CurrentPreviewMat;

	floral::vec3f								m_CamPos;

	std::atomic<u32>							m_Counter;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
