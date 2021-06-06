#include "FontRenderer.h"

#include <floral/math/utils.h>

#include <clover/logger.h>

#include <calyx/context.h>

#include <insigne/ut_textures.h>
#include <insigne/ut_shading.h>

#include "Graphics/MaterialParser.h"

namespace font_renderer
{
// ----------------------------------------------------------------------------

extern LinearAllocator                          g_Allocator;

static const size                               k_charCount = 96;
static const size                               k_verticesLimit = 4096;
static const size                               k_indicesLimit = 8192;

// ----------------------------------------------------------------------------

inline size get_text_handle(size i_fid, size i_tid)
{
    return (i_fid << 16 | i_tid);
}

inline void split_text_handle(size i_hdl, size* o_fid, size* o_tid)
{
    *o_tid = i_hdl & 0xffff;
    *o_fid = (i_hdl >> 16) & 0xffff;
}

// ----------------------------------------------------------------------------

FontRenderer::FontRenderer(floral::filesystem<stone::FreelistArena>* i_fs)
    : m_FileSystem(i_fs)
{
}

// ----------------------------------------------------------------------------

FontRenderer::~FontRenderer()
{
}

// ----------------------------------------------------------------------------

void FontRenderer::Initialize()
{
    m_FontArena = g_Allocator.allocate_arena<LinearArena>(SIZE_MB(2));
    m_TextArena = g_Allocator.allocate_arena<FreelistArena>(SIZE_MB(2));
    m_MaterialDataArena = g_Allocator.allocate_arena<LinearArena>(SIZE_MB(1));
    m_DrawArena = g_Allocator.allocate_arena<LinearArena>(SIZE_MB(2));
    m_TempArena = g_Allocator.allocate_arena<FreelistArena>(SIZE_MB(2));

    m_FontDB.reserve(4, m_FontArena);
    m_DBStaticText2D.reserve(4, m_FontArena);

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
    m_ScreenSize = floral::vec2f(commonCtx->window_width, commonCtx->window_height);

    m_TempArena->free_all();
	floral::relative_path wdir = floral::build_relative_path("commons");
	floral::push_directory(m_FileSystem, wdir);
    floral::relative_path matPath = floral::build_relative_path("text3d.mat");
    mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_TempArena);
    bool createResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_TempArena, m_MaterialDataArena);
    FLORAL_ASSERT(createResult);

    matPath = floral::build_relative_path("text2d.mat");
    matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_TempArena);
    createResult = mat_loader::CreateMaterial(&m_MSPair2D, m_FileSystem, matDesc, m_TempArena, m_MaterialDataArena);
    FLORAL_ASSERT(createResult);
    floral::pop_directory(m_FileSystem);
}

