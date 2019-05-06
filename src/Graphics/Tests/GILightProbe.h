#pragma once

#include <floral/stdaliases.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"

namespace stone
{

class GILightProbe : public ITestSuite, public IDebugUI
{
public:
	GILightProbe();
	~GILightProbe();

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

private:
	floral::fixed_array<VertexPNC, LinearAllocator>		m_Vertices;
	floral::fixed_array<u32, LinearAllocator>			m_Indices;
	floral::fixed_array<GeoQuad, LinearAllocator>		m_Patches;

	SceneData									m_SceneData;

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::ub_handle_t						m_UB;
	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

	LinearArena*								m_MemoryArena;
	DebugDrawer									m_DebugDrawer;
	FreeCamera									m_CameraMotion;
};

}
