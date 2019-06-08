#include "ShapeGen.h"

#include <clover/Logger.h>

#include <floral/comgeo/shapegen.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
//layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out highp vec3 v_Normal_W;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	highp vec4 norm_W = iu_XForm * vec4(l_Normal_L, 0.0f);
	gl_Position = iu_WVP * pos_W;
	v_Normal_W = norm_W.xyz;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in highp vec3 v_Normal_W;

void main()
{
	o_Color = vec4(v_Normal_W, 1.0f);
}
)";


ShapeGen::ShapeGen()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(3.0f, 3.0f, 3.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
}

ShapeGen::~ShapeGen()
{
}

void ShapeGen::OnInitialize()
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

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(128);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}

	// generate geometry and upload
	{
		g_TemporalLinearArena.free_all();
		floral::fixed_array<VertexPNC, LinearArena> vertices(2048u, &g_TemporalLinearArena);
		floral::fixed_array<s32, LinearArena> indices(4096u, &g_TemporalLinearArena);
		floral::fixed_array<VertexPNC, LinearArena> mnfVertices(2048u, &g_TemporalLinearArena);
		floral::fixed_array<s32, LinearArena> mnfIndices(4096u, &g_TemporalLinearArena);

		vertices.resize_ex(2048u);
		indices.resize_ex(4096u);
		mnfVertices.resize_ex(2048u);
		mnfIndices.resize_ex(4096u);

		u32 vtxCount = 0, idxCount = 0;
		u32 mnfVtxCount = 0, mnfIdxCount = 0;

		floral::push_generation_transform(floral::construct_translation3d(1.0f, 0.0f, 0.0f));
		floral::push_generation_transform(floral::construct_scaling3d(0.5f, 0.5f, 0.5f));
		floral::quaternionf q = floral::construct_quaternion_euler(45.0f, 0.0f, 0.0f);
		floral::push_generation_transform(q.to_transform());

		{
			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_unit_plane_3d(
					0, sizeof(VertexPNC),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
					&vertices[0], &indices[0],

					0, sizeof(VertexPNC), 0.05f,
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
					&mnfVertices[0], &mnfIndices[0]);

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;
		}

		floral::reset_generation_transforms_stack();

		floral::push_generation_transform(floral::construct_translation3d(0.0f, 0.0f, 1.0f));
		floral::push_generation_transform(floral::construct_scaling3d(0.3f, 0.3f, 0.3f));

		{
			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_unit_plane_3d(
					vtxCount, sizeof(VertexPNC),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
					&vertices[vtxCount], &indices[idxCount],

					mnfVtxCount, sizeof(VertexPNC), 0.05f,
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
					&mnfVertices[mnfVtxCount], &mnfIndices[mnfIdxCount]);

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;
		}

		floral::reset_generation_transforms_stack();

		{
			floral::manifold_geo_generate_result_t genResult = floral::generate_manifold_quadtes_unit_plane_3d(
					vtxCount, sizeof(VertexPNC),
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal, 0.1f,
					&vertices[vtxCount], &indices[idxCount],

					mnfVtxCount, sizeof(VertexPNC), 0.05f,
					floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
					&mnfVertices[mnfVtxCount], &mnfIndices[mnfIdxCount]);

			vtxCount += genResult.vertices_generated;
			idxCount += genResult.indices_generated;
			mnfVtxCount += genResult.manifold_vertices_generated;
			mnfIdxCount += genResult.manifold_indices_generated;
		}

		vertices.resize_ex(vtxCount);
		indices.resize_ex(idxCount);
		mnfVertices.resize_ex(mnfVtxCount);
		mnfIndices.resize_ex(mnfIdxCount);

		//insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(VertexPNC), 0);
		//insigne::copy_update_ib(m_IB, &indices[0], indices.get_size(), 0);
		insigne::copy_update_vb(m_VB, &mnfVertices[0], mnfVertices.get_size(), sizeof(VertexPNC), 0);
		insigne::copy_update_ib(m_IB, &mnfIndices[0], mnfIndices.get_size(), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/sample_vs");
		desc.fs_path = floral::path("/scene/sample_fs");

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

void ShapeGen::OnUpdate(const f32 i_deltaMs)
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

void ShapeGen::OnDebugUIUpdate(const f32 i_deltaMs)
{
}

void ShapeGen::OnRender(const f32 i_deltaMs)
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

void ShapeGen::OnCleanUp()
{
}

}
