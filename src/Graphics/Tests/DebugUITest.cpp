#include "DebugUITest.h"

#include <clover.h>
#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

namespace stone
{

DebugUITest::DebugUITest()
{
}

DebugUITest::~DebugUITest()
{
}

void DebugUITest::OnInitialize()
{
}

void DebugUITest::OnUpdate(const f32 i_deltaMs)
{
}

void DebugUITest::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("DebugUITest Controller");
	if (ImGui::Button("Test Clover Log"))
	{
		CLOVER_DEBUG("test test test");
	}
	ImGui::End();
}

void DebugUITest::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void DebugUITest::OnCleanUp()
{
}

}
