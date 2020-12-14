#include "DebugDrawer.h"

#include <math.h>

#include <insigne/system.h>
#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include <clover/Logger.h>

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
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

static const_cstr s_FragmentShader = R"(#version 300 es

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
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(32));
}

DebugDrawer::~DebugDrawer()
{
}

// ---------------------------------------------
void DebugDrawer::DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color)
{
	u32 currentIdx = m_DebugVertices[m_CurrentBufferIdx].get_size();

	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { i_x0, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { i_x1, i_color } );
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

	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v0, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v1, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v2, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v3, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v4, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v5, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v6, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v7, i_color } );

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

void DebugDrawer::DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1,
		const floral::vec3f& i_p2, const floral::vec3f& i_p3,
		const floral::vec4f& i_color)
{
	DrawLine3D(i_p0, i_p1, i_color);
	DrawLine3D(i_p1, i_p2, i_color);
	DrawLine3D(i_p2, i_p3, i_color);
	DrawLine3D(i_p3, i_p0, i_color);
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
		m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { s_icosahedronVertices[i] * i_radius + i_origin, i_color });
	for (u32 i = 0; i < 120; i++)
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + s_icosahedronIndices[i]);
}

void DebugDrawer::DrawIcosphere3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color)
{
}

// ---------------------------------------------

void DebugDrawer::DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const floral::vec4f& i_color)
{
	floral::vec3f minCorner = i_position - floral::vec3f(i_size / 2.0f);
	floral::vec3f maxCorner = i_position + floral::vec3f(i_size / 2.0f);
	DrawSolidBox3D(minCorner, maxCorner, i_color);
}

void DebugDrawer::DrawSolidBox3D(const floral::vec3f& i_minCorner, const floral::vec3f& i_maxCorner, const floral::vec4f& i_color)
{
	floral::vec3f v2(i_minCorner);
	floral::vec3f v4(i_maxCorner);
	floral::vec3f v0(v4.x, v2.y, v4.z);
	floral::vec3f v1(v4.x, v2.y, v2.z);
	floral::vec3f v3(v2.x, v2.y, v4.z);
	floral::vec3f v5(v4.x, v4.y, v2.z);
	floral::vec3f v6(v2.x, v4.y, v2.z);
	floral::vec3f v7(v2.x, v4.y, v4.z);

	u32 currentIdx = m_DebugSurfaceVertices[m_CurrentBufferIdx].get_size();

	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v0, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v1, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v2, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v3, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v4, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v5, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v6, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v7, i_color } );

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
}

// ---------------------------------------------

void DebugDrawer::Initialize()
{
	CLOVER_VERBOSE("Initializing DebugDrawer");
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// DebugLine and DebugSurface has already been registered

	static const u32 s_verticesLimit = 1u << 17;
	static const s32 s_indicesLimit = 1u << 18;
	m_DebugVertices[0].init(s_verticesLimit, m_MemoryArena);
	m_DebugVertices[1].init(s_verticesLimit, m_MemoryArena);
	m_DebugIndices[0].init(s_indicesLimit, m_MemoryArena);
	m_DebugIndices[1].init(s_indicesLimit, m_MemoryArena);
	m_DebugSurfaceVertices[0].init(s_verticesLimit, m_MemoryArena);
	m_DebugSurfaceVertices[1].init(s_verticesLimit, m_MemoryArena);
	m_DebugSurfaceIndices[0].init(s_indicesLimit, m_MemoryArena);
	m_DebugSurfaceIndices[1].init(s_indicesLimit, m_MemoryArena);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_MB(32);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_MB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_MB(16);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_SurfaceVB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_MB(8);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_SurfaceIB = newIB;
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
		desc.vs_path = floral::path("/internal/debug_draw_vs");
		desc.fs_path = floral::path("/internal/debug_draw_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_XForm");
		m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		m_Material.render_state.depth_write = false;
		m_Material.render_state.depth_test = false;
		m_Material.render_state.cull_face = false;
		m_Material.render_state.blending = true;
		m_Material.render_state.blend_equation = insigne::blend_equation_e::func_add;
		m_Material.render_state.blend_func_sfactor = insigne::factor_e::fact_src_alpha;
		m_Material.render_state.blend_func_dfactor = insigne::factor_e::fact_one_minus_src_alpha;
	}
	// flush the initialization pass
	insigne::dispatch_render_pass();
}

