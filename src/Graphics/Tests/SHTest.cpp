#include "SHTest.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/prt.h"
#include "Graphics/stb_image.h"
#include "Graphics/GeometryBuilder.h"

namespace stone
{

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

mediump vec3 evalSH(in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;			// 1/sqrt(4pi)
	const mediump float c1 = 0.4886025119f;			// sqrt(3/4pi)
	const mediump float c2 = 1.092548431f;			// sqrt(15/4pi)
	const mediump float c3 = 0.3153915653f;			// sqrt(5/16pi)

	const mediump float a0 = 3.141593f;
	const mediump float a1 = 2.094395f;
	const mediump float a2 = 0.785398f;

	return
		a0 * c0 * iu_Coeffs[0].xyz					// band 0

		- a1 * c1 * i_normal.y * iu_Coeffs[1].xyz	// band 1
		+ a1 * c1 * i_normal.z * iu_Coeffs[2].xyz	// band 1
		- a1 * c1 * i_normal.x * iu_Coeffs[3].xyz	// band 1

		+ a2 * c2 * i_normal.x * i_normal.y * iu_Coeffs[4].xyz
		- a2 * c2 * i_normal.y * i_normal.z * iu_Coeffs[5].xyz
		+ a2 * c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * iu_Coeffs[6].xyz
		- a2 * c2 * i_normal.x * i_normal.z * iu_Coeffs[7].xyz
		+ a2 * c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * iu_Coeffs[8].xyz;
}

mediump vec3 evalSH_irr(in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;
	const mediump float c1 = 0.4886025119f;
	const mediump float c2 = 2.185096861f;	// sqrt(15/pi)
	const mediump float c3 = 1.261566261f;	// sqrt(5/pi)
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

mediump vec3 evalSH_radiance(in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;			// 1/sqrt(4pi)
	const mediump float c1 = 0.4886025119f;			// sqrt(3/4pi)
	const mediump float c2 = 1.092548431f;			// sqrt(15/4pi)
	const mediump float c3 = 0.3153915653f;			// sqrt(5/16pi)
	return
		c0 * iu_Coeffs[0].xyz					// band 0

		- c1 * i_normal.y * iu_Coeffs[1].xyz	// band 1
		+ c1 * i_normal.z * iu_Coeffs[2].xyz	// band 1
		- c1 * i_normal.x * iu_Coeffs[3].xyz	// band 1

		+ c2 * i_normal.x * i_normal.y * iu_Coeffs[4].xyz
		- c2 * i_normal.y * i_normal.z * iu_Coeffs[5].xyz
		+ c3 * (-1.0f + 3.0f * i_normal.z * i_normal.z) * iu_Coeffs[6].xyz
		- c2 * i_normal.x * i_normal.z * iu_Coeffs[7].xyz
		+ c2 * (i_normal.x * i_normal.x - i_normal.y * i_normal.y) * iu_Coeffs[8].xyz;
}

void main()
{
	mediump vec3 c = evalSH_irr(v_Normal);
	//c = pow(c, vec3(1.0f / 2.2f));
	o_Color = vec4(c, 1.0f);
}
)";

SHTest::SHTest()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(0.0f, 0.0f, -3.0f), floral::vec3f(0.0f, 0.0f, 3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
}

SHTest::~SHTest()
{
}

void SHTest::OnInitialize()
{
	m_ProbeVertices.init(1024u, &g_StreammingAllocator);
	m_ProbeIndices.init(4096u, &g_StreammingAllocator);

	{
		floral::mat4x4f m = floral::construct_scaling3d(floral::vec3f(0.4f));
		GenIcosphere_Tris_P(m, m_ProbeVertices, m_ProbeIndices);
	}

	float u, v;
	{
		floral::vec3f dir(0.0f, 0.1f, -0.2f);
		dir = floral::normalize(dir);
		map_cartesian_to_mirror_ball_tex_coord(dir.x, dir.y, dir.z, u, v);
	}

	color3* result = nullptr;
#if 0
	{
		int sqrt_n_samples = 100;
		int n_samples = sqrt_n_samples * sqrt_n_samples;
		int n_coeffs = 9;
		sh_sample* samples = new sh_sample[n_samples];
		for (int i = 0; i < n_samples; i++)
		{
			samples[i].coeff = new double[n_coeffs];
			for (int j = 0; j < n_coeffs; j++)
			{
				samples[i].coeff[j] = 0.0;
			}
		}

		sh_setup_spherical_samples(samples, sqrt_n_samples);
		result = new double[n_coeffs];
		for (int i = 0; i < n_coeffs; i++)
		{
			result[i] = 0.0;
		}
		sh_project_polar_function(light_fn, n_samples, n_coeffs, samples, result);
	}
#else
	{
		int channels = 0;
		image_t image;
		image.hdr_pixels = stbi_loadf("grace_probe.hdr", &image.width, &image.height, &channels, 0);

		int sqrt_n_samples = 100;
		int n_samples = sqrt_n_samples * sqrt_n_samples;
		int n_coeffs = 9;
		sh_sample* samples = new sh_sample[n_samples];
		for (int i = 0; i < n_samples; i++)
		{
			samples[i].coeff = new double[n_coeffs];
			for (int j = 0; j < n_coeffs; j++)
			{
				samples[i].coeff[j] = 0.0;
			}
		}

		sh_setup_spherical_samples(samples, sqrt_n_samples);
		result = new color3[n_coeffs];
		for (int i = 0; i < n_coeffs; i++)
		{
			result[i].r = 0.0f;
			result[i].g = 0.0f;
			result[i].b = 0.0f;
		}
		sh_project_light_image(&image, n_samples, n_coeffs, samples, result);
	}
	if (0)
	{
		result[0] = color3 { 0.79f, 0.44f, 0.54f };
		result[1] = color3 { 0.39f, 0.35f, 0.60f };
		result[2] = color3 { -0.34f, -0.18f, -0.27f };
		result[3] = color3 { -0.29f, -0.06f, 0.01f };
		result[4] = color3 { -0.11f, -0.05f, -0.12f };
		result[5] = color3 { -0.26f, -0.22f, -0.47f };
		result[6] = color3 { -0.16f, -0.09f, -0.15f };
		result[7] = color3 { 0.56f, 0.21f, 0.14f };
		result[8] = color3 { 0.21f, -0.05f, -0.30f };
	}
#endif

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

	// upload probe data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_SHData.XForm = floral::mat4x4f(1.0f);
		for (u32 i = 0; i < 9; i++)
		{
			m_SHData.CoEffs[i] = floral::vec4f(result[i].r, result[i].g, result[i].b, 1.0);
		}
		insigne::update_ub(newUB, &m_SHData, sizeof(m_SHData), 0);

		m_ProbeUB = newUB;
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
}

void SHTest::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);

	m_DebugDrawer.BeginFrame();
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(1.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 1.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
	m_DebugDrawer.EndFrame();
}

void SHTest::OnDebugUIUpdate(const f32 i_deltaMs)
{
}

void SHTest::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<SurfaceP>(m_ProbeVB, m_ProbeIB, m_ProbeMaterial);
	m_DebugDrawer.Render(m_SceneData.WVP);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHTest::OnCleanUp()
{
}

}
