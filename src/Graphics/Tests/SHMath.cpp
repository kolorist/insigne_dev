#include "SHMath.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

namespace stone {

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
};

out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	//v_Normal = normalize(iu_XForm * vec4(l_Position_L, 0.0f)).xyz;
	v_Normal = normalize(l_Position_L);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_SHData
{
	mediump vec4 iu_Coeffs[9];
};

in mediump vec3 v_Normal;

const mediump float c1 = 0.429043f;
const mediump float c2 = 0.511664f;
const mediump float c3 = 0.743125f;
const mediump float c4 = 0.886227f;
const mediump float c5 = 0.247708f;

mediump vec3 evalSH(in mediump vec3 i_normal)
{
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
		c3 * iu_Coeffs[6].xyz * i_normal.z * i_normal.z - c5 * iu_Coeffs[6].xyz +
		2.0f * c1 * iu_Coeffs[7].xyz * i_normal.x * i_normal.z +
		c1 * iu_Coeffs[8].xyz * (i_normal.x * i_normal.x - i_normal.y * i_normal.y);
}

void main()
{
	mediump vec3 c = evalSH(v_Normal);
	o_Color = vec4(c, 1.0f);
}
)";

SHMath::SHMath()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

SHMath::~SHMath()
{
}

void SHMath::OnInitialize()
{
	m_Vertices.init(256u, &g_StreammingAllocator);
	m_Indices.init(512u, &g_StreammingAllocator);
	m_SHData.init(27u, &g_StreammingAllocator);

	{
		{
			floral::mat4x4f m = floral::construct_scaling3d(0.15f, 0.15f, 0.15f);
			GenIcosphere_Tris_PosColor(floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}
	}

	// sh data
	{
		floral::file_info shFile = floral::open_file("27probes.cbsh");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(shFile.file_size);
		floral::read_all_file(shFile, dataStream);
		floral::close_file(shFile);

		for (u32 i = 0; i < 27; i++) {
			SHData shData;
			for (u32 j = 0; j < 9; j++) {
				floral::vec3f shBand;
				dataStream.read_bytes((p8)&shBand, sizeof(floral::vec3f));
				shData.CoEffs[j] = floral::vec4f(shBand.x, shBand.y, shBand.z, 0.0f);
			}
			m_SHData.push_back(shData);
		}

		m_MemoryArena->free_all();
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DemoVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		insigne::update_vb(newVB, &m_Vertices[0], m_Vertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
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
		m_CamView.position = floral::vec3f(7.0f, 0.5f, 0.0f);
		m_CamView.look_at = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		m_SceneData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);
		m_SceneData.XForm = floral::mat4x4f(1.0f);
		m_DebugWVP = m_SceneData.WVP;

		insigne::update_ub(newUB, &m_SceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		SHData shData;
		shData.CoEffs[0] = floral::vec4f(10.261054f, 14.317350f, 10.261044f, 0.0f);
		shData.CoEffs[1] = floral::vec4f(-6.633853f, -5.090485f, -6.633842f, 0.0f);
		shData.CoEffs[2] = floral::vec4f(13.300049f, 19.393381f, 13.300061f, 0.0f);
		shData.CoEffs[3] = floral::vec4f(1.463530f, -0.079820f, 1.463536f, 0.0f);
		shData.CoEffs[4] = floral::vec4f(0.234119f, -0.444766f, 0.234110f, 0.0f);
		shData.CoEffs[5] = floral::vec4f(-10.240585f, -7.494582f, -10.240602f, 0.0f);
		shData.CoEffs[6] = floral::vec4f(9.065324f, 14.902674f, 9.065321f, 0.0f);
		shData.CoEffs[7] = floral::vec4f(2.581493f, -0.164537f, 2.581484f, 0.0f);
		shData.CoEffs[8] = floral::vec4f(-2.101417f, -2.101410f, -2.101410f, 0.0f);

		insigne::copy_update_ub(newUB, &shData, sizeof(SHData), 0);
		m_SHUB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_SHData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/sh_probe");
		desc.fs_path = floral::path("/internal/sh_probe");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = m_UB;
		}

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_SHData");
			m_Material.uniform_blocks[ubSlot].value = m_SHUB;
		}
	}
	m_DebugDrawer.Initialize();
}

void SHMath::OnUpdate(const f32 i_deltaMs)
{
	m_DebugDrawer.BeginFrame();
	floral::vec3f s = m_CameraMotion.GetStartCoord();
	floral::vec3f e = m_CameraMotion.GetEndCoord();
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), s, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), e, floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	floral::vec4f p(0.0f, 0.0f, 1.0f, 1.0f);
	floral::mat4x4f r = m_CameraMotion.GetRotation().normalize().to_transform();
	p = r * p;
	floral::vec3f pp(p.x, p.y, p.z);
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), pp, floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, floral::length(pp), 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.EndFrame();
}

void SHMath::OnRender(const f32 i_deltaMs)
{
	static f32 elapsedTime = 0.0f;
	elapsedTime += i_deltaMs;

	floral::vec4f camPos(7.0f, 0.5f, 0.0f, 1.0f);
	camPos = m_CameraMotion.GetRotation().normalize().to_transform() * camPos;
	m_CamView.position = floral::vec3f(camPos.x, camPos.y, camPos.z);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	{
		const u32 steps = 2;
		const f32 dx = 2.0f;
		const f32 dy = 2.0f;
		const f32 dz = 2.0f;
		const f32 disp = 0.2f;
		const f32 stepDist = (dx - disp * 2.0f) / (f32)steps;
		const floral::vec3f minCorner(-1.0f);

		u32 shidx = 0;

		for (u32 i = 0; i <= steps; i++) {
			for (u32 j = 0; j <= steps; j++) {
				for (u32 k = 0; k <= steps; k++) {
					floral::vec3f pos(
							minCorner.x + disp + i * stepDist,
							minCorner.y + disp + j * stepDist,
							minCorner.z + disp + k * stepDist);
					m_SceneData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);
					m_SceneData.XForm = floral::construct_translation3d(pos);
					insigne::copy_update_ub(m_SHUB, &m_SHData[shidx], sizeof(SHData), 0);
					shidx++;
					insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);
					insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
					insigne::dispatch_render_pass();
				}
			}
		}
	}
	m_DebugDrawer.Render(m_DebugWVP);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHMath::OnCleanUp()
{
}

}
