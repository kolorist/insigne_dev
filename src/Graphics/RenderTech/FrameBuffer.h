#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace tech
{

class FrameBuffer : public ITestSuite
{
public:
	FrameBuffer();
	~FrameBuffer();

	const_cstr									GetName() const override;

	void										OnInitialize(floral::filesystem<FreelistArena>* i_fs) override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;

	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

	insigne::framebuffer_handle_t				m_PostFXBuffer;
	insigne::shader_handle_t					m_GrayScaleShader;
	insigne::material_desc_t					m_GrayScaleMaterial;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
