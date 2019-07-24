#include "GPUQuad2DRasterize.h"
#include <calyx/context.h>

#include <clover.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

out mediump vec4 v_VertexColor;

void main() {
	v_VertexColor = l_Color;
	gl_Position = vec4(l_Position_L, 1.0f);
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_VertexColor;

void main()
{
	o_Color = vec4(v_VertexColor);
}
)";

GPUQuad2DRasterize::GPUQuad2DRasterize()
	: m_CameraMotion(
			floral::camera_view_t { floral::vec3f(3.0f, 3.0f, 3.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
			floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

GPUQuad2DRasterize::~GPUQuad2DRasterize()
{
}

void GPUQuad2DRasterize::OnInitialize()
{
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

	{
		VertexPC vertices[] =
		{
			VertexPC { { -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f} },
			VertexPC { { 1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f} },
			VertexPC { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f} },
			VertexPC { { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f} },
		};

		s32 indices[] =
		{ 0, 1, 2, 2, 3, 0 };

		{
			insigne::vbdesc_t desc;
			desc.region_size = SIZE_KB(256);
			desc.stride = sizeof(VertexPC);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::dynamic_draw;

			insigne::vb_handle_t newVB = insigne::create_vb(desc);
			insigne::copy_update_vb(newVB, vertices, 4, sizeof(VertexPC), 0);
			m_VB = newVB;
		}

		{
			insigne::ibdesc_t desc;
			desc.region_size = SIZE_KB(128);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::dynamic_draw;

			insigne::ib_handle_t newIB = insigne::create_ib(desc);
			insigne::copy_update_ib(newIB, indices, 6, 0);
			m_IB = newIB;
		}
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/cornel_box_vs");
		desc.fs_path = floral::path("/scene/cornel_box_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);
	}

	m_DebugDrawer.Initialize();
}

void GPUQuad2DRasterize::OnUpdate(const f32 i_deltaMs)
{
	m_CameraMotion.OnUpdate(i_deltaMs);

	m_DebugDrawer.BeginFrame();

	// ground grid cover [-2.0..2.0]
	floral::vec4f gridColor(0.3f, 0.4f, 0.5f, 1.0f);
	for (s32 i = -2; i < 3; i++)
	{
		for (s32 j = -2; j < 3; j++)
		{
			floral::vec3f startX(1.0f * i, 0.0f, 1.0f * j);
			floral::vec3f startZ(1.0f * j, 0.0f, 1.0f * i);
			floral::vec3f endX(1.0f * i, 0.0f, -1.0f * j);
			floral::vec3f endZ(-1.0f * j, 0.0f, 1.0f * i);
			m_DebugDrawer.DrawLine3D(startX, endX, gridColor);
			m_DebugDrawer.DrawLine3D(startZ, endZ, gridColor);
		}
	}

	// coordinate unit vectors
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.5f, 0.0f, 0.1f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.5f, 0.1f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	m_DebugDrawer.DrawLine3D(floral::vec3f(0.1f, 0.0f, 0.1f), floral::vec3f(0.1f, 0.0f, 0.5f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

	m_DebugDrawer.EndFrame();
}

void GPUQuad2DRasterize::OnDebugUIUpdate(const f32 i_deltaMs)
{
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();

	ImGui::Begin("GPUQuad2DRasterize Controller");
	ImGui::Text("Screen resolution: %d x %d",
			commonCtx->window_width,
			commonCtx->window_height);

	ImGui::End();
}

void GPUQuad2DRasterize::OnRender(const f32 i_deltaMs)
{
	// camera
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	m_DebugDrawer.Render(m_SceneData.WVP);
	insigne::draw_surface<SurfacePC>(m_VB, m_IB, m_Material);
	IDebugUI::OnFrameRender(i_deltaMs);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GPUQuad2DRasterize::OnCleanUp()
{
}

}
