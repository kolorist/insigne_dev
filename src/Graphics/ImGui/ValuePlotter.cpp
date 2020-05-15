#include "ValuePlotter.h"

#include <floral/containers/array.h>
#include <floral/math/utils.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/counters.h>
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

ValuePlotter::ValuePlotter()
{
}

//-------------------------------------------------------------------

ValuePlotter::~ValuePlotter()
{
}

//-------------------------------------------------------------------

ICameraMotion* ValuePlotter::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr ValuePlotter::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void ValuePlotter::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
}

//-------------------------------------------------------------------

void ValuePlotter::_OnUpdate(const f32 i_deltaMs)
{
	static f32 deltaMs[64];
	static f32 maxValue = 0.0f;
	static f32 minValue = 0.0f;
	static s32 currIdx = 0;

	const u64 validIdx = insigne::get_valid_debug_info_range_begin();
	f32 valueMs = insigne::g_debug_frame_counters[validIdx].frame_duration_ms;
	deltaMs[currIdx] = valueMs;
	size plotIdx = currIdx;
	currIdx--;
	if (currIdx < 0) currIdx = 63;
	if (valueMs > maxValue) maxValue = valueMs;
	if (valueMs < minValue) minValue = valueMs;

	ImGui::Begin("Controller");
	ImGui::Text("Header text");

	PlotValuesWrap("delta ms", deltaMs, 0.0f, 33.0f, IM_ARRAYSIZE(deltaMs), 150, plotIdx, IM_COL32(0, 255, 0, 255), IM_COL32(0, 255, 0, 255));
	PlotValuesWrap("delta ms 2", deltaMs, 0.0f, 33.0f, IM_ARRAYSIZE(deltaMs), 150, plotIdx, IM_COL32(255, 0, 0, 255), IM_COL32(255, 0, 0, 255));

	ImGui::Text("Footer text");
	ImGui::End();
}

//-------------------------------------------------------------------

void ValuePlotter::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void ValuePlotter::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
}

//-------------------------------------------------------------------
}
}
