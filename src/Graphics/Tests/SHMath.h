#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class SHMath : public ITestSuite {
	public:
		SHMath();
		~SHMath();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

	private:
		floral::fixed_array<DemoVertex, LinearAllocator>	m_Vertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;

		struct SceneData {
			floral::mat4x4f						XForm;
			floral::mat4x4f						WVP;
		};

		struct SHData {
			floral::vec4f						CoEffs[9];
		};

		floral::fixed_array<SHData, LinearAllocator>	m_SHData;

		SceneData								m_SceneData;

		insigne::vb_handle_t					m_VB;
		insigne::ib_handle_t					m_IB;
		insigne::ub_handle_t					m_UB;
		insigne::ub_handle_t					m_SHUB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		LinearArena*							m_MemoryArena;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
};

}
