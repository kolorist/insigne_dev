#pragma once

#include "ITestSuite.h"
#include "Graphics/IDebugUI.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/FreeCamera.h"

namespace stone
{

class InterreflectPRT : public ITestSuite, public IDebugUI
{
public:
	InterreflectPRT();
	~InterreflectPRT();

	void										OnInitialize() override;
	void										OnUpdate(const f32 i_deltaMs) override;
	void										OnDebugUIUpdate(const f32 i_deltaMs) override;
	void										OnRender(const f32 i_deltaMs) override;
	void										OnCleanUp() override;

	ICameraMotion*								GetCameraMotion() override { return &m_CameraMotion; }

private:
	void										ComputeLightSH();
	void										ComputePRT();

private:
	struct SceneData
	{
		floral::mat4x4f							XForm;
		floral::mat4x4f							WVP;
	};

	struct SceneLight
	{
		floral::vec4f							LightSH[9];
	};

private:
	floral::fixed_array<VertexPNCSH, LinearAllocator>	m_Vertices;
	floral::fixed_array<s32, LinearAllocator>			m_Indices;
	floral::fixed_array<VertexP, LinearAllocator>		m_MnfVertices;
	floral::fixed_array<s32, LinearAllocator>			m_MnfIndices;

	SceneData									m_SceneData;
	SceneLight									m_SceneLight;

	insigne::vb_handle_t						m_VB;
	insigne::ib_handle_t						m_IB;
	insigne::shader_handle_t					m_Shader;
	insigne::material_desc_t					m_Material;

	insigne::ub_handle_t						m_UB;
	insigne::ub_handle_t						m_LightUB;

	insigne::framebuffer_handle_t				m_HDRBuffer;
	insigne::vb_handle_t						m_SSVB;
	insigne::ib_handle_t						m_SSIB;
	insigne::shader_handle_t					m_ToneMapShader;
	insigne::material_desc_t					m_ToneMapMaterial;

private:
	DebugDrawer									m_DebugDrawer;
	FreeCamera									m_CameraMotion;
	LinearArena*								m_MemoryArena;
};

}
