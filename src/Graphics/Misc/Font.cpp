#include "Font.h"

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/stb_truetype.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace misc
{
//-------------------------------------------------------------------


Font::Font()
{
}

Font::~Font()
{
}

ICameraMotion* Font::GetCameraMotion()
{
	return nullptr;
}

const_cstr Font::GetName() const
{
	return k_name;
}

void Font::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/misc/font");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
#if 0
	m_MemoryArena->free_all();
	floral::relative_path iFilePath = floral::build_relative_path("NotoSans-Regular.ttf");
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	FLORAL_ASSERT(iFile.file_size > 0);
	p8 ttfData = (p8)m_MemoryArena->allocate(iFile.file_size);
	floral::read_all_file(iFile, ttfData);
	floral::close_file(iFile);

	p8 fontBitmap = (p8)m_MemoryArena->allocate(1024 * 1024);
	stbtt_bakedchar* charData = m_MemoryArena->allocate_array<stbtt_bakedchar>(96);
	stbtt_BakeFontBitmap(ttfData, 0, 40.0f, fontBitmap, 1024, 1024, 32, 96, charData);

	insigne::texture_desc_t texDesc;
	texDesc.width = 1024;
	texDesc.height = 1024;
	texDesc.format = insigne::texture_format_e::r;
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.dimension = insigne::texture_dimension_e::tex_2d;
	texDesc.has_mipmap = false;
	texDesc.data = fontBitmap;

	insigne::texture_handle_t tex = insigne::create_texture(texDesc);

	CLOVER_VERBOSE("Font loaded");
#endif
}

void Font::_OnUpdate(const f32 i_deltaMs)
{
	debugdraw::DrawText3D("12345", floral::vec3f(0.0f, 0.0f, 0.0f));
}

void Font::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Font::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
