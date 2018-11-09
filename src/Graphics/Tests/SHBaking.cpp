#include "SHBaking.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"

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

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_GeometryFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	o_Color = v_Color;
}
)";

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

	GenTessellated3DPlane_Tris_PNC(floral::mat4x4f(1.0f), 3.0f, 10, floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f),
			m_GeoVertices, m_GeoIndices);

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

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_CamView.position = floral::vec3f(5.0f, 5.0f, 5.0f);
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
	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void SHBaking::OnUpdate(const f32 i_deltaMs)
{
}

void SHBaking::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	// render here
	insigne::draw_surface<SurfacePNC>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHBaking::OnCleanUp()
{
}

}
