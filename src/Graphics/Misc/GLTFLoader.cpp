#include "GLTFLoader.h"

#include <clover/Logger.h>
#include <insigne/ut_render.h>

#include "InsigneImGui.h"

namespace stone
{
namespace misc
{
//-------------------------------------------------------------------

GLTFLoader::GLTFLoader()
{
}

GLTFLoader::~GLTFLoader()
{
}

ICameraMotion* GLTFLoader::GetCameraMotion()
{
	return nullptr;
}

const_cstr GLTFLoader::GetName() const
{
	return k_name;
}

void GLTFLoader::_OnInitialize()
{
}

void GLTFLoader::_OnUpdate(const f32 i_deltaMs)
{
}

void GLTFLoader::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GLTFLoader::_OnCleanUp()
{
}

//-------------------------------------------------------------------
}
}
