#include "DebugDrawer.h"

#include <math.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

namespace stone {

static const_cstr s_VertexShader = R"(
#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec4 v_Color;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color;
	highp vec4 pos = iu_WVP * pos_W;
	gl_Position = pos;
}
)";

static const_cstr s_FragmentShader = R"(
#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	o_Color = v_Color;
}
)";

DebugDrawer::DebugDrawer()
	: m_CurrentBufferIdx(0)
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

DebugDrawer::~DebugDrawer()
{
}

// ---------------------------------------------
void DebugDrawer::DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color)
{
	u32 currentIdx = m_DebugVertices[m_CurrentBufferIdx].get_size();

	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { i_x0, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { i_x1, i_color } );
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
}

void DebugDrawer::DrawAABB3D(const floral::aabb3f& i_aabb, const floral::vec4f& i_color)
{
	floral::vec3f v0(i_aabb.min_corner);
	floral::vec3f v6(i_aabb.max_corner);
	floral::vec3f v1(v6.x, v0.y, v0.z);
	floral::vec3f v2(v6.x, v0.y, v6.z);
	floral::vec3f v3(v0.x, v0.y, v6.z);
	floral::vec3f v4(v0.x, v6.y, v0.z);
	floral::vec3f v5(v6.x, v6.y, v0.z);
	floral::vec3f v7(v0.x, v6.y, v6.z);

	u32 currentIdx = m_DebugVertices[m_CurrentBufferIdx].get_size();

	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v0, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v1, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v2, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v3, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v4, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v5, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v6, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { v7, i_color } );

	// bottom
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	// top
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	// side
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
}

void DebugDrawer::DrawIcosahedron3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color)
{
	static const floral::vec3f s_icosahedronVertices[12] = {
		floral::vec3f(-0.525731f, 0, 0.850651f), floral::vec3f(0.525731f, 0, 0.850651f),
		floral::vec3f(-0.525731f, 0, -0.850651f), floral::vec3f(0.525731f, 0, -0.850651f),
		floral::vec3f(0, 0.850651f, 0.525731f), floral::vec3f(0, 0.850651f, -0.525731f),
		floral::vec3f(0, -0.850651f, 0.525731f), floral::vec3f(0, -0.850651f, -0.525731f),
		floral::vec3f(0.850651f, 0.525731f, 0), floral::vec3f(-0.850651f, 0.525731f, 0),
		floral::vec3f(0.850651f, -0.525731f, 0), floral::vec3f(-0.850651f, -0.525731f, 0)
	};

	static const u32 s_icosahedronIndices[120] = {
		1,4,4,0,0,1,  4,9,9,0,0,4,  4,5,5,9,9,4,  8,5,5,4,4,8,  1,8,8,4,4,1,
		1,10,10,8,8,1, 10,3,3,8,8,10, 8,3,3,5,5,8,  3,2,2,5,5,3,  3,7,7,2,2,3,
		3,10,10,7,7,3, 10,6,6,7,7,10, 6,11,11,7,7,6, 6,0,0,11,11,6, 6,1,1,0,0,6,
		10,1,1,6,6,10, 11,0,0,9,9,11, 2,11,11,9,9,2, 5,2,2,9,9,5,  11,2,2,7,7,11
	};

	u32 idx = m_DebugVertices[m_CurrentBufferIdx].get_size();
	for (u32 i = 0; i < 12; i++)
		m_DebugVertices[m_CurrentBufferIdx].push_back(DebugVertex { s_icosahedronVertices[i] * i_radius + i_origin, i_color });
	for (u32 i = 0; i < 120; i++)
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + s_icosahedronIndices[i]);
}

void DebugDrawer::DrawIcosphere3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color)
{
}

// ---------------------------------------------

void DebugDrawer::Initialize()
{
	static const u32 s_verticesLimit = 8192u;
	static const s32 s_indicesLimit = 16384u;
	m_DebugVertices[0].init(s_verticesLimit, m_MemoryArena);
	m_DebugVertices[1].init(s_verticesLimit, m_MemoryArena);
	m_DebugIndices[0].init(s_indicesLimit, m_MemoryArena);
	m_DebugIndices[1].init(s_indicesLimit, m_MemoryArena);


	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.stride = sizeof(DebugVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(64);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_UB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_XForm");
		m_Material.uniform_blocks[ubSlot].value = m_UB;
	}
	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void DebugDrawer::Render(const floral::mat4x4f& i_wvp)
{
	m_Data[m_CurrentBufferIdx].WVP = i_wvp;
	insigne::update_ub(m_UB, &m_Data[m_CurrentBufferIdx], sizeof(MyData), 0);

	if (m_DebugIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::draw_surface<DebugLine>(m_VB, m_IB, m_Material);
}

void DebugDrawer::BeginFrame()
{
	m_CurrentBufferIdx = (m_CurrentBufferIdx + 1) % 2;
	m_DebugVertices[m_CurrentBufferIdx].empty();
	m_DebugIndices[m_CurrentBufferIdx].empty();
}

void DebugDrawer::EndFrame()
{
	if (m_DebugVertices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_vb(m_VB, &(m_DebugVertices[m_CurrentBufferIdx][0]), m_DebugVertices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_vb(m_VB, nullptr, 0, 0);

	if (m_DebugIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_ib(m_IB, &(m_DebugIndices[m_CurrentBufferIdx][0]), m_DebugIndices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_ib(m_IB, nullptr, 0, 0);
}

}
