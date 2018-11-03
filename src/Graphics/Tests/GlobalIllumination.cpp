#include "GlobalIllumination.h"

#include <insigne/commons.h>
#include <insigne/counters.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>

#include "Graphics/GeometryBuilder.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Graphics/stb_image_write.h"

#include "GIShader.h"

namespace stone {

GlobalIllumination::GlobalIllumination()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

GlobalIllumination::~GlobalIllumination()
{
}

void GlobalIllumination::OnInitialize()
{
	m_DebugDrawer.Initialize();
}

void GlobalIllumination::OnUpdate(const f32 i_deltaMs)
{
	m_DebugDrawer.BeginFrame();
	m_DebugDrawer.EndFrame();
}

void GlobalIllumination::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GlobalIllumination::OnCleanUp()
{
}

}
