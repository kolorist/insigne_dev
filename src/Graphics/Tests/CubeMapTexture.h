#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class CubeMapTexture : public ITestSuite {
	public:
		CubeMapTexture();
		~CubeMapTexture();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

	private:
		floral::fixed_array<DemoVertex, LinearAllocator>	m_Vertices;
		floral::fixed_array<DebugVertex, LinearAllocator>	m_DebugVertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;
		floral::fixed_array<u32, LinearAllocator>			m_DebugIndices;
		struct MyData {
			floral::mat4x4f						WVP;
		};
		MyData									m_Data;

		struct MySSData {
			floral::vec2f						SSPosition;
		};
		MySSData								m_SSData;

		insigne::vb_handle_t					m_VB;
		insigne::vb_handle_t					m_DebugVB;
		insigne::ib_handle_t					m_IB;
		insigne::ib_handle_t					m_DebugIB;
		insigne::ub_handle_t					m_UB;
		insigne::ub_handle_t					m_DebugUB;
		insigne::shader_handle_t				m_Shader;
		insigne::shader_handle_t				m_DebugShader;
		insigne::material_desc_t				m_Material;
		insigne::material_desc_t				m_DebugMaterial;
		insigne::texture_handle_t				m_Texture;

		LinearArena*							m_MemoryArena;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
};

}
