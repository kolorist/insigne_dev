#include "LightProbePlacement.h"

#include <floral/io/nativeio.h>

#include <clover/Logger.h>

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

// ---------------------------------------------

static const_cstr s_ProbeVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm_unused;
	highp mat4 iu_WVP;
};

layout(std140) uniform ub_ProbeData
{
	highp mat4 iu_XForm;
	mediump vec4 iu_Coeffs[9];
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_ProbeFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_ProbeData
{
	highp mat4 iu_XForm;
	mediump vec4 iu_Coeffs[9];
};

in mediump vec3 v_Normal;

const mediump float c0 = 0.2820947918f;
const mediump float c1 = 0.4886025119f;
const mediump float c2 = 2.185096861f;	// sqrt(15/pi)
const mediump float c3 = 1.261566261f;	// sqrt(5/pi)

mediump vec3 evalSH(in mediump vec3 i_normal)
{
	return
		c0 * iu_Coeffs[0].xyz					// band 0

		- c1 * i_normal.y * iu_Coeffs[1].xyz * 0.667f
		+ c1 * i_normal.z * iu_Coeffs[2].xyz * 0.667f
		- c1 * i_normal.x * iu_Coeffs[3].xyz * 0.667f

		+ c2 * i_normal.x * i_normal.y * 0.5f * iu_Coeffs[4].xyz * 0.25f
		- c2 * i_normal.y * i_normal.z * 0.5f * iu_Coeffs[5].xyz * 0.25f
		+ c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * 0.25f * iu_Coeffs[6].xyz * 0.25f
		- c2 * i_normal.x * i_normal.z * 0.5f * iu_Coeffs[7].xyz * 0.25f
		+ c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * 0.25f * iu_Coeffs[8].xyz * 0.25f;
}

void main()
{
	mediump vec3 c = evalSH(v_Normal);
	o_Color = vec4(c, 1.0f);
}
)";

LightProbePlacement::LightProbePlacement()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(5.0f, 2.5f, 2.0f), floral::vec3f(-5.0f, -2.5f, -2.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_DrawCoordinate(false)
	, m_DrawProbeLocations(false)
	, m_DrawProbeRangeMin(0)
	, m_DrawProbeRangeMax(0)
	, m_DrawOctree(false)
	, m_DrawScene(true)
	, m_DrawSHProbes(false)
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

LightProbePlacement::~LightProbePlacement()
{
}

void LightProbePlacement::OnInitialize()
{
	m_Vertices.init(2048u, &g_StreammingAllocator);
	m_Indices.init(8192u, &g_StreammingAllocator);
	m_Patches.init(1024u, &g_StreammingAllocator);
	m_ProbeLocations.init(2048u, &g_StreammingAllocator);

	m_ProbeVertices.init(1024u, &g_StreammingAllocator);
	m_ProbeIndices.init(4096u, &g_StreammingAllocator);
	m_SHData.init(512u, &g_StreammingAllocator);

	{
		floral::mat4x4f m = floral::construct_scaling3d(floral::vec3f(0.1f));
		GenIcosphere_Tris_P(m, m_ProbeVertices, m_ProbeIndices);
	}

	{
		floral::mat4x4f mBottom = floral::construct_translation3d(0.0f, -1.0f, 0.0f);
		floral::mat4x4f mLeft = floral::construct_translation3d(0.0f, 0.0f, 2.5f)
			* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mRight = floral::construct_translation3d(0.0f, 0.0f, -2.5f)
			* floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();
		floral::mat4x4f mBack = floral::construct_translation3d(-1.0f, 0.0f, 0.0f)
			* floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();

		GenQuadTesselated3DPlane_Tris_PNC(
				mBottom,
				2.0f, 5.0f, 0.3f, floral::vec4f(0.3f, 0.0f, 0.3f, 1.0f),
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
				2.0f, 5.0f, 0.3f, floral::vec4f(0.0f, 0.3f, 0.3f, 1.0f),
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
		floral::mat4x4f mB1Bottom = floral::construct_translation3d(0.0f, -0.999f, 0.0f)
			* floral::construct_quaternion_euler(180.0f, 43.0f, 0.0f).to_transform();
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
		GenQuadTesselated3DPlane_Tris_PNC(
				mB1Bottom,
				0.7f, 0.7f, 0.3f, floral::vec4f(0.3f, 0.4f, 0.5f, 1.0f),
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

	// upload probe geometry
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(VertexP);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		insigne::update_vb(newVB, &m_ProbeVertices[0], m_ProbeVertices.get_size(), 0);
		m_ProbeVB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		insigne::update_ib(newIB, &m_ProbeIndices[0], m_ProbeIndices.get_size(), 0);
		m_ProbeIB = newIB;
	}

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

	// upload probe data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(512);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);
		m_ProbeUB = newUB;
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

	// probe shaders
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_ProbeData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_ProbeFS);
		desc.vs_path = floral::path("/debug/probe_vs");
		desc.fs_path = floral::path("/debug/probe_fs");

		m_ProbeShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ProbeShader, m_ProbeMaterial);

		{
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_Scene");
			m_ProbeMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}

		{
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_ProbeData");
			m_ProbeMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_ProbeUB };
		}
	}

	m_DebugDrawer.Initialize();

	m_ProbeValidator.Initialize();

	DoScenePartition();

	SnapshotAllocatorInfos();
	m_ProbeValidator.Setup(m_SceneData.XForm, m_ProbeLocations);
	m_ProbeValidator.SetupGIMaterial(m_Material);
}

