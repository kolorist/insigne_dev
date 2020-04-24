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
