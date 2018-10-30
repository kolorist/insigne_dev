#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/TrackballCamera.h"

namespace stone {

class GlobalIllumination : public ITestSuite {
	public:
		struct SHData {
			floral::vec4f						CoEffs[9];
		};

		GlobalIllumination();
		~GlobalIllumination();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		static SHData							LinearInterpolate(const SHData& d0, const SHData& d1, const f32 weight);

		ICameraMotion*							GetCameraMotion() override { return &m_CameraMotion; }

	private:
		DebugDrawer								m_DebugDrawer;
		TrackballCamera							m_CameraMotion;

		floral::fixed_array<VertexPNC, LinearAllocator>		m_Vertices;
		floral::fixed_array<DemoTexturedVertex, LinearAllocator>	m_SSVertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;
		floral::fixed_array<u32, LinearAllocator>			m_SSIndices;
		floral::fixed_array<floral::mat4x4f, LinearAllocator>	m_SHWVPs;

		struct LightData {
			floral::vec4f						Direction;	// uniform buffer layout, sorry
			floral::vec4f						Color;
			floral::vec4f						Radiance;
		};

		struct SceneData {
			floral::mat4x4f						XForm;
			floral::mat4x4f						WVP;
			floral::vec4f						CameraPos;
		};

		floral::fixed_array<floral::vec3f, LinearAllocator>	m_SHPos;
		floral::fixed_array<SHData, LinearAllocator>	m_SHData;

		SceneData								m_SceneData;
		LightData								m_LightData;
		SceneData								m_ShadowSceneData;
		floral::aabb3f							m_SceneAABB;

		insigne::vb_handle_t					m_VB;
		insigne::vb_handle_t					m_SSVB;
		insigne::ib_handle_t					m_IB;
		insigne::ib_handle_t					m_SSIB;
		insigne::ub_handle_t					m_UB;
		insigne::ub_handle_t					m_LightDataUB;
		insigne::ub_handle_t					m_ShadowUB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;
		insigne::shader_handle_t				m_ShadowShader;
		insigne::material_desc_t				m_ShadowMaterial;

		insigne::shader_handle_t				m_FinalBlitShader;
		insigne::material_desc_t				m_FinalBlitMaterial;

		insigne::framebuffer_handle_t			m_SHRenderBuffer;
		floral::fixed_array<floral::vec3f, LinearAllocator>	m_SHCamPos;

		insigne::framebuffer_handle_t			m_ShadowRenderBuffer;
		insigne::framebuffer_handle_t			m_MainRenderBuffer;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;

		floral::camera_view_t					m_ShadowCamView;
		floral::camera_ortho_t					m_ShadowCamProj;
		LinearArena*							m_MemoryArena;
};

}
