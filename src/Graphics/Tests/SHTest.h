#pragma once

#include <floral/stdaliases.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/FreeCamera.h"

namespace stone
{

class SHTest : public ITestSuite, public IDebugUI
{
public:
	SHTest();
	~SHTest();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	struct SHProbeData {
		floral::mat4x4f							XForm;
		floral::vec4f							CoEffs[9];
	};

private:
	floral::fixed_array<VertexP, LinearAllocator>	m_ProbeVertices;
	floral::fixed_array<u32, LinearAllocator>		m_ProbeIndices;
	SceneData									m_SceneData;
	SHProbeData									m_SHData;

	insigne::ub_handle_t						m_UB;

	insigne::vb_handle_t						m_ProbeVB;
	insigne::ib_handle_t						m_ProbeIB;
	insigne::ub_handle_t						m_ProbeUB;
	insigne::shader_handle_t					m_ProbeShader;
	insigne::material_desc_t					m_ProbeMaterial;

private:
	DebugDrawer									m_DebugDrawer;
	FreeCamera									m_CameraMotion;
};

}
