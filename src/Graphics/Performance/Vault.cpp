#include "Vault.h"

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
}

// -------------------------------------------------------------------
}
}
