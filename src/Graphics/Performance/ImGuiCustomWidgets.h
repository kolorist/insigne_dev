#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/Tests/ITestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace perf
{

class ImGuiCustomWidgets : public ITestSuite
{
public:
	ImGuiCustomWidgets();
	~ImGuiCustomWidgets();

	const_cstr									GetName() const override;

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

	floral::mat4x4f								m_ViewMatrix;
	floral::mat4x4f								m_ProjectionMatrix;

	// resource control
private:
	ssize										m_BuffersBeginStateId;
	ssize										m_ShadingBeginStateId;
	ssize										m_TextureBeginStateId;
	ssize										m_RenderBeginStateId;
};

}
}
