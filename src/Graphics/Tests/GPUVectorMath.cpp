#include "GPUVectorMath.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

namespace stone {

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Dynamic
{
	highp mat4 iu_XForm;
	mediump vec4 iu_Color;
};

layout(std140) uniform ub_Static
{
	highp mat4 iu_WVP;
};

void main() {
	highp vec4 pos_W = iu_XForm * vec4(l_Position_L, 1.0f);
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Dynamic
{
	highp mat4 iu_XForm;
	mediump vec4 iu_Color;
};

void main()
{
	o_Color = iu_Color;
}
)";

GPUVectorMath::GPUVectorMath()
{
}

GPUVectorMath::~GPUVectorMath()
{
}

void GPUVectorMath::OnInitialize()
{
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DemoVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		m_Vertices.init(8u, &g_StreammingAllocator);
		m_Vertices.push_back({ floral::vec3f(-1.0f, -1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, -1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, -1.0f, -1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(-1.0f, -1.0f, -1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(-1.0f, 1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, 1.0f, 1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(1.0f, 1.0f, -1.0f), floral::vec4f(0.0f) });
		m_Vertices.push_back({ floral::vec3f(-1.0f, 1.0f, -1.0f), floral::vec4f(0.0f) });

		insigne::update_vb(newVB, &m_Vertices[0], 8, 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		m_Indices.init(36u, &g_StreammingAllocator);
		m_Indices.push_back(2);
		m_Indices.push_back(1);
		m_Indices.push_back(0);
		m_Indices.push_back(0);
		m_Indices.push_back(3);
		m_Indices.push_back(2);

		m_Indices.push_back(4);
		m_Indices.push_back(5);
		m_Indices.push_back(6);
		m_Indices.push_back(6);
		m_Indices.push_back(7);
		m_Indices.push_back(4);

		m_Indices.push_back(3);
		m_Indices.push_back(0);
		m_Indices.push_back(4);
		m_Indices.push_back(4);
		m_Indices.push_back(7);
		m_Indices.push_back(3);

		m_Indices.push_back(5);
		m_Indices.push_back(1);
		m_Indices.push_back(2);
		m_Indices.push_back(2);
		m_Indices.push_back(6);
		m_Indices.push_back(5);

		m_Indices.push_back(4);
		m_Indices.push_back(0);
		m_Indices.push_back(1);
		m_Indices.push_back(1);
		m_Indices.push_back(5);
		m_Indices.push_back(4);

		m_Indices.push_back(6);
		m_Indices.push_back(2);
		m_Indices.push_back(3);
		m_Indices.push_back(3);
		m_Indices.push_back(7);
		m_Indices.push_back(6);

		insigne::update_ib(newIB, &m_Indices[0], m_Indices.get_size(), 0);
		m_IB = newIB;
	}

	// static data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		// camera
		m_CamView.position = floral::vec3f(5.0f, 5.0f, 5.0f);
		m_CamView.look_at = floral::vec3f(0.0f);
		m_CamView.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		m_CamProj.near_plane = 0.01f; m_CamProj.far_plane = 100.0f;
		m_CamProj.fov = 60.0f;
		m_CamProj.aspect_ratio = 16.0f / 9.0f;

		m_StaticData.WVP = floral::construct_perspective(m_CamProj) * construct_lookat_point(m_CamView);

		insigne::update_ub(newUB, &m_StaticData, sizeof(StaticData), 0);
		m_StaticDataUB = newUB;
	}

	// dynamic data
	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_DynamicData[0].XForm =
			floral::construct_translation3d(0.0f, 0.0f, 2.0f) *
			floral::construct_quaternion_euler(0.0f, 0.0f, 45.0f).to_transform() *
			floral::construct_scaling3d(0.2f, 0.2f, 0.2f);
		m_DynamicData[0].Color = floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f);

		m_DynamicData[1].XForm =
			floral::construct_translation3d(0.0f, 0.0f, -2.0f) *
			floral::construct_quaternion_euler(0.0f, 45.0f, 0.0f).to_transform() *
			floral::construct_scaling3d(0.2f, 0.2f, 0.2f);
		m_DynamicData[1].Color = floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f);

		m_DynamicData[2].XForm =
			floral::construct_translation3d(1.0f, 0.0f, 0.0f) *
			floral::construct_quaternion_euler(0.0f, 0.0f, 0.0f).to_transform() *
			floral::construct_scaling3d(0.3f, 0.3f, 0.3f);
		m_DynamicData[2].Color = floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f);

		insigne::update_ub(newUB, &m_DynamicData[0], sizeof(DynamicData), 0);
		insigne::update_ub(newUB, &m_DynamicData[1], sizeof(DynamicData), 256);
		insigne::update_ub(newUB, &m_DynamicData[2], sizeof(DynamicData), 512);
		m_DynamicDataUB = newUB;
	}

	// shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Dynamic", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Static", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Static");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_StaticDataUB };
		}

		// dynamic uniform data
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Dynamic");
			m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_DynamicDataUB };
		}
	}

	SnapshotAllocatorInfos();
	insigne::dispatch_render_pass();
}

void GPUVectorMath::OnUpdate(const f32 i_deltaMs)
{
}

void GPUVectorMath::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	{
		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Dynamic");

		m_Material.uniform_blocks[ubSlot].value.offset = 0;
		insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);

		m_Material.uniform_blocks[ubSlot].value.offset = 256;
		insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);

		m_Material.uniform_blocks[ubSlot].value.offset = 512;
		insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
	}

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GPUVectorMath::OnCleanUp()
{
}

}