void LightProbePlacement::DoScenePartition()
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
		m_SceneAABB.min_corner = minCorner;
		m_SceneAABB.max_corner = maxCorner;

		floral::aabb3f extrapolateSceneAABB = m_SceneAABB;
		extrapolateSceneAABB.min_corner = minCorner - floral::vec3f(0.1f);
		extrapolateSceneAABB.max_corner = maxCorner + floral::vec3f(0.1f);

		Partition(extrapolateSceneAABB);
	}

	for (u32 i = 0; i < m_Octants.get_size(); i++)
	{
		floral::vec3f v[8];
		v[0] = m_Octants[i].Geometry.min_corner;
		v[6] = m_Octants[i].Geometry.max_corner;
		v[1] = floral::vec3f(v[0].x, v[0].y, v[6].z);
		v[2] = floral::vec3f(v[6].x, v[0].y, v[6].z);
		v[3] = floral::vec3f(v[6].x, v[0].y, v[0].z);

		v[4] = floral::vec3f(v[0].x, v[6].y, v[0].z);
		v[5] = floral::vec3f(v[0].x, v[6].y, v[6].z);
		v[7] = floral::vec3f(v[6].x, v[6].y, v[0].z);

		for (u32 j = 0; j < 8; j++)
		{
			if (floral::point_inside_aabb(v[j], m_SceneAABB))
			{
				if (m_ProbeLocations.find(v[j]) == m_ProbeLocations.get_terminated_index())
					m_ProbeLocations.push_back(v[j]);
			}
		}
	}
}

const bool LightProbePlacement::CanStopPartition(floral::aabb3f& i_rootOctant)
{
	if (IsOctantTooSmall(i_rootOctant))
		return true;

	if (IsOctantHasTriangles(i_rootOctant, 0))
		return false;
	return true;
}

const bool LightProbePlacement::IsOctantTooSmall(floral::aabb3f& i_rootOctant)
{
	if (i_rootOctant.max_corner.x - i_rootOctant.min_corner.x < 0.3f)
		return true;
	return false;
}

const bool LightProbePlacement::IsOctantHasTriangles(floral::aabb3f& i_rootOctant, const u32 i_threshold)
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

void LightProbePlacement::Partition(floral::aabb3f& i_rootOctant)
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

void LightProbePlacement::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_DebugDrawer.BeginFrame();

	if (m_DrawCoordinate)
	{
		m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(1.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 1.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
		m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
	}

	if (m_DrawOctree)
	{
		for (u32 i = 0; i < m_Octants.get_size(); i++)
		{
			m_DebugDrawer.DrawAABB3D(m_Octants[i].Geometry, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
		}
	}

	if (m_DrawProbeLocations)
	{
		if (m_DrawProbeRangeMax >= m_DrawProbeRangeMin)
		{
			//for (u32 i = 0; i < m_ProbeLocations.get_size(); i++)
			for (s32 i = m_DrawProbeRangeMin; i < m_DrawProbeRangeMax; i++)
			{
				m_DebugDrawer.DrawIcosahedron3D(m_ProbeLocations[i], 
						0.05f, floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f));
			}
		}
	}

	if (m_DrawSHProbes)
	{
	}

	m_DebugDrawer.EndFrame();
}

