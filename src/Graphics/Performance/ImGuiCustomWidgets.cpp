#include "ImGuiCustomWidgets.h"

#include <floral/containers/array.h>
#include <floral/gpds/camera.h>

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

static const_cstr k_SuiteName = "imgui custom widgets";

//----------------------------------------------

static const_cstr s_VertexShaderCode = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_VertColor;

out vec4 o_VertColor;

void main() {
	o_VertColor = l_VertColor;
	gl_Position = vec4(l_Position_L, 1.0f);
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


//----------------------------------------------

ImGuiCustomWidgets::ImGuiCustomWidgets()
{
}

ImGuiCustomWidgets::~ImGuiCustomWidgets()
{
}

const_cstr ImGuiCustomWidgets::GetName() const
{
	return k_SuiteName;
}

void ImGuiCustomWidgets::OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfacePC>();

	floral::inplace_array<VertexPC, 3> vertices;
	vertices.push_back(VertexPC { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
	vertices.push_back(VertexPC { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });
	vertices.push_back(VertexPC { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } });

	floral::inplace_array<s32, 3> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;
		
		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(VertexPC), 0);
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
	}

	floral::camera_view_t view;
	view.position = floral::vec3f(1.0f, 1.0f, 1.0f);
	view.look_at = floral::vec3f(0.0f, 0.0f, 0.0f);
	view.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);
	m_ViewMatrix = floral::construct_lookat_point(view);
}


void ImGuiCustomWidgets::OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Custom Widgets");
	static floral::vec2f v2(1.0f, 2.0f);
	static floral::vec3f v3(1.0f, 2.0f, 3.0f);
	static floral::vec4f v4(1.0f, 2.0f, 3.0f, 4.0f);
	DebugVec2f("v2", &v2);
	DebugVec3f("v3", &v3);
	DebugVec4f("v4", &v4);

	DebugMat4fRowOrder("View", &m_ViewMatrix);

	ImGui::End();
}

void ImGuiCustomWidgets::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<SurfacePC>(m_VB, m_IB, m_Material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void ImGuiCustomWidgets::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	insigne::unregister_surface_type<SurfacePC>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

}
}
