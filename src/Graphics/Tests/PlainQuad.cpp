#include "PlainQuad.h"

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

namespace stone {
#define TEST_CASE 2
#if (TEST_CASE == 0)
// very simple, stress nothing
static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	gl_Position = pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Data
{
	mediump vec4 iu_Color;
};

void main()
{
	o_Color = iu_Color;
}
)";
#elif (TEST_CASE == 1)
// arithmetic (tripipe) stress
static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

out mediump vec3 v_color;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_color = l_Position_L;
	gl_Position = pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Data
{
	mediump vec4 iu_Color;
};

in mediump vec3 v_color;

void main()
{
	mediump vec3 a = v_color;
	a = sqrt(a) * a / log(a) / sin(a) / cos(a) / tan(a) * sqrt(a);
	a *= vec3(0.00001);
	o_Color = iu_Color + vec4(a, 1.0f);
}
)";
#elif (TEST_CASE == 2)
// varying (tripipe) stress
static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

out mediump vec3 v_color0;
out mediump vec3 v_color1;
out mediump vec3 v_color2;
out mediump vec3 v_color3;
out highp vec3 v_color4;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_color0 = l_Position_L.xxx;
	v_color1 = l_Position_L.yyy;
	v_color2 = l_Position_L.zzz;
	v_color3 = l_Position_L.xyz;
	v_color4 = l_Position_L.zyx;
	gl_Position = pos_W;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

layout(std140) uniform ub_Data
{
	mediump vec4 iu_Color;
};

in mediump vec3 v_color0;
in mediump vec3 v_color1;
in mediump vec3 v_color2;
in mediump vec3 v_color3;
in highp vec3 v_color4;

void main()
{
	mediump vec3 a = (v_color0 + v_color1 + v_color2 + v_color3 + v_color4) / 5.0f;
	o_Color = iu_Color * vec4(a, 1.0f);
}
)";
#endif

PlainQuadTest::PlainQuadTest()
{
}

PlainQuadTest::~PlainQuadTest()
{
}

void PlainQuadTest::OnInitialize()
{
	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DemoVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);

		m_Vertices.init(4u, &g_StreammingAllocator);
		m_Vertices.push_back({ floral::vec3f(-0.5f, -0.5f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f) });
		m_Vertices.push_back({ floral::vec3f(0.5f, -0.5f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f) });
		m_Vertices.push_back({ floral::vec3f(0.5f, 0.5f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f) });
		m_Vertices.push_back({ floral::vec3f(-0.5f, 0.5f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f) });

		insigne::update_vb(newVB, &m_Vertices[0], 4, 0);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);

		m_Indices.init(6u, &g_StreammingAllocator);
		m_Indices.push_back(0);
		m_Indices.push_back(1);
		m_Indices.push_back(2);
		m_Indices.push_back(2);
		m_Indices.push_back(3);
		m_Indices.push_back(0);

		insigne::update_ib(newIB, &m_Indices[0], 6, 0);
		m_IB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_Data.Color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);

		insigne::update_ub(newUB, &m_Data, sizeof(MyData), 0);
		m_UB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Data", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_Data");
		m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t{0, 0, m_UB};
	}
	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void PlainQuadTest::OnUpdate(const f32 i_deltaMs)
{
}

void PlainQuadTest::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	// render here
	insigne::draw_surface<DemoSurface>(m_VB, m_IB, m_Material);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void PlainQuadTest::OnCleanUp()
{
}

}
