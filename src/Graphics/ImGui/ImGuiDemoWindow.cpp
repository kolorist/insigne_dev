#include "ImGuiDemoWindow.h"

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
namespace gui
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

ImGuiDemoWindow::ImGuiDemoWindow()
{
}

//-------------------------------------------------------------------

ImGuiDemoWindow::~ImGuiDemoWindow()
{
}

//-------------------------------------------------------------------

ICameraMotion* ImGuiDemoWindow::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr ImGuiDemoWindow::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void ImGuiDemoWindow::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
}

//-------------------------------------------------------------------

void ImGuiDemoWindow::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::ShowTestWindow();
}

//-------------------------------------------------------------------

void ImGuiDemoWindow::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void ImGuiDemoWindow::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
}

//-------------------------------------------------------------------
}
}
