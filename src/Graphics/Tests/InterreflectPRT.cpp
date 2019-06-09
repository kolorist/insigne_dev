#include "InterreflectPRT.h"

#include <floral/comgeo/shapegen.h>

#include <clover/Logger.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include "Graphics/prt.h"
#include "Graphics/GeometryBuilder.h"

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec4 v_VertexColor;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_VertexColor;

void main()
{
	o_Color = v_VertexColor;
}
)";

InterreflectPRT::InterreflectPRT()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(3.0f, 3.0f, 0.0f), floral::vec3f(-3.0f, -3.0f, 0.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

InterreflectPRT::~InterreflectPRT()
{
}

void InterreflectPRT::OnInitialize()
{
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_SceneData.WVP = m_CameraMotion.GetWVP();
		m_SceneData.XForm = floral::mat4x4f(1.0f);

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	m_Vertices.init(4096u, &g_StreammingAllocator);
	m_Indices.init(8192u, &g_StreammingAllocator);

	{
		f32 subdivision = 0.2f;
		floral::mat4x4f mBottom = floral::construct_translation3d(0.0f, -1.0f, 0.0f);
		floral::mat4x4f mLeft = floral::construct_translation3d(0.0f, 0.0f, 1.5f)
			* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mRight = floral::construct_translation3d(0.0f, 0.0f, -1.5f)
			* floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mBack = floral::construct_translation3d(-1.0f, 0.0f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();

		GenTesselated3DPlane_Tris_PNC(
				mBottom,
				2.0f, 3.0f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mLeft,
				2.0f, 2.0f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mRight,
				2.0f, 2.0f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mBack,
				2.0f, 3.0f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);

		floral::mat4x4f mB1Top = floral::construct_translation3d(0.0f, 0.4f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 43.0f, 0.0f).to_transform();
		floral::mat4x4f mB1Front = floral::construct_quaternion_euler(0.0f, 43.0f, 0.0f).to_transform()
			* floral::construct_translation3d(0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();
		floral::mat4x4f mB1Back = floral::construct_quaternion_euler(0.0f, 43.0f, 0.0f).to_transform()
			* floral::construct_translation3d(-0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, 90.0f).to_transform();
		floral::mat4x4f mB1Left = floral::construct_quaternion_euler(0.0f, -47.0f, 0.0f).to_transform()
			* floral::construct_translation3d(0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();
		floral::mat4x4f mB1Right = floral::construct_quaternion_euler(0.0f, -47.0f, 0.0f).to_transform()
			* floral::construct_translation3d(-0.35f, -0.3f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, 90.0f).to_transform();
		GenTesselated3DPlane_Tris_PNC(
				mB1Top,
				0.7f, 0.7f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Front,
				1.4f, 0.7f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Back,
				1.4f, 0.7f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Left,
				1.4f, 0.7f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
		GenTesselated3DPlane_Tris_PNC(
				mB1Right,
				1.4f, 0.7f, subdivision, floral::vec4f(1.0f),
				m_Vertices, m_Indices);
	}

	PerformRaycastTest();

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		insigne::update_vb(newVB, &m_Vertices[0], m_Vertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		insigne::update_ib(newIB, &m_Indices[0], m_Indices.get_size(), 0);
		m_IB = newIB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/cornel_box_vs");
		desc.fs_path = floral::path("/scene/cornel_box_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
	}

	m_DebugDrawer.Initialize();
}

void InterreflectPRT::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);

	m_DebugDrawer.BeginFrame();

	// ground grid cover [-2.0..2.0]
	floral::vec4f gridColor(0.3f, 0.4f, 0.5f, 1.0f);
	for (s32 i = -2; i < 3; i++)
	{
		for (s32 j = -2; j < 3; j++)
		{
			floral::vec3f startX(1.0f * i, 0.0f, 1.0f * j);
			floral::vec3f startZ(1.0f * j, 0.0f, 1.0f * i);
			floral::vec3f endX(1.0f * i, 0.0f, -1.0f * j);
			floral::vec3f endZ(-1.0f * j, 0.0f, 1.0f * i);
			m_DebugDrawer.DrawLine3D(startX, endX, gridColor);
			m_DebugDrawer.DrawLine3D(startZ, endZ, gridColor);
		}
	}

	// coordinate unit vectors
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.5f, 0.0f, 0.1f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.5f, 0.1f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.0f, 0.5f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
	m_DebugDrawer.EndFrame();
}

void InterreflectPRT::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Interreflect PRT");
	ImGui::Text("grace_probe and cornell box");
	ImGui::End();
}

void InterreflectPRT::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);

	m_DebugDrawer.Render(m_SceneData.WVP);
	IDebugUI::OnFrameRender(i_deltaMs);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void InterreflectPRT::OnCleanUp()
{
}

//----------------------------------------------
void InterreflectPRT::PerformRaycastTest()
{
	s32 sqrtNSamples = 10;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = m_MemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	for (u32 i = 0; i < m_Vertices.get_size(); i++)
	{
		u32 rayHitGeometry = 0;
		u32 rayUpper = 0;
		for (s32 j = 0; j < NSamples; j++)
		{
			const sh_sample& sample = samples[j];

			highp_vec3_t normal = highp_vec3_t(m_Vertices[i].Normal.x, m_Vertices[i].Normal.y, m_Vertices[i].Normal.z);
			f64 cosineTerm = floral::dot(normal, sample.vec);
			if (cosineTerm > 0.0f) // upper hemisphere
			{
				rayUpper++;
				floral::ray3df ray;
				ray.o = m_Vertices[i].Position;
				ray.d = floral::vec3f((f32)sample.vec.x, (f32)sample.vec.y, (f32)sample.vec.z);

				for (u32 idx = 0; idx < m_Indices.get_size(); idx += 3)
				//u32 idx = 234;
				{
					u32 tIdx0 = m_Indices[idx];
					u32 tIdx1 = m_Indices[idx + 1];
					u32 tIdx2 = m_Indices[idx + 2];

					if (tIdx0 == i || tIdx1 == i || tIdx2 == i) continue;

					float t = 0.0f;
					const bool isHit = ray_triangle_intersect(ray,
							m_Vertices[tIdx0].Position, m_Vertices[tIdx1].Position, m_Vertices[tIdx2].Position,
							&t);

					if (isHit && t > -0.001f)
					{
						
						rayHitGeometry++;
						break;
					}
				}
			}
		}

		CLOVER_DEBUG("%d: %d - %d", i, rayHitGeometry, rayUpper);
		f32 color = 1.0f - (f32)rayHitGeometry / (f32)rayUpper;
		m_Vertices[i].Color = floral::vec4f(color, color, color, 1.0f);
	}
}

}
