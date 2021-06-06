#pragma once

#include <floral/stdaliases.h>
#include <floral/io/filesystem.h>
#include <floral/containers/fast_array.h>

#include <helich/alloc_schemes.h>
#include <helich/tracking_policies.h>
#include <helich/allocator.h>

#include <insigne/commons.h>

#include "Memory/MemorySystem.h"

#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/stb_truetype.h"
#include "Graphics/MaterialLoader.h"

namespace font_renderer
{
// ----------------------------------------------------------------------------

typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>     LinearAllocator;

typedef ssize                                   FontHandle;
typedef ssize                                   StaticTextHandle;

enum class Alignment
{
    Left,
    Center,
    Right
};

// ----------------------------------------------------------------------------

class FontRenderer
{
private:
    typedef helich::allocator<helich::stack_scheme, helich::no_tracking_policy>     LinearArena;
    typedef helich::allocator<helich::freelist_scheme, helich::no_tracking_policy>  FreelistArena;

    struct Glyph
    {
        s32                                     codePoint;
        s32                                     width, height;
        s32                                     advanceX, offsetX, offsetY;
        s32                                     aX, aY;
    };

    typedef floral::fast_fixed_array<Glyph, LinearArena>                    GlyphArray;

    struct Font
    {
        f32                                     fontSize;
        f32                                     scaleFactor;
        stbtt_fontinfo                          fontInfo;
        GlyphArray                              glyphs;

        s32                                     atlasSize;
        s32                                     padding;
        insigne::texture_handle_t               atlas;
        insigne::material_desc_t                material;
    };

    struct StaticText
    {
        const_cstr                              str;
        u32                                     hashValue;
        size                                    lenStr;
        floral::vec3f                           position;
        geo3d::GlyphVertex*                     drawData;
    };

    typedef floral::fast_fixed_array<Font, LinearArena>                       FontArray;
    typedef floral::fast_fixed_array<StaticText, LinearArena>                 StaticTextArray;

    struct StaticText2DRegistry
    {
        StaticTextArray                         db;
        bool                                    isDirty;
        geo3d::GlyphVertex*                     vertexData;
        u32*                                    indexData;
        insigne::vb_handle_t                    vb;
        insigne::ib_handle_t                    ib;
    };

    typedef floral::fast_fixed_array<StaticText2DRegistry, LinearArena>       DBStaticText2D;

public:
    FontRenderer(floral::filesystem<stone::FreelistArena>* i_fs);
    ~FontRenderer();

    void                                        Initialize();

    const FontHandle                            AddFont(const_cstr i_ttfName, const f32 i_size);

    StaticTextHandle                            AddText2D(const floral::vec2f& i_pos, const_cstr i_text, const FontHandle& i_fontHandle, const f32 i_size, const font_renderer::Alignment i_align);

    void                                        Render(const floral::mat4x4f& i_wvp);
    void                                        Render2D();

private:
    void                                        InternalBake2D();
    const size                                  InternalBakeString2D(const floral::vec3f& i_position, const_cstr i_str, const size& i_strLen, const size& i_fontIdx, geo3d::GlyphVertex* o_drawData);

private:
    floral::filesystem<stone::FreelistArena>*   m_FileSystem;
    floral::vec2f                               m_ScreenSize;

    FontArray                                   m_FontDB;
    DBStaticText2D                              m_DBStaticText2D;

    mat_loader::MaterialShaderPair              m_MSPair;
    mat_loader::MaterialShaderPair              m_MSPair2D;

private:
    LinearArena*                                m_FontArena;
    FreelistArena*                              m_TextArena;
    LinearArena*                                m_MaterialDataArena;
    LinearArena*                                m_DrawArena;
    FreelistArena*                              m_TempArena;
};

// ----------------------------------------------------------------------------
}
