#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/TrackballCamera.h"
#include "Graphics/DebugDrawer.h"

namespace stone {

class SHMath : public ITestSuite {
	public:
		struct SHData {
			floral::vec4f						CoEffs[9];
		};

	public:
		SHMath();
		~SHMath();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return &m_CameraMotion; }

	private:
		SHData									ReadSHDataFromFile(const_cstr i_filePath);
		SHData									LinearInterpolate(const SHData& d0, const SHData& d1, const f32 weight);
		floral::vec3f							EvalSH(const SHData& i_shData, const floral::vec3f& i_normal);

	private:
		// camera motion
		DebugDrawer								m_DebugDrawer;
		TrackballCamera							m_CameraMotion;

		floral::fixed_array<DemoVertex, LinearAllocator>	m_Vertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;
		floral::fixed_array<VertexPNC, LinearAllocator>		m_SurfVertices;
		floral::fixed_array<u32, LinearAllocator>			m_SurfIndices;

		struct SceneData {
			floral::mat4x4f						XForm;
			floral::mat4x4f						WVP;
		};

		SceneData								m_SceneData;

		SHData									m_RedSH;
		SHData									m_GreenSH;
		SHData									m_AvgSH;

		insigne::vb_handle_t					m_VB;
		insigne::vb_handle_t					m_SurfVB;
		insigne::ib_handle_t					m_IB;
		insigne::ib_handle_t					m_SurfIB;
		insigne::ub_handle_t					m_UB;
		insigne::ub_handle_t					m_SHUB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		insigne::texture_handle_t				m_Texture;
		insigne::texture_handle_t				m_Texture2;
		insigne::shader_handle_t				m_CubeShader;
		insigne::material_desc_t				m_CubeMaterial;
		insigne::shader_handle_t				m_SurfShader;
		insigne::material_desc_t				m_SurfMaterial;

		LinearArena*							m_MemoryArena;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
		floral::mat4x4f							m_DebugWVP;
};

}