void DebugDrawer::CleanUp()
{
	CLOVER_VERBOSE("Cleaning up DebugDrawer");

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

void DebugDrawer::Render(const floral::mat4x4f& i_wvp)
{
	m_Data[m_CurrentBufferIdx].WVP = i_wvp;
	insigne::update_ub(m_UB, &m_Data[m_CurrentBufferIdx], sizeof(MyData), 0);

	if (m_DebugIndices[m_CurrentBufferIdx].get_size() > 0)
	{
		insigne::draw_surface<DebugLine>(m_VB, m_IB, m_Material);
	}

	if (m_DebugSurfaceIndices[m_CurrentBufferIdx].get_size() > 0)
	{
		insigne::draw_surface<DebugSurface>(m_SurfaceVB, m_SurfaceIB, m_Material);
	}
}

void DebugDrawer::BeginFrame()
{
	m_CurrentBufferIdx = (m_CurrentBufferIdx + 1) % 2;
	m_DebugVertices[m_CurrentBufferIdx].empty();
	m_DebugIndices[m_CurrentBufferIdx].empty();
	m_DebugSurfaceVertices[m_CurrentBufferIdx].empty();
	m_DebugSurfaceIndices[m_CurrentBufferIdx].empty();
}

void DebugDrawer::EndFrame()
{
	if (m_DebugVertices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_vb(m_VB, &(m_DebugVertices[m_CurrentBufferIdx][0]), m_DebugVertices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_vb(m_VB, nullptr, 0, 0);

	if (m_DebugIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_ib(m_IB, &(m_DebugIndices[m_CurrentBufferIdx][0]), m_DebugIndices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_ib(m_IB, nullptr, 0, 0);

	if (m_DebugSurfaceVertices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_vb(m_SurfaceVB, &(m_DebugSurfaceVertices[m_CurrentBufferIdx][0]), m_DebugSurfaceVertices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_vb(m_SurfaceVB, nullptr, 0, 0);

	if (m_DebugSurfaceIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_ib(m_SurfaceIB, &(m_DebugSurfaceIndices[m_CurrentBufferIdx][0]), m_DebugSurfaceIndices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_ib(m_SurfaceIB, nullptr, 0, 0);
}

//----------------------------------------------
static DebugDrawer* s_DebugDrawer;

static const floral::vec4f k_Colors[] = {
	floral::vec4f(0.0f, 0.0f, 0.5f, 1.0f),		// blue
	floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f),		// bright blue
	floral::vec4f(0.5f, 0.0f, 0.0f, 1.0f),		// red
	floral::vec4f(0.5f, 0.0f, 0.5f, 1.0f),		// magenta
	floral::vec4f(0.5f, 0.0f, 1.0f, 1.0f),		// violet
	floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f),		// bright red
	floral::vec4f(1.0f, 0.0f, 0.5f, 1.0f),		// purple
	floral::vec4f(1.0f, 0.0f, 1.0f, 1.0f),		// bright magenta
	floral::vec4f(0.0f, 0.5f, 0.0f, 1.0f),		// green
	floral::vec4f(0.0f, 0.5f, 0.5f, 1.0f),		// cyan
	floral::vec4f(0.0f, 0.5f, 1.0f, 1.0f),		// sky blue
	floral::vec4f(0.5f, 0.5f, 0.0f, 1.0f),		// yellow
	floral::vec4f(0.5f, 0.5f, 0.5f, 1.0f),		// gray
	floral::vec4f(0.5f, 0.5f, 1.0f, 1.0f),		// pale blue
	floral::vec4f(1.0f, 0.5f, 0.0f, 1.0f),		// orange
	floral::vec4f(1.0f, 0.5f, 0.5f, 1.0f),		// pink
	floral::vec4f(1.0f, 0.5f, 1.0f, 1.0f),		// pale magenta
	floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f),		// bright green
	floral::vec4f(0.0f, 1.0f, 0.5f, 1.0f),		// sea green
	floral::vec4f(0.0f, 1.0f, 1.0f, 1.0f),		// bright cyan
	floral::vec4f(0.5f, 1.0f, 0.0f, 1.0f),		// lime green
	floral::vec4f(0.5f, 1.0f, 0.5f, 1.0f),		// pale green
	floral::vec4f(0.5f, 1.0f, 1.0f, 1.0f),		// pale cyan
	floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f),		// bright yellow
	floral::vec4f(1.0f, 1.0f, 0.5f, 1.0f),		// pale yellow
};
static const size k_ColorsCount = 25;

namespace debugdraw
{

void Initialize()
{
	FLORAL_ASSERT(s_DebugDrawer == nullptr);
	s_DebugDrawer = g_PersistanceResourceAllocator.allocate<DebugDrawer>();
	s_DebugDrawer->Initialize();
}

void CleanUp()
{
	s_DebugDrawer->CleanUp();
}

void BeginFrame()
{
	s_DebugDrawer->BeginFrame();
}

void EndFrame()
{
	s_DebugDrawer->EndFrame();
}

void Render(const floral::mat4x4f& i_wvp)
{
	s_DebugDrawer->Render(i_wvp);
}

void DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawLine3D(i_x0, i_x1, i_color);
}

void DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1, const floral::vec3f& i_p2, const floral::vec3f& i_p3, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawQuad3D(i_p0, i_p1, i_p2, i_p3, i_color);
}

void DrawAABB3D(const floral::aabb3f& i_aabb, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawAABB3D(i_aabb, i_color);
}

void DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawPoint3D(i_position, i_size, i_color);
}

void DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const size i_colorIdx)
{
	s_DebugDrawer->DrawPoint3D(i_position, i_size, k_Colors[i_colorIdx % k_ColorsCount]);
}

}

}
