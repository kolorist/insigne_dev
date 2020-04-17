#include "Empty.h"

#include <insigne/ut_render.h>

#include "InsigneImGui.h"

namespace stone
{
namespace perf
{

static const_cstr k_SuiteName = "no_rendering";

Empty::Empty()
{
}

Empty::~Empty()
{
}

const_cstr Empty::GetName() const
{
	return k_SuiteName;
}

void Empty::OnInitialize()
{
}

void Empty::OnUpdate(const f32 i_deltaMs)
{
}

void Empty::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Empty::OnCleanUp()
{
}

}
}
