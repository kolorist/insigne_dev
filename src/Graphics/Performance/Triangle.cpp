#include "Triangle.h"

#include <floral/containers/array.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

static const_cstr s_VertexShaderCode = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position_L;
layout (location = 1) in mediump vec4 l_VertColor;

out vec4 o_VertColor;

void main() {
	o_VertColor = l_VertColor;
	gl_Position = vec4(l_Position_L, 0.0f, 1.0f);
}
)";

static const_cstr s_FragmentShaderCode = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 o_VertColor;

void main()
{
	o_Color = o_VertColor;
}
)";


//-------------------------------------------------------------------

Triangle::Triangle()
{
}

//-------------------------------------------------------------------

Triangle::~Triangle()
{
}

//-------------------------------------------------------------------

ICameraMotion* Triangle::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr Triangle::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void Triangle::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePC>();

	floral::inplace_array<geo2d::VertexPC, 3> vertices;
	vertices.push_back(geo2d::VertexPC { { 0.0f, 0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
	vertices.push_back(geo2d::VertexPC { { -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } });
	vertices.push_back(geo2d::VertexPC { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f } });

	floral::inplace_array<s32, 3> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(geo2d::VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(geo2d::VertexPC), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_IB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_IB, &indices[0], indices.get_size(), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();

		strcpy(desc.vs, s_VertexShaderCode);
		strcpy(desc.fs, s_FragmentShaderCode);
		desc.vs_path = floral::path("/demo/simple_vs");
		desc.fs_path = floral::path("/demo/simple_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);
		m_Material.render_state.depth_write = true;
		m_Material.render_state.depth_test = true;
		m_Material.render_state.depth_func = insigne::compare_func_e::func_less_or_equal;
		m_Material.render_state.blending = false;
		m_Material.render_state.cull_face = true;
		m_Material.render_state.face_side = insigne::face_side_e::back_side;
		m_Material.render_state.front_face = insigne::front_face_e::face_ccw;
	}
}

//-------------------------------------------------------------------

void Triangle::_OnUpdate(const f32 i_deltaMs)
{
}

//-------------------------------------------------------------------

void Triangle::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePC>(m_VB, m_IB, m_Material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void Triangle::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePC>();
}

//-------------------------------------------------------------------
}
}
