#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class OmniShadow : public ITestSuite {
	public:
		OmniShadow();
		~OmniShadow();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

	private:
		floral::fixed_array<VertexPNC, LinearAllocator>		m_Vertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;

		struct LightData {
			floral::vec4f						Position;	// uniform buffer layout, sorry
			floral::vec4f						Color;
			floral::vec4f						Radiance;
		};

		struct SceneData {
			floral::mat4x4f						XForm;
			floral::mat4x4f						WVP;
		};

		SceneData								m_SceneData;
		LightData								m_LightData;

		insigne::vb_handle_t					m_VB;
		insigne::ib_handle_t					m_IB;
		insigne::ub_handle_t					m_UB;
		insigne::ub_handle_t					m_LightDataUB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		insigne::framebuffer_handle_t			m_ShadowRenderBuffer;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
};

}
