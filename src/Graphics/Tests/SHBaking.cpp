#include "SHBaking.h"

#include <clover.h>
#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Graphics/stb_image_write.h"

#include "Graphics/GeometryBuilder.h"
#include "SHBakingUtils.h"

namespace stone {

static const_cstr s_GeometryVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in highp vec3 l_Normal_L;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_WVP;
};

out mediump vec4 v_Color;
out mediump vec3 v_Normal;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color;
	v_Normal = l_Normal_L;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_GeometryFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;
in mediump vec3 v_Normal;

void main()
{
	o_Color = v_Color;
	//o_Color = vec4(v_Normal, 1.0f);
}
)";

// ---------------------------------------------
static const_cstr s_ProbeVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
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

// ---------------------------------------------

SHBaking::SHBaking()
{
}

SHBaking::~SHBaking()
{
}

void SHBaking::OnInitialize()
{
	m_GeoVertices.init(512u, &g_StreammingAllocator);
	m_GeoIndices.init(1024u, &g_StreammingAllocator);

	m_ProbeVertices.init(1024u, &g_StreammingAllocator);
	m_ProbeIndices.init(4096u, &g_StreammingAllocator);

	m_SHPositions.init(256u, &g_StreammingAllocator);
	m_SHData.init(256u, &g_StreammingAllocator);
	m_EnvSceneData.init(256 * 6, &g_StreammingAllocator);

	// debug geometry
	{
		floral::mat4x4f m = floral::construct_scaling3d(floral::vec3f(0.1f));
		GenIcosphere_Tris_P(m, m_ProbeVertices, m_ProbeIndices);
	}

	// scene geometry
	{
		floral::mat4x4f m1(1.0f), m2(1.0f), m3(1.0f);
		m2 = floral::construct_translation3d(0.0f, 0.75f, 1.5f)
			* floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform()
			* floral::construct_scaling3d(1.0f, 1.0f, 0.5f);
		m3 = floral::construct_translation3d(0.0f, 0.75f, -1.5f)
			* floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform()
			* floral::construct_scaling3d(1.0f, 1.0f, 0.5f);

		GenTessellated3DPlane_Tris_PNC(m1, 1.5f, 10, floral::vec4f(0.3f, 0.3f, 0.3f, 1.0f),
				m_GeoVertices, m_GeoIndices);

		// here, extract the locations to render SH environment map
		for (u32 i = 0; i < m_GeoVertices.get_size(); i++) {
			m_SHPositions.push_back(m_GeoVertices[i].Position);
			SHProbeData d;
			d.XForm = floral::construct_translation3d(m_SHPositions[i] + floral::vec3f(0.0f, 0.2f, 0.0f));
			m_SHData.push_back(d);
		}

		GenTessellated3DPlane_Tris_PNC(m2, 1.5f, 3, floral::vec4f(1.5f, 0.0f, 0.0f, 1.0f),
				m_GeoVertices, m_GeoIndices);

		GenTessellated3DPlane_Tris_PNC(m3, 1.5f, 3, floral::vec4f(0.0f, 1.5f, 0.0f, 1.0f),
				m_GeoVertices, m_GeoIndices);
	}

	// sh wvps
	{
		static floral::vec3f faceUpDirs[] = {
			floral::vec3f(0.0f, 1.0f, 0.0f),	// positive X
			floral::vec3f(0.0f, 1.0f, 0.0f),	// negative X

			floral::vec3f(1.0f, 0.0f, 0.0f),	// positive Y
			floral::vec3f(0.0f, 0.0f, -1.0f),	// negative Y

			floral::vec3f(0.0f, 1.0f, 0.0f),	// positive Z
			floral::vec3f(0.0f, 1.0f, 0.0f),	// negative Z
		};

		static floral::vec3f faceLookAtDirs[] = {
			floral::vec3f(1.0f, 0.0f, 0.0f),	// positive X
			floral::vec3f(-1.0f, 0.0f, 0.0f),	// negative X

			floral::vec3f(0.0f, 1.0f, 0.0f),	// positive Y
			floral::vec3f(0.0f, -1.0f, 0.0f),	// negative Y

			floral::vec3f(0.0f, 0.0f, 1.0f),	// positive Z
			floral::vec3f(0.0f, 0.0f, -1.0f),	// negative Z
		};

		floral::camera_persp_t camProj;
		camProj.near_plane = 0.01f; camProj.far_plane = 20.0f;
		camProj.fov = 90.0f;
		camProj.aspect_ratio = 1.0f;
		floral::mat4x4f proj = floral::construct_perspective(camProj);

		for (u32 i = 0; i < m_SHPositions.get_size(); i++) {
			for (u32 f = 0; f < 6; f++) {
				floral::camera_view_t camView;
				camView.position = m_SHPositions[i] + floral::vec3f(0.0f, 0.2f, 0.0f);
				camView.up_direction = faceUpDirs[f];
				camView.look_at = faceLookAtDirs[f];
				floral::mat4x4f wvp = proj * floral::construct_lookat_dir(camView);
				m_EnvSceneData.push_back(SceneData{ wvp });
			}
		}
	}

	// envmap render buffer
	{
		// 1536 x 256
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));
		desc.width = 1536; desc.height = 256;
		m_EnvMapRenderBuffer = insigne::create_framebuffer(desc);
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

	// upload scene geometry
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(VertexPNC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		insigne::update_vb(newVB, &m_GeoVertices[0], m_GeoVertices.get_size(), 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		insigne::update_ib(newIB, &m_GeoIndices[0], m_GeoIndices.get_size(), 0);
		m_IB = newIB;
	}

	// upload probe data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		g_TemporalLinearArena.free_all();
		FLORAL_ASSERT(sizeof(SHProbeData) <= 256);
		FLORAL_ASSERT(256 * m_SHData.get_size() <= SIZE_KB(64));
		p8 cpuData = (p8)g_TemporalLinearArena.allocate(SIZE_KB(64));
		memset(cpuData, 0, SIZE_KB(64));

		for (u32 i = 0; i < m_SHData.get_size(); i++) {
			p8 pData = cpuData + 256 * i;
			memcpy(pData, &m_SHData[i], sizeof(SHProbeData));
		}

		insigne::copy_update_ub(newUB, cpuData, 256 * m_SHData.get_size(), 0);
		m_ProbeUB = newUB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(256);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		g_TemporalLinearArena.free_all();
		FLORAL_ASSERT(256 * m_EnvSceneData.get_size() <= SIZE_KB(256));
		p8 cpuData = (p8)g_TemporalLinearArena.allocate(SIZE_KB(256));
		memset(cpuData, 0, SIZE_KB(256));

		for (u32 i = 0; i < m_EnvSceneData.get_size(); i++) {
			p8 pData = cpuData + 256 * i;
			memcpy(pData, &m_EnvSceneData[i], sizeof(SceneData));
		}

		insigne::copy_update_ub(newUB, cpuData, 256 * m_EnvSceneData.get_size(), 0);
		m_EnvMapSceneUB = newUB;
	}

	// upload scene data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_CamView.position = floral::vec3f(5.0f, 1.0f, 0.0f);
		m_CamView.look_at = floral::vec3f(0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		SceneData sceneData;
		sceneData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);

		insigne::copy_update_ub(newUB, &sceneData, sizeof(SceneData), 0);
		m_UB = newUB;
	}

	// probe shaders
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_ProbeData", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_ProbeFS);

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

	// scene shaders
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_GeometryVS);
		strcpy(desc.fs, s_GeometryFS);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		{
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}
	}

	// env map shaders
	{
		insigne::infuse_material(m_Shader, m_EnvMapMaterial);
		{
			u32 ubSlot = insigne::get_material_uniform_block_slot(m_EnvMapMaterial, "ub_Scene");
			m_EnvMapMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_EnvMapSceneUB };
		}
	}
	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void SHBaking::OnUpdate(const f32 i_deltaMs)
{
}

