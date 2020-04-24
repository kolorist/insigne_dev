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

void PlotValuesWrap(const_cstr i_title, f32* i_values, const f32 i_minValue, const f32 i_maxValue, const size i_arraySize, const s32 i_height,
		const s32 i_startIdx = 0, const ImU32 i_labelColor = 0xFFFFFFFF, const ImU32 i_lineColor = 0xFFFFFFFF)
{
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImFont* font = ImGui::GetFont();
	const f32 valueFontSize = ImGui::GetFontSize();
	ImVec2 canvasPos = ImGui::GetCursorScreenPos();
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();

	ImVec2 frameSize(canvasSize.x, i_height);
	c8 minValueText[32];
	c8 maxValueText[32];
	sprintf(minValueText, "%4.2f", i_minValue);
	sprintf(maxValueText, "%4.2f", i_maxValue);
	ImVec2 minValueTextSize = font->CalcTextSizeA(valueFontSize, FLT_MAX, 0.0f, minValueText);
	ImVec2 maxValueTextSize = font->CalcTextSizeA(valueFontSize, FLT_MAX, 0.0f, maxValueText);

	ImGui::BeginGroup();
	ImGui::Text("%s (%4.2f)", i_title, i_values[i_startIdx]);
	bool visible = ImGui::BeginChildFrame(ImGui::GetID(i_title), frameSize);

	const f32 textPanelWidth = floral::max(maxValueTextSize.x, minValueTextSize.x) + 4.0f;
	const f32 textHeight = minValueTextSize.y;
	ImVec2 contentMin = ImGui::GetWindowPos();
	const ImVec2 textPanelContentMin = contentMin;
	const ImVec2 contentSize = ImGui::GetWindowSize();
	ImVec2 rectMax(contentMin.x + contentSize.x, contentMin.y + contentSize.y);
	drawList->PushClipRect(contentMin, rectMax, true);

	if (visible)
	{
		contentMin.x += textPanelWidth;
		const f32 width = contentSize.x - textPanelWidth;
		const f32 height = contentSize.y - 1;

		drawList->AddText(font, ImGui::GetFontSize(), textPanelContentMin, i_labelColor, maxValueText);
		drawList->AddText(font, ImGui::GetFontSize(), ImVec2(textPanelContentMin.x, textPanelContentMin.y + height - textHeight), i_labelColor, minValueText);

		if (i_arraySize > 1)
		{
			const s32 numValues = (s32)i_arraySize;
			const f32 stepWidth = width / (numValues - 1);
			s32 v1 = i_startIdx;
			s32 end = v1 - 1;
			if (end < 0)
			{
				end = i_arraySize - 1;
			}

			s32 i = 0;
			do
			{
				s32 v2 = (v1 + 1) % i_arraySize;
				f32 value1 = floral::clamp(i_values[v1], i_minValue, i_maxValue);
				f32 value2 = floral::clamp(i_values[v2], i_minValue, i_maxValue);
				ImVec2 p1(contentMin.x + i * stepWidth, contentMin.y + height - (value1 - i_minValue) * height / i_maxValue);
				ImVec2 p2(contentMin.x + (i + 1) * stepWidth, contentMin.y + height - (value2 - i_minValue) * height / i_maxValue);
				drawList->AddLine(p1, p2, i_lineColor);
				i++;
				v1 = v2;
			}
			while (v1 != end);
		}
	}

	drawList->PopClipRect();

	ImGui::EndChildFrame();
	ImGui::EndGroup();
}

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

	PlotValuesWrap("delta ms", deltaMs, 0.0f, 33.0f, IM_ARRAYSIZE(deltaMs), 50, plotIdx, IM_COL32(0, 255, 0, 255), IM_COL32(0, 255, 0, 255));
	PlotValuesWrap("delta ms 2", deltaMs, 0.0f, 33.0f, IM_ARRAYSIZE(deltaMs), 50, plotIdx, IM_COL32(255, 0, 0, 255), IM_COL32(255, 0, 0, 255));

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
