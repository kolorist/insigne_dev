#include "OctreePartition.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"
#include "Graphics/CBObjLoader.h"
#include "Graphics/shapegen.h"

namespace stone {

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

OctreePartition::OctreePartition()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

OctreePartition::~OctreePartition()
{
}

void OctreePartition::OnInitialize()
{
	m_Vertices.init(2048u, &g_StreammingAllocator);
	m_Indices.init(8192u, &g_StreammingAllocator);
	m_Patches.init(1024u, &g_StreammingAllocator);

	{
		floral::mat4x4f mBottom = floral::construct_translation3d(0.0f, -1.0f, 0.0f);
		floral::mat4x4f mLeft = floral::construct_translation3d(0.0f, 0.0f, 1.5f)
			* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mRight = floral::construct_translation3d(0.0f, 0.0f, -1.5f)
			* floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mBack = floral::construct_translation3d(-1.0f, 0.0f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();

		GenQuadTesselated3DPlane_Tris_PNC(
				mBottom,
				2.0f, 3.0f, 0.3f, floral::vec4f(0.3f, 0.0f, 0.3f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mLeft,
				2.0f, 2.0f, 0.3f, floral::vec4f(0.3f, 0.3f, 0.0f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mRight,
				2.0f, 2.0f, 0.3f, floral::vec4f(0.3f, 0.2f, 0.1f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mBack,
				2.0f, 3.0f, 0.3f, floral::vec4f(0.0f, 0.3f, 0.3f, 1.0f),
				m_Vertices, m_Indices, m_Patches);

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
		GenQuadTesselated3DPlane_Tris_PNC(
				mB1Top,
				0.7f, 0.7f, 0.3f, floral::vec4f(0.3f, 0.4f, 0.5f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mB1Front,
				1.4f, 0.7f, 0.3f, floral::vec4f(0.4f, 0.3f, 0.5f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mB1Back,
				1.4f, 0.7f, 0.3f, floral::vec4f(0.3f, 0.5f, 0.4f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mB1Left,
				1.4f, 0.7f, 0.3f, floral::vec4f(0.5f, 0.4f, 0.3f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
		GenQuadTesselated3DPlane_Tris_PNC(
				mB1Right,
				1.4f, 0.7f, 0.3f, floral::vec4f(0.3f, 0.3f, 0.3f, 1.0f),
				m_Vertices, m_Indices, m_Patches);
	}

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
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_CamView.position = floral::vec3f(5.0f, 2.5f, 2.0f);
		m_CamView.look_at = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		m_SceneData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);
		m_SceneData.XForm = floral::mat4x4f(1.0f);

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
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

	DoScenePartition();
}

void OctreePartition::DoScenePartition()
{
	m_Octants.init(8192u, &g_StreammingAllocator);
	// find scene AABB
	{
		floral::vec3f minCorner(9999.9f, 9999.9f, 9999.9f);
		floral::vec3f maxCorner(-9999.9f, -9999.9f, -9999.9f);

		for (u32 i = 0; i < m_Vertices.get_size(); i++)
		{
			floral::vec3f v = m_Vertices[i].Position;
			if (v.x < minCorner.x) minCorner.x = v.x;
			if (v.y < minCorner.y) minCorner.y = v.y;
			if (v.z < minCorner.z) minCorner.z = v.z;
			if (v.x > maxCorner.x) maxCorner.x = v.x;
			if (v.y > maxCorner.y) maxCorner.y = v.y;
			if (v.z > maxCorner.z) maxCorner.z = v.z;
		}
		m_SceneAABB.min_corner = minCorner - floral::vec3f(0.1f);
		m_SceneAABB.max_corner = maxCorner + floral::vec3f(0.1f);
	}
	Partition(m_SceneAABB);
}

const bool OctreePartition::CanStopPartition(floral::aabb3f& i_rootOctant)
{
	if (IsOctantTooSmall(i_rootOctant))
		return true;

	if (IsOctantHasTriangles(i_rootOctant, 0))
		return false;
	return true;
}

const bool OctreePartition::IsOctantTooSmall(floral::aabb3f& i_rootOctant)
{
	if (i_rootOctant.max_corner.x - i_rootOctant.min_corner.x < 0.3f)
		return true;
	return false;
}

const bool OctreePartition::IsOctantHasTriangles(floral::aabb3f& i_rootOctant, const u32 i_threshold)
{
	u32 triangleInside = 0;
	for (u32 i = 0; i < m_Indices.get_size(); i += 3)
	{
		floral::vec3f v0 = m_Vertices[m_Indices[i]].Position;
		floral::vec3f v1 = m_Vertices[m_Indices[i + 1]].Position;
		floral::vec3f v2 = m_Vertices[m_Indices[i + 2]].Position;

		if (triangle_aabb_intersect(v0, v1, v2, i_rootOctant))
		{
			triangleInside++;
		}

		if (triangleInside > i_threshold)
		{
			return true;
		}
	}
	return false;
}

void OctreePartition::Partition(floral::aabb3f& i_rootOctant)
{
	if (CanStopPartition(i_rootOctant))
		return;

	floral::vec3f v[8];
	v[0] = i_rootOctant.min_corner;
	v[6] = i_rootOctant.max_corner;
	v[1] = floral::vec3f(v[0].x, v[0].y, v[6].z);
	v[2] = floral::vec3f(v[6].x, v[0].y, v[6].z);
	v[3] = floral::vec3f(v[6].x, v[0].y, v[0].z);

	v[4] = floral::vec3f(v[0].x, v[6].y, v[0].z);
	v[5] = floral::vec3f(v[0].x, v[6].y, v[6].z);
	v[7] = floral::vec3f(v[6].x, v[6].y, v[0].z);

	floral::aabb3f octants[8];

	octants[0].min_corner = v[0];
	octants[0].max_corner = (v[0] + v[6]) / 2.0f;

	octants[1].min_corner = (v[0] + v[1]) / 2.0f;
	octants[1].max_corner = (v[1] + v[6]) / 2.0f;

	octants[2].min_corner = (v[0] + v[3]) / 2.0f;
	octants[2].max_corner = (v[3] + v[6]) / 2.0f;

	octants[3].min_corner = (v[0] + v[2]) / 2.0f;
	octants[3].max_corner = (v[2] + v[6]) / 2.0f;

	octants[4].min_corner = (v[0] + v[4]) / 2.0f;
	octants[4].max_corner = (v[4] + v[6]) / 2.0f;

	octants[5].min_corner = (v[0] + v[5]) / 2.0f;
	octants[5].max_corner = (v[5] + v[6]) / 2.0f;

	octants[6].min_corner = (v[0] + v[6]) / 2.0f;
	octants[6].max_corner = v[6];

	octants[7].min_corner = (v[0] + v[7]) / 2.0f;
	octants[7].max_corner = (v[6] + v[7]) / 2.0f;

	for (u32 i = 0; i < 8; i++)
	{
		if (IsOctantTooSmall(octants[i]))
		{
			const bool hasTri = IsOctantHasTriangles(octants[i], 0);
			if (hasTri)
				m_Octants.push_back({octants[i], true, true});
		}
		Partition(octants[i]);
	}
}

void OctreePartition::OnUpdate(const f32 i_deltaMs)
{
	m_DebugDrawer.BeginFrame();
	for (u32 i = 0; i < m_Octants.get_size(); i++)
	{
		m_DebugDrawer.DrawAABB3D(m_Octants[i].Geometry, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	}
	m_DebugDrawer.EndFrame();
}

void OctreePartition::OnDebugUIUpdate(const f32 i_deltaMs)
{
}

void OctreePartition::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePC>(m_VB, m_IB, m_Material);
	m_DebugDrawer.Render(m_SceneData.WVP);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void OctreePartition::OnCleanUp()
{
}

}
