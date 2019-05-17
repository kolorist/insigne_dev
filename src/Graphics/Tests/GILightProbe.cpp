#include "GILightProbe.h"

#include <floral/io/nativeio.h>

#include <clover/Logger.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

namespace stone
{

#define TO_IDX(x, y, z)							(25 * y + 5 * x + z)

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
out highp vec3 v_PosW;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	v_PosW = l_Position_L;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_LightGrid
{
	highp vec4 iu_AABBMinCorner;
	highp vec4 iu_AABBMaxCorner;
	uint iu_SubDivision;
};

in mediump vec4 v_VertexColor;
in highp vec3 v_PosW;

void main()
{
	const mediump vec3 k_StartColor = vec3(0.3f, 0.4f, 0.5f);
	const mediump vec3 k_EndColor = vec3(0.8f, 0.9f, 1.0f);

	highp float dlx = (iu_AABBMaxCorner.x - iu_AABBMinCorner.x) / float(iu_SubDivision);
	highp float dly = (iu_AABBMaxCorner.y - iu_AABBMinCorner.y) / float(iu_SubDivision);
	highp float dlz = (iu_AABBMaxCorner.z - iu_AABBMinCorner.z) / float(iu_SubDivision);
	uint sdx = uint(floor((v_PosW.x - iu_AABBMinCorner.x) / dlx));
	uint sdy = uint(floor((v_PosW.y - iu_AABBMinCorner.y) / dly));
	uint sdz = uint(floor((v_PosW.z - iu_AABBMinCorner.z) / dlz));

	mediump vec3 outColor = mix(k_StartColor, k_EndColor, float(sdx + sdy + sdz) / float(iu_SubDivision * 3u));
	o_Color = vec4(outColor, 1.0f);
	//o_Color = v_VertexColor;
}
)";

GILightProbe::GILightProbe()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(5.0f, 2.5f, 2.0f), floral::vec3f(-5.0f, -2.5f, -2.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
}

GILightProbe::~GILightProbe()
{
}

void GILightProbe::OnInitialize()
{
	m_Vertices.init(2048u, &g_StreammingAllocator);
	m_Indices.init(8192u, &g_StreammingAllocator);
	m_Patches.init(1024u, &g_StreammingAllocator);
	m_SHPositions.init(1024u, &g_StreammingAllocator);

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

	floral::vec4f minCorner(9999.9f, 9999.9f, 9999.9f, 0.0f);
	floral::vec4f maxCorner(-9999.9f, -9999.9f, -9999.9f, 0.0f);
	{
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
		minCorner -= floral::vec4f(0.1f, 0.1f, 0.1f, 0.0f);
		maxCorner += floral::vec4f(0.1f, 0.1f, 0.1f, 0.0f);

		m_LightGridData.AABBMinCorner = minCorner;
		m_LightGridData.AABBMaxCorner = maxCorner;
		m_LightGridData.SubDivision = 4;

		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		insigne::update_ub(newUB, &m_LightGridData, sizeof(LightGridData), 0);
		m_LightUB = newUB;
	}

	// read sh data
	{
		memset(&m_ProbesData, 0, sizeof(ProbesData));

		floral::file_info f = floral::open_file("gfx/sh/shdata.bin");
		floral::file_stream fs;
		fs.buffer = (p8)m_MemoryArena->allocate(f.file_size);
		floral::read_all_file(f, fs);
		u32 numSHProbes = 0;
		fs.read(&numSHProbes);

		f32 sdx = (maxCorner.x - minCorner.x) / 4;
		f32 sdy = (maxCorner.y - minCorner.y) / 4;
		f32 sdz = (maxCorner.z - minCorner.z) / 4;

		for (u32 i = 0; i < numSHProbes; i++)
		{
			floral::vec3f shPos;
			floral::vec4f coEffs[9];
			fs.read(&shPos);
			s32 lx = (s32)roundf((shPos.x - minCorner.x) / sdx);
			s32 ly = (s32)roundf((shPos.y - minCorner.y) / sdy);
			s32 lz = (s32)roundf((shPos.z - minCorner.z) / sdz);
			m_SHPositions.push_back(shPos);
			CLOVER_DEBUG("Read probe at: %d - %d - %d", lx, ly, lz);
			for (u32 j = 0; j < 9; j++)
			{
				fs.read(&coEffs[j]);
				s32 idx = TO_IDX(lx, ly, lz);
				m_ProbesData.Probes[idx * 9 + j] = coEffs[j];
			}
		}

		FLORAL_ASSERT(fs.is_eos());
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_LightGrid", insigne::param_data_type_e::param_ub));

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
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_LightGrid");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_LightUB };
		}
	}

	m_DebugDrawer.Initialize();
}

void GILightProbe::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);
}

void GILightProbe::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::SetNextWindowSize(ImVec2(400, 800));
	ImGui::Begin("GILightProbe Controller");
	if (ImGui::Button("Test Clover Log"))
	{
		CLOVER_DEBUG("test test test");
	}
	ImGui::End();
}

void GILightProbe::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GILightProbe::OnCleanUp()
{
}

}