const FontHandle FontRenderer::AddFont(const_cstr i_ttfName, const f32 i_size)
{
    // TODO: too many fonts?
    m_TempArena->free_all();

    floral::absolute_path iFilePath = floral::get_application_directory();
    floral::relative_path fontPath = floral::build_relative_path(i_ttfName);
    floral::concat_path(&iFilePath, fontPath);
    floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
    if (iFile.file_size <= 0)
    {
        CLOVER_ERROR("Cannot find font: '%s'", i_ttfName);
        return -1;
    }
    p8 ttfData = (p8)m_TempArena->allocate(iFile.file_size);
    floral::read_all_file(iFile, ttfData);
    floral::close_file(iFile);

    FontHandle newFontHandle = m_FontDB.get_size();
    m_FontDB.push_back(Font {});
    Font& newFont = m_FontDB[(size)newFontHandle];

    stbtt_InitFont(&newFont.fontInfo, ttfData, 0);
    newFont.fontSize = i_size;
    newFont.scaleFactor = stbtt_ScaleForPixelHeight(&newFont.fontInfo, i_size);
    newFont.glyphs.reserve(k_charCount, m_FontArena);

    s32 ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&newFont.fontInfo, &ascent, &descent, &lineGap);
    CLOVER_DEBUG("Font '%s': ascent %d; descent %d; lineGap %d", i_ttfName, ascent, descent, lineGap);

    floral::fast_fixed_array<p8, FreelistArena> glyphBitmaps(k_charCount, m_TempArena);
    for (size i = 0; i < k_charCount; i++)
    {
        s32 charWidth = 0, charHeight = 0;
        s32 ch = i + 32;

        s32 x0 = 0, x1 = 0, y0 = 0, y1 = 0;
        stbtt_GetCodepointBitmapBox(&newFont.fontInfo, ch, newFont.scaleFactor, newFont.scaleFactor, &x0, &y0, &x1, &y1);
        charWidth = x1 - x0;
        charHeight = y1 - y0;

        s32 advanceX = 0, offsetX = 0, offsetY = y0, leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&newFont.fontInfo, ch, &advanceX, &leftSideBearing);
        advanceX = (s32)(advanceX * newFont.scaleFactor);
        offsetX = (s32)(leftSideBearing * newFont.scaleFactor);
        offsetY += (s32)(ascent * newFont.scaleFactor);

        p8 bmData = nullptr;
        if (ch == 32)
        {
            charWidth = advanceX;
            charHeight = newFont.fontSize;
            size bitmapSize = charWidth * charHeight;
            bmData = (p8)m_TempArena->allocate(bitmapSize);
            memset(bmData, 0, bitmapSize);
        }
        else
        {
            size bitmapSize = charWidth * charHeight;
            bmData = (p8)m_TempArena->allocate(bitmapSize);
            stbtt_MakeCodepointBitmap(&newFont.fontInfo, bmData, charWidth, charHeight, charWidth, newFont.scaleFactor, newFont.scaleFactor, ch);
        }

        Glyph newGlyph {
            ch,
            charWidth, charHeight,
            advanceX, offsetX, offsetY,
            0, 0
        };

        glyphBitmaps.push_back(bmData);
        newFont.glyphs.push_back(newGlyph);
    }

    // pack into an atlas
    newFont.padding = 4;
    f32 requiredArea = 0.0f;
    for (size i = 0; i < k_charCount; i++)
    {
        requiredArea += (newFont.glyphs[i].width + 2 * newFont.padding) * (newFont.glyphs[i].height + 2 * newFont.padding);
    }
    f32 guessSize = sqrtf(requiredArea) * 1.3f;
    s32 imageSize = floral::next_pow2((s32)guessSize);
    newFont.atlasSize = imageSize;

    insigne::texture_desc_t texDesc;
    texDesc.width = imageSize;
    texDesc.height = imageSize;
    FLORAL_ASSERT(texDesc.width <= 2048 && texDesc.height <= 2048);
    texDesc.format = insigne::texture_format_e::r;
    texDesc.min_filter = insigne::filtering_e::linear;
    texDesc.mag_filter = insigne::filtering_e::linear;
    texDesc.dimension = insigne::texture_dimension_e::tex_2d;
    texDesc.has_mipmap = false;
    texDesc.compression = insigne::texture_compression_e::no_compression;
    texDesc.data = nullptr;
    texDesc.wrap_s = insigne::wrap_e::clamp_to_edge;
    texDesc.wrap_t = insigne::wrap_e::clamp_to_edge;

    insigne::prepare_texture_desc(texDesc);

    // create the atlas
    s32 offsetX = newFont.padding, offsetY = newFont.padding;
    p8 atlas = (p8)texDesc.data;
    for (size i = 0; i < k_charCount; i++)
    {
        Glyph& glyph = newFont.glyphs[i];
        p8 bmData = glyphBitmaps[i];
        glyph.aX = offsetX;
        glyph.aY = offsetY;
        // copy glyph's image to atlas
        for (s32 y = 0; y < glyph.height; y++)
        {
            for (s32 x = 0; x < glyph.width; x++)
            {
                atlas[(offsetY + y) * imageSize + offsetX + x] =
                    bmData[y * glyph.width + x];
            }
        }

        offsetX += (glyph.width + 2 * newFont.padding);
        if (offsetX >= (imageSize - glyph.width - 2 * newFont.padding))
        {
            offsetX = newFont.padding;
            offsetY += (newFont.fontSize + 2 * newFont.padding);
            if (offsetY > (imageSize - newFont.fontSize - 2 * newFont.padding))
            {
                break;
            }
        }
    }

    newFont.atlas = insigne::create_texture(texDesc);
    insigne::infuse_material(m_MSPair2D.shader, newFont.material);
    insigne::helpers::assign_texture(newFont.material, "u_FontAtlas", newFont.atlas);

    size rId = m_DBStaticText2D.get_size();
    m_DBStaticText2D.push_back(StaticText2DRegistry {});
    StaticText2DRegistry& newRegistry = m_DBStaticText2D[rId];
    newRegistry.db.reserve(1024, m_FontArena);
    newRegistry.isDirty = false;
    newRegistry.vertexData = m_DrawArena->allocate_array<geo3d::GlyphVertex>(k_verticesLimit);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_MB(2);
		desc.stride = sizeof(geo3d::GlyphVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		newRegistry.vb = insigne::create_vb(desc);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_MB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		newRegistry.ib = insigne::create_ib(desc);
	}

    newRegistry.indexData = m_DrawArena->allocate_array<u32>(k_indicesLimit);

    CLOVER_INFO("Font '%s' added", i_ttfName);
    return newFontHandle;
}