void LightProbePlacement::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("LightProbePlacement Controller");
	if (ImGui::CollapsingHeader("CameraMotion", ImGuiTreeNodeFlags_DefaultOpen))
	{
		floral::vec2f cp = m_CameraMotion.GetCursorPos();
		floral::vec3f pos = m_CameraMotion.GetPosition();

		ImGui::Text("Position: (%4.2f; %4.2f; %4.2f)", pos.x, pos.y, pos.z);
		ImGui::Text("Cursor position: (%4.2f; %4.2f)", cp.x, cp.y);
	}
	if (ImGui::CollapsingHeader("DebugDraw", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Draw coordinates", &m_DrawCoordinate);
		ImGui::Checkbox("Draw probes' locations", &m_DrawProbeLocations);
		ImGui::SliderInt("Probe's draw range FROM", &m_DrawProbeRangeMin, 0, m_ProbeLocations.get_size());
		ImGui::SliderInt("Probe's draw range TO", &m_DrawProbeRangeMax, 0, m_ProbeLocations.get_size());
		ImGui::Checkbox("Draw octree", &m_DrawOctree);
		ImGui::Checkbox("Draw scene", &m_DrawScene);

		if (ImGui::Button("Place probes"))
		{
			m_ProbePlacement = true;
		}

		if (ImGui::Button("Bake SHs"))
		{
			m_SHBaking = true;
		}

		if (ImGui::Button("Draw SH probes"))
		{
			if (m_ProbeValidator.IsSHBakingFinished())
			{
				// upload SH data
				g_TemporalLinearArena.free_all();
				FLORAL_ASSERT(sizeof(SHProbeData) <= 256);
				FLORAL_ASSERT(256 * m_SHData.get_size() <= SIZE_KB(512));
				p8 cpuData = (p8)g_TemporalLinearArena.allocate(SIZE_KB(512));
				memset(cpuData, 0, SIZE_KB(512));

				for (u32 i = 0; i < m_SHData.get_size(); i++) {
					p8 pData = cpuData + 256 * i;
					memcpy(pData, &m_SHData[i], sizeof(SHProbeData));
				}

				insigne::copy_update_ub(m_ProbeUB, cpuData, 256 * m_SHData.get_size(), 0);
				m_DrawSHProbes = true;
			}
		}

		if (ImGui::Button("Export SH data"))
		{
			if (m_ProbeValidator.IsSHBakingFinished())
			{
				floral::file_info f = floral::open_output_file("shdata.bin");
				floral::output_file_stream os;
				floral::map_output_file(f, os);

				u32 numSHProbes = m_ProbeLocations.get_size();
				os.write(numSHProbes);

				for (u32 i = 0; i < numSHProbes; i++)
				{
					os.write(m_ProbeLocations[i]);
					for (u32 j = 0; j < 9; j++)
					{
						os.write(m_SHData[i].CoEffs[j]);
					}
				}

				floral::close_file(f);
				CLOVER_DEBUG("SH data exported");
			}
		}
	}
	ImGui::End();
}

void LightProbePlacement::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	if (m_ProbePlacement)
	{
		if (!m_ProbeValidator.IsValidationFinished())
		{
			floral::simple_callback<void, const insigne::material_desc_t&> renderCb;
			renderCb.bind<LightProbePlacement, &LightProbePlacement::RenderCallback>(this);

			m_ProbeValidator.Validate(renderCb);
		}

		if (m_ProbeValidator.IsValidationFinished())
		{
			static bool updated = false;
			if (!updated)
			{
				m_ProbeLocations = m_ProbeValidator.GetValidatedLocations();
				updated = true;
			}
		}
	}

	if (m_SHBaking)
	{
		if (!m_ProbeValidator.IsSHBakingFinished())
		{
			floral::simple_callback<void, const insigne::material_desc_t&> renderCb;
			renderCb.bind<LightProbePlacement, &LightProbePlacement::RenderCallback>(this);

			m_ProbeValidator.BakeSH(renderCb);
		}

		if (m_ProbeValidator.IsSHBakingFinished())
		{
			static bool updated = false;
			if (!updated)
			{
				const floral::fixed_array<floral::vec3f, LinearAllocator>& shPos = m_ProbeValidator.GetValidatedLocations();
				const floral::fixed_array<SHData, LinearAllocator>& shData = m_ProbeValidator.GetValidatedSHData();

				for (u32 i = 0; i < shPos.get_size(); i++) {
					SHProbeData d;
					d.XForm = floral::construct_translation3d(shPos[i]) * floral::construct_scaling3d(floral::vec3f(0.8f));
					for (u32 j = 0; j < 9; j++)
					{
						d.CoEffs[j] = shData[i].CoEffs[j];
					}
					m_SHData.push_back(d);
				}

				updated = true;
			}
		}
	}

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	if (m_DrawScene)
	{
		insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);
	}

	if (m_DrawSHProbes)
	{
		for (u32 i = 0; i < m_SHData.get_size(); i++) {
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_ProbeData");
			m_ProbeMaterial.uniform_blocks[ubSlot].value.offset = 256 * i;
			insigne::draw_surface<SurfaceP>(m_ProbeVB, m_ProbeIB, m_ProbeMaterial);
		}
	}

	m_DebugDrawer.Render(m_SceneData.WVP);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void LightProbePlacement::RenderCallback(const insigne::material_desc_t& i_material)
{
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, i_material);
}

void LightProbePlacement::OnCleanUp()
{
}

}
