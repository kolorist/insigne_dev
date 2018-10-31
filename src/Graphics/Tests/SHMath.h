#pragma once

#include <floral.h>
#include <insigne/commons.h>

#include "ITestSuite.h"

#include "Memory/MemorySystem.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/TrackballCamera.h"
#include "Graphics/DebugDrawer.h"

namespace stone {

#if 0
const mediump float c1 = 0.429043f;
const mediump float c2 = 0.511664f;
const mediump float c3 = 0.743125f;
const mediump float c4 = 0.886227f;
const mediump float c5 = 0.247708f;

	return
		// constant term, lowest frequency //////
		c4 * iu_Coeffs[0].xyz +

		// axis aligned terms ///////////////////
		2.0f * c2 * iu_Coeffs[1].xyz * i_normal.y +
		2.0f * c2 * iu_Coeffs[2].xyz * i_normal.z +
		2.0f * c2 * iu_Coeffs[3].xyz * i_normal.x +

		// band 2 terms /////////////////////////
		2.0f * c1 * iu_Coeffs[4].xyz * i_normal.x * i_normal.y +
		2.0f * c1 * iu_Coeffs[5].xyz * i_normal.y * i_normal.z +
		2.0f * c1 * iu_Coeffs[7].xyz * i_normal.x * i_normal.z +
		c3 * iu_Coeffs[6].xyz * i_normal.z * i_normal.z - c5 * iu_Coeffs[6].xyz +
		c1 * iu_Coeffs[8].xyz * (i_normal.x * i_normal.x - i_normal.y * i_normal.y);
#endif

class SHMath : public ITestSuite {
	public:
		SHMath();
		~SHMath();

		void									OnInitialize() override;
		void									OnUpdate(const f32 i_deltaMs) override;
		void									OnRender(const f32 i_deltaMs) override;
		void									OnCleanUp() override;

		ICameraMotion*							GetCameraMotion() override { return &m_CameraMotion; }

	private:
		// camera motion
		DebugDrawer								m_DebugDrawer;
		TrackballCamera							m_CameraMotion;

		floral::fixed_array<DemoVertex, LinearAllocator>	m_Vertices;
		floral::fixed_array<u32, LinearAllocator>			m_Indices;

		struct SceneData {
			floral::mat4x4f						XForm;
			floral::mat4x4f						WVP;
		};

		struct SHData {
			floral::vec4f						CoEffs[9];
		};

		SceneData								m_SceneData;

		insigne::vb_handle_t					m_VB;
		insigne::ib_handle_t					m_IB;
		insigne::ub_handle_t					m_UB;
		insigne::ub_handle_t					m_SHUB;
		insigne::shader_handle_t				m_Shader;
		insigne::material_desc_t				m_Material;

		insigne::texture_handle_t				m_Texture;
		insigne::texture_handle_t				m_Texture2;
		insigne::shader_handle_t				m_CubeShader;
		insigne::material_desc_t				m_CubeMaterial;

		LinearArena*							m_MemoryArena;

		floral::camera_view_t					m_CamView;
		floral::camera_persp_t					m_CamProj;
		floral::mat4x4f							m_DebugWVP;
};

}
