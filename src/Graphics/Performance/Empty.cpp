#include "Empty.h"

#include <clover/Logger.h>
#include <insigne/ut_render.h>

#include "InsigneImGui.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------


Empty::Empty()
	: m_DataArenaRegion { "stone/dynamic/sky", SIZE_MB(32), &m_DataArena }
{
}

Empty::~Empty()
{
}

ICameraMotion* Empty::GetCameraMotion()
{
	return nullptr;
}

const_cstr Empty::GetName() const
{
	return k_name;
}

void Empty::_OnInitialize()
{
	g_MemoryManager.initialize_allocator(m_DataArenaRegion);
}

void Empty::_OnUpdate(const f32 i_deltaMs)
{
}

void Empty::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Empty::_OnCleanUp()
{
	g_MemoryManager.destroy_allocator(m_DataArenaRegion);
}

//-------------------------------------------------------------------
}
}
