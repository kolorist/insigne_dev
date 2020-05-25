#include "TestSuite.h"

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

namespace stone
{
// ------------------------------------------------------------------

void TestSuite::OnInitialize(floral::filesystem<FreelistArena>* i_fs)
{
	m_FileSystem = i_fs;

	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	_OnInitialize();
}

void TestSuite::OnUpdate(const f32 i_deltaMs)
{
	_OnUpdate(i_deltaMs);
}

void TestSuite::OnRender(const f32 i_deltaMs)
{
	_OnRender(i_deltaMs);
}

void TestSuite::OnCleanUp()
{
	_OnCleanUp();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	m_FileSystem = nullptr;

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

// ------------------------------------------------------------------
}
