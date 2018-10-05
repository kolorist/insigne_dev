#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class PlainTextureQuad : public ITestSuite {
	public:
		PlainTextureQuad();
		~PlainTextureQuad();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

	private:
		floral::fixed_array<DemoTexturedVertex, LinearAllocator>	m_Vertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;
		struct MyData {
			floral::vec4f						Color;
		};
		MyData									m_Data;

		insigne::vb_handle_t					m_VB;
		insigne::ib_handle_t					m_IB;
		insigne::ub_handle_t					m_UB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;
		insigne::texture_handle_t				m_Texture;

		LinearArena*							m_MemoryArena;
};

}