// ----------------------------------------------------------------------------

font_renderer::StaticTextHandle FontRenderer::AddText2D(const floral::vec2f& i_pos, const_cstr i_text, const FontHandle& i_fontHandle, const f32 i_size, const font_renderer::Alignment i_align)
{
    size txtLen = strlen(i_text);
    cstr txt = (cstr)m_TextArena->allocate(txtLen + 1);
    geo3d::GlyphVertex* drawData = (geo3d::GlyphVertex*)m_TextArena->allocate(sizeof(geo3d::GlyphVertex) * txtLen * 4);
    strcpy(txt, i_text);
    StaticText newText {
        txt,
        floral::compute_crc32_naive(txt),
        txtLen,
        floral::vec3f(i_pos.x, i_pos.y, 0.0f),
        drawData
    };

    size fId = (size)i_fontHandle;
    StaticText2DRegistry& registry = m_DBStaticText2D[fId];
    registry.isDirty = true;
    size textId = registry.db.get_size();
    registry.db.push_back(newText);

    return StaticTextHandle(get_text_handle(fId, textId));
}

// ----------------------------------------------------------------------------

void FontRenderer::Render(const floral::mat4x4f& i_wvp)
{
}

// ----------------------------------------------------------------------------

void FontRenderer::Render2D()
{
    InternalBake2D();

    size numDB = m_DBStaticText2D.get_size();
    for (size i = 0; i < numDB; i++)
    {
        const Font& font = m_FontDB[i];
        const StaticText2DRegistry& registry = m_DBStaticText2D[i];
        insigne::draw_surface<geo3d::GlyphSurface>(registry.vb, registry.ib, font.material);
    }
}

// ----------------------------------------------------------------------------

