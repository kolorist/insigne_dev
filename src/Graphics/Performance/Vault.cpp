#include "Vault.h"

#include <floral/io/filesystem.h>

#include <clover/Logger.h>

#include <insigne/ut_render.h>

#include "InsigneImGui.h"

namespace stone
{
namespace perf
{
// -------------------------------------------------------------------

Vault::Vault()
{
}

Vault::~Vault()
{
}

ICameraMotion* Vault::GetCameraMotion()
{
	return nullptr;
}

const_cstr Vault::GetName() const
{
	return k_name;
}

void Vault::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);

	m_FSMemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(1));

#if 0
	floral::absolute_path workingDir = floral::get_application_directory();
	floral::relative_path dataPath = floral::build_relative_path("../data/tests/perf/vault");
	floral::concat_path(&workingDir, dataPath);
	floral::filesystem<FreelistArena>* fs = floral::create_filesystem(workingDir, m_FSMemoryArena);

	floral::relative_path filePath = floral::build_relative_path("out.cbsh");
	floral::file_info tmpFile = floral::open_file_read(fs, filePath);
#endif
}

void Vault::_OnUpdate(const f32 i_deltaMs)
{
}

void Vault::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Vault::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	g_StreammingAllocator.free(m_FSMemoryArena);
}

// -------------------------------------------------------------------
}
}
