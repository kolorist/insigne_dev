#include "Font.h"

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/FontRenderer.h"

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

    // add fonts
    const font_renderer::FontHandle arialFont = m_FontRenderer->AddFont("arial.ttf", 50);
    // font_renderer::FontHandle someFont = m_FontRenderer->AddFont("some_font.ttf", 50);

    // static texts
    font_renderer::StaticTextHandle static0 = m_FontRenderer->AddText2D(floral::vec2f(400, 400), "Some Text. WAR", arialFont, 30, font_renderer::Alignment::Left);
    font_renderer::StaticTextHandle static1 = m_FontRenderer->AddText2D(floral::vec2f(400, 200), "Hello World", arialFont, 30, font_renderer::Alignment::Left);
    // font_renderer::StaticTextHandle static1 = m_FontRenderer->AddText3D(floral::vec3f(0.0f, 0.0f, 0.0f), "Some Text 3D", 50, font_renderer::Alignment::Left);
    // font_renderer::SetText(static0, "Some Text"); // => hash checking then doing nothing
    // font_renderer::SetText(static0, "Some Other Text"); // => update

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
    // dynamic text
    // m_FontRenderer->DrawText2D(floral::vec2f(0, 0), "FPS: 12", 20, font_renderer::Alignment::Left);
    // m_FontRenderer->DrawText3D(floral::vec3f(0, 0, 0), "FPS: 12", 30, font_renderer::Alignment::Left);
	debugdraw::DrawText3D("12345", floral::vec3f(0.0f, 0.0f, 0.0f));
}

void Font::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

    m_FontRenderer->Render(floral::mat4x4f(1.0f));
    m_FontRenderer->Render2D();
	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Font::_OnCleanUp()
{
    // cleant up font_renderer
    // font_renderer::CleanUp();

	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
