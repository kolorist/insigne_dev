#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"

#include "Memory/MemorySystem.h"
#include "Graphics/FreeCamera.h"
#include "Graphics/DebugDrawer.h"

namespace stone {

class GPUQuad2DRasterize : public ITestSuite, public IDebugUI
{
private:
	struct SceneData
	{
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	SceneData									m_SceneData;

public:
	GPUQuad2DRasterize();
	~GPUQuad2DRasterize();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	insigne::ub_handle_t						m_UB;

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

private:
	DebugDrawer									m_DebugDrawer;
	LinearArena*								m_MemoryArena;
	FreeCamera									m_CameraMotion;
};

}
