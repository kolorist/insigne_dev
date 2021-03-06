#include "OmniShadow.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

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

layout(std140) uniform ub_Light
{
	highp vec4 iu_LightPosition;
	mediump vec4 iu_LightColor;
	mediump vec4 iu_LightRadiance;
};

out mediump vec4 v_VertexColor;
out highp vec3 v_Normal;
out highp vec3 v_LightDirection;

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	v_VertexColor = l_Color;
	v_Normal = l_Normal_L;
	v_LightDirection = iu_LightPosition.xyz - pos_W.xyz;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Light
{
	highp vec4 iu_LightPosition;
	mediump vec4 iu_LightColor;
	mediump vec4 iu_LightRadiance;
};

in mediump vec4 v_VertexColor;
in highp vec3 v_Normal;
in highp vec3 v_LightDirection;

void main()
{
	highp vec3 n = normalize(v_Normal);
	highp vec3 l = normalize(v_LightDirection);

	mediump float f = dot(n, l);

	o_Color = vec4(f);
}
)";

OmniShadow::OmniShadow()
{
}

OmniShadow::~OmniShadow()
{
}

void OmniShadow::OnInitialize()
{
	m_Vertices.init(128u, &g_StreammingAllocator);
	m_Indices.init(256u, &g_StreammingAllocator);

	{
		// bottom
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, -1.0f, 0.0f);
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// top
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 1.0f, 0.0f) *
				floral::construct_quaternion_euler(180.0f, 0.0f, 0.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// right
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 0.0f, -1.0f) *
				floral::construct_quaternion_euler(90.0f, 0.0f, 0.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// left
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 0.0f, 1.0f) *
				floral::construct_quaternion_euler(-90.0f, 0.0f, 0.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// back
		{
			floral::mat4x4f m =
				floral::construct_translation3d(-1.0f, 0.0f, 0.0f) *
				floral::construct_quaternion_euler(0.0f, 0.0f, -90.0f).to_transform();
			Gen3DPlane_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 1.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// small box
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, -0.6f, 0.45f) *
				floral::construct_quaternion_euler(0.0f, 35.0f, 0.0f).to_transform() *
				floral::construct_scaling3d(0.2f, 0.4f, 0.2f);
			GenBox_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// large box
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.2f, -0.4f, -0.4f) *
				floral::construct_quaternion_euler(0.0f, -15.0f, 0.0f).to_transform() *
				floral::construct_scaling3d(0.3f, 0.6f, 0.3f);
			GenBox_Tris_PosNormalColor(floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}

		// point light cube
		{
			floral::mat4x4f m =
				floral::construct_translation3d(0.0f, 0.9f, 0.0f) *
				floral::construct_scaling3d(0.05f, 0.05f, 0.05f);
			GenBox_Tris_PosNormalColor(floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f), m, m_Vertices, m_Indices);
		}
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
		// 3072 * 512
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.width = 512 * 6;
		desc.height = 512;
		m_ShadowRenderBuffer = insigne::create_framebuffer(desc);
	}

	// camera
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_CamView.position = floral::vec3f(5.0f, 0.5f, 0.0f);
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

	// light source
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_LightData.Position = floral::vec4f(0.0f, 0.9f, 0.0f, 1.0f);
		m_LightData.Color = floral::vec4f(1.0f);
		m_LightData.Radiance = floral::vec4f(50.0f);

		insigne::update_ub(newUB, &m_LightData, sizeof(LightData), 0);
		m_LightDataUB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Light", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/cornel_box_vs");
		desc.fs_path = floral::path("/internal/cornel_box_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Scene");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		}

		// light data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Light");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_LightDataUB };
		}
	}
}

void OmniShadow::OnUpdate(const f32 i_deltaMs)
{
}

void OmniShadow::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void OmniShadow::OnCleanUp()
{
}

}
