#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class CbFormats : public ITestSuite {
	public:
		CbFormats();
		~CbFormats();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return nullptr; }

	private:
		struct SceneData {
			floral::mat4x4f						XForm;
			floral::mat4x4f						WVP;
		};

		SceneData								m_SceneData;

		floral::fixed_array<insigne::vb_handle_t, LinearAllocator>	m_VBs;
		floral::fixed_array<insigne::ib_handle_t, LinearAllocator>	m_IBs;

		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;

		LinearArena*							m_MemoryArena;
		LinearArena*							m_PlyReaderArena;
};

}
