#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class ICameraMotion;

class SHBaking : public ITestSuite {
	public:
		SHBaking();
		~SHBaking();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return nullptr; }

	private:
		struct SceneData {
			floral::mat4x4f						WVP;
		};

		struct SHProbeData {
			floral::mat4x4f						XForm;
			floral::vec4f						CoEffs[9];
		};

	private:
		floral::fixed_array<VertexPNC, LinearAllocator>	m_GeoVertices;
		floral::fixed_array<u32, LinearAllocator>		m_GeoIndices;

		floral::fixed_array<VertexP, LinearAllocator>	m_ProbeVertices;
		floral::fixed_array<u32, LinearAllocator>		m_ProbeIndices;

		floral::fixed_array<floral::vec3f, LinearAllocator>		m_SHPositions;
		floral::fixed_array<SceneData, LinearAllocator>			m_EnvSceneData;
		floral::fixed_array<SHProbeData, LinearAllocator>		m_SHData;

		insigne::vb_handle_t					m_VB;
		insigne::ib_handle_t					m_IB;
		insigne::ub_handle_t					m_UB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		insigne::vb_handle_t					m_ProbeVB;
		insigne::ib_handle_t					m_ProbeIB;
		insigne::ub_handle_t					m_ProbeUB;
		insigne::shader_handle_t				m_ProbeShader;
		insigne::material_desc_t				m_ProbeMaterial;

		insigne::ub_handle_t					m_EnvMapSceneUB;
		insigne::material_desc_t				m_EnvMapMaterial;
		insigne::framebuffer_handle_t			m_EnvMapRenderBuffer;
		f32*									m_EnvMapPixelData;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
};

}
