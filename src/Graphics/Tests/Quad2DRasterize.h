#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"

#include "Memory/MemorySystem.h"
#include "Graphics/FreeCamera.h"
#include "Graphics/DebugDrawer.h"

namespace stone {

class Quad2DRasterize : public ITestSuite, public IDebugUI
{
public:
	Quad2DRasterize();
	~Quad2DRasterize();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	insigne::ub_handle_t						m_UB;
	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::material_desc_t					m_Material;

	insigne::shader_handle_t					m_Shader;
	insigne::texture_handle_t					m_Texture;

private:
	LinearArena*								m_MemoryArena;
};

}
