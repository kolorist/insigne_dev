#pragma once

#include <floral.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone {

class FormFactorsBaking : public ITestSuite {
	public:
		FormFactorsBaking();
		~FormFactorsBaking();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return nullptr; }

	private:
		void									CalculateFormfactors();
		void									CalculateRadiosity();

	private:
		struct SceneData {
			floral::mat4x4f						WVP;
		};

	private:
		DebugDrawer								m_DebugDrawer;

		floral::fixed_array<VertexPNCC, LinearAllocator>	m_GeoVertices;
		floral::fixed_array<u32, LinearAllocator>		m_GeoIndices;
		floral::fixed_array<GeoQuad, LinearAllocator>	m_GeoPatches;

		insigne::vb_handle_t					m_VB;
		insigne::ib_handle_t					m_IB;
		insigne::ub_handle_t					m_UB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
		floral::mat4x4f							m_WVP;

		LinearArena*							m_MemoryArena;
};

}
