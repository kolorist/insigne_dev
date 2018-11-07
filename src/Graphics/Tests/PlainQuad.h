#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class ICameraMotion;

class PlainQuadTest : public ITestSuite {
	public:
		PlainQuadTest();
		~PlainQuadTest();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return nullptr; }

	private:
		floral::fixed_array<VertexPC, LinearAllocator>	m_Vertices;
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
};

}
