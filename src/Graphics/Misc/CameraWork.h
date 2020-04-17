#pragma once

#include <floral/stdaliases.h>

#include <insigne/commons.h>

#include "Graphics/TestSuite.h"
#include "Memory/MemorySystem.h"

namespace stone
{
namespace misc
{
// ------------------------------------------------------------------

class CameraWork : public TestSuite
{
public:
	static constexpr const_cstr k_name			= "camera work";

public:
	CameraWork();
	~CameraWork();

	ICameraMotion*								GetCameraMotion() override;
	const_cstr									GetName() const override;

private:
	void										_OnInitialize() override;
	void										_OnUpdate(const f32 i_deltaMs) override;
	void										_OnRender(const f32 i_deltaMs) override;
	void										_OnCleanUp() override;

private:
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;

	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;
};

// ------------------------------------------------------------------
}
}