void SHBaking::OnRender(const f32 i_deltaMs)
{
	static bool finishCaptured = false;
	static bool finishUpdateSH = false;
	static bool envCaptured = false;
	static bool envWritten = false;
	static u64 frameIdx = 0;
	static u32 shid = 0;
	if (!finishCaptured) {
		if (!envCaptured) {
			for (u32 f = 0; f < 6; f++) {
				insigne::begin_render_pass(m_EnvMapRenderBuffer, 256 * f, 0, 256, 256);
				u32 ubSlot = insigne::get_material_uniform_block_slot(m_EnvMapMaterial, "ub_Scene");
				m_EnvMapMaterial.uniform_blocks[ubSlot].value.offset = 256 * (f + 6 * shid);
				insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_EnvMapMaterial);
				insigne::end_render_pass(m_EnvMapRenderBuffer);
				insigne::dispatch_render_pass();
			}

			g_TemporalLinearArena.free_all();
			m_EnvMapPixelData = g_TemporalLinearArena.allocate_array<f32>(1536 * 256 * 3);
			frameIdx = insigne::schedule_framebuffer_capture(m_EnvMapRenderBuffer, m_EnvMapPixelData);
			envCaptured = true;
		}

		if (envCaptured && !envWritten && insigne::get_current_frame_idx() >= frameIdx) {
			//stbi_flip_vertically_on_write(1);
			//stbi_write_hdr("out.hdr", 1536, 256, 3, m_EnvMapPixelData);
			f64 shr[9];
			f64 shg[9];
			f64 shb[9];
			ComputeSH(shr, shg, shb, m_EnvMapPixelData);
			envWritten = true;

			for (u32 i = 0; i < 9; i++) {
				m_SHData[shid].CoEffs[i] = floral::vec4f(shr[i], shg[i], shb[i], 0.0f);
			}
			shid++;
			envCaptured = false;
			envWritten = false;
			CLOVER_DEBUG("Capture %d probes", shid);
		}
		if (shid == m_SHPositions.get_size()) finishCaptured = true;
	}

	if (finishCaptured && !finishUpdateSH) {
		p8 cpuData = (p8)g_TemporalLinearArena.allocate(SIZE_KB(64));
		memset(cpuData, 0, SIZE_KB(64));

		for (u32 i = 0; i < m_SHData.get_size(); i++) {
			p8 pData = cpuData + 256 * i;
			memcpy(pData, &m_SHData[i], sizeof(SHProbeData));
		}

		insigne::copy_update_ub(m_ProbeUB, cpuData, 256 * m_SHData.get_size(), 0);
		insigne::dispatch_render_pass();
		finishUpdateSH = true;
	}

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	// render here
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);

	for (u32 i = 0; i < m_SHData.get_size(); i += 2) {
		u32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_ProbeData");
		m_ProbeMaterial.uniform_blocks[ubSlot].value.offset = 256 * i;
		insigne::draw_surface<SurfaceP>(m_ProbeVB, m_ProbeIB, m_ProbeMaterial);
	}
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHBaking::OnCleanUp()
{
}

}