void FontRenderer::InternalBake2D()
{
    size numDB = m_DBStaticText2D.get_size();
    for (size i = 0; i < numDB; i++)
    {
        StaticText2DRegistry& registry = m_DBStaticText2D[i];
        if (registry.isDirty)
        {
            p8 vertexData = (p8)registry.vertexData;
            u32* indexData = registry.indexData;
            size numVertices = 0;
            size numIndices = 0;
            size numTexts = registry.db.get_size();
            for (size j = 0; j < numTexts; j++)
            {
                StaticText& txt = registry.db[j];
                const size quads = InternalBakeString2D(txt.position, txt.str, txt.lenStr, i, txt.drawData);
                memcpy(vertexData, txt.drawData, quads * 4 * sizeof(geo3d::GlyphVertex));
                for (size q = 0; q < quads; q++)
                {
                    indexData[0] = numVertices + 0;
                    indexData[1] = numVertices + 3;
                    indexData[2] = numVertices + 2;

                    indexData[3] = numVertices + 2;
                    indexData[4] = numVertices + 1;
                    indexData[5] = numVertices + 0;
                    numVertices += 4;
                    numIndices += 6;
                    indexData += 6;
                }
                vertexData += (quads * 4 * sizeof(geo3d::GlyphVertex));
            }

            insigne::update_vb(registry.vb, registry.vertexData, numVertices, 0);
            insigne::update_ib(registry.ib, registry.indexData, numIndices, 0);

            registry.isDirty = false;
        }
    }
}

// ----------------------------------------------------------------------------

const size FontRenderer::InternalBakeString2D(const floral::vec3f& i_position, const_cstr i_str, const size& i_strLen, const size& i_fontIdx, geo3d::GlyphVertex* o_drawData)
{
    const Font& font = m_FontDB[i_fontIdx];
    floral::vec3f position(i_position.x / m_ScreenSize.x * 2.0f - 1.0f, -i_position.y / m_ScreenSize.y * 2.0f + 1.0f, 0.0f);
    // TODO: remove hardcode
    floral::vec2f px = floral::vec2f(2.0f, 2.0f) / floral::vec2f(1280.0f, 720.0f);
    floral::vec2f dt = floral::vec2f(1.0f, 1.0f) / (f32)font.atlasSize;
    //floral::vec2f leftOrg(-strLenFrag * 0.5f * px.x, 0.0f);
    floral::vec2f leftOrg(0.0f, 0.0f);
    floral::vec4f i_color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);
    size validQuads = 0;

    geo3d::GlyphVertex* v = o_drawData;
    for (size i = 0; i < i_strLen; i++)
    {
        u8 ascii = i_str[i];
        const Glyph& glyphInfo = font.glyphs[ascii - 32];

        if (ascii != ' ')
        {
            const floral::vec2f s0(glyphInfo.aX * dt.x, glyphInfo.aY * dt.y);
            const floral::vec2f s1((glyphInfo.aX + glyphInfo.width) * dt.x, (glyphInfo.aY + glyphInfo.height) * dt.y);

            floral::vec2f dim = floral::vec2f(glyphInfo.width, glyphInfo.height) * px;
            floral::vec2f pos = leftOrg + floral::vec2f(glyphInfo.offsetX, -glyphInfo.offsetY) * px;
            v[0] = geo3d::GlyphVertex { position, floral::vec4f(pos.x, pos.y, s0.x, s0.y), i_color };
            v[1] = geo3d::GlyphVertex { position, floral::vec4f(pos.x + dim.x, pos.y, s1.x, s0.y), i_color };
            v[2] = geo3d::GlyphVertex { position, floral::vec4f(pos.x + dim.x, pos.y - dim.y, s1.x, s1.y), i_color };
            v[3] = geo3d::GlyphVertex { position, floral::vec4f(pos.x, pos.y - dim.y, s0.x, s1.y), i_color };
            v += 4;
            validQuads++;

            /*
            m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
            m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
            m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

            m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
            m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
            m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
            */
        }

        f32 advanceX = glyphInfo.advanceX;
        if (i < i_strLen - 1)
        {
            s32 kernX = stbtt_GetCodepointKernAdvance(&font.fontInfo, i_str[i], i_str[i + 1]);
            advanceX += font.scaleFactor * kernX;
        }

        leftOrg.x += advanceX * px.x;
    }
    return validQuads;
}

// ----------------------------------------------------------------------------
}
