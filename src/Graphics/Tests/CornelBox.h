#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{

class CornelBox : public ITestSuite, public IDebugUI
{
public:
	CornelBox();
	~CornelBox();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return nullptr; }

private:
	floral::fixed_array<VertexPNC, LinearAllocator>		m_Vertices;
	floral::fixed_array<u32, LinearAllocator>			m_Indices;
	floral::fixed_array<GeoQuad, LinearAllocator>		m_Patches;


	struct SceneData {
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	SceneData									m_SceneData;

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::ub_handle_t						m_UB;
	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

	floral::camera_view_t						m_CamView;
	floral::camera_persp_t						m_CamProj;

	LinearArena*								m_MemoryArena;
};

}
