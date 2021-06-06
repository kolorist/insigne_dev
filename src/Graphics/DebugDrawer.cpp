#include "DebugDrawer.h"

#include <math.h>

#include <floral/math/utils.h>

#include <insigne/system.h>
#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include <clover/Logger.h>

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Color;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec4 v_Color;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_Color = l_Color;
	highp vec4 pos = iu_WVP * pos_W;
	gl_Position = pos;
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es

layout (location = 0) out mediump vec4 o_Color;

in mediump vec4 v_Color;

void main()
{
	o_Color = v_Color;
}
)";

static const_cstr s_TextVS = R"(#version 300 es
layout (location = 0) in highp vec3 l_Position_L;
layout (location = 1) in mediump vec4 l_Corner;
layout (location = 2) in mediump vec4 l_Color;

layout(std140) uniform ub_XForm
{
	highp mat4 iu_WVP;
};

out mediump vec2 v_TexCoord;
out mediump vec4 v_Color;

void main() {
	v_Color = l_Color;
	v_TexCoord = l_Corner.zw;
	highp vec4 pos = iu_WVP * vec4(l_Position_L, 1.0f);
	gl_Position = pos / pos.w;
	gl_Position.xy += l_Corner.xy;
}
)";

static const_cstr s_TextFS = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

in mediump vec2 v_TexCoord;
in mediump vec4 v_Color;

uniform mediump sampler2D u_Tex;

void main()
{
	o_Color = mix(vec4(0.3f, 0.4f, 0.5f, 0.5f), v_Color, texture(u_Tex, v_TexCoord).r);
    //o_Color = vec4(v_Color.rgb, texture(u_Tex, v_TexCoord).r);
}
)";

const f32 k_fontSize = 40.0f;
const s32 k_padding = 4;
f32 k_scaleFactor = 1.0f;
floral::vec2f k_atlasSize(0.0f, 0.0f);
stbtt_fontinfo fontInfo;

DebugDrawer::DebugDrawer()
	: m_CurrentBufferIdx(0)
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(32));
}

DebugDrawer::~DebugDrawer()
{
}

// ---------------------------------------------
void DebugDrawer::DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color)
{
	u32 currentIdx = m_DebugVertices[m_CurrentBufferIdx].get_size();

	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { i_x0, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { i_x1, i_color } );
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
}

void DebugDrawer::DrawAABB3D(const floral::aabb3f& i_aabb, const floral::vec4f& i_color)
{
	floral::vec3f v0(i_aabb.min_corner);
	floral::vec3f v6(i_aabb.max_corner);
	floral::vec3f v1(v6.x, v0.y, v0.z);
	floral::vec3f v2(v6.x, v0.y, v6.z);
	floral::vec3f v3(v0.x, v0.y, v6.z);
	floral::vec3f v4(v0.x, v6.y, v0.z);
	floral::vec3f v5(v6.x, v6.y, v0.z);
	floral::vec3f v7(v0.x, v6.y, v6.z);

	u32 currentIdx = m_DebugVertices[m_CurrentBufferIdx].get_size();

	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v0, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v1, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v2, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v3, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v4, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v5, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v6, i_color } );
	m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { v7, i_color } );

	// bottom
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	// top
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	// side
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
}

void DebugDrawer::DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1,
		const floral::vec3f& i_p2, const floral::vec3f& i_p3,
		const floral::vec4f& i_color)
{
	DrawLine3D(i_p0, i_p1, i_color);
	DrawLine3D(i_p1, i_p2, i_color);
	DrawLine3D(i_p2, i_p3, i_color);
	DrawLine3D(i_p3, i_p0, i_color);
}

void DebugDrawer::DrawIcosahedron3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color)
{
	static const floral::vec3f s_icosahedronVertices[12] = {
		floral::vec3f(-0.525731f, 0, 0.850651f), floral::vec3f(0.525731f, 0, 0.850651f),
		floral::vec3f(-0.525731f, 0, -0.850651f), floral::vec3f(0.525731f, 0, -0.850651f),
		floral::vec3f(0, 0.850651f, 0.525731f), floral::vec3f(0, 0.850651f, -0.525731f),
		floral::vec3f(0, -0.850651f, 0.525731f), floral::vec3f(0, -0.850651f, -0.525731f),
		floral::vec3f(0.850651f, 0.525731f, 0), floral::vec3f(-0.850651f, 0.525731f, 0),
		floral::vec3f(0.850651f, -0.525731f, 0), floral::vec3f(-0.850651f, -0.525731f, 0)
	};

	static const u32 s_icosahedronIndices[120] = {
		1,4,4,0,0,1,  4,9,9,0,0,4,  4,5,5,9,9,4,  8,5,5,4,4,8,  1,8,8,4,4,1,
		1,10,10,8,8,1, 10,3,3,8,8,10, 8,3,3,5,5,8,  3,2,2,5,5,3,  3,7,7,2,2,3,
		3,10,10,7,7,3, 10,6,6,7,7,10, 6,11,11,7,7,6, 6,0,0,11,11,6, 6,1,1,0,0,6,
		10,1,1,6,6,10, 11,0,0,9,9,11, 2,11,11,9,9,2, 5,2,2,9,9,5,  11,2,2,7,7,11
	};

	u32 idx = m_DebugVertices[m_CurrentBufferIdx].get_size();
	for (u32 i = 0; i < 12; i++)
		m_DebugVertices[m_CurrentBufferIdx].push_back(VertexPC { s_icosahedronVertices[i] * i_radius + i_origin, i_color });
	for (u32 i = 0; i < 120; i++)
		m_DebugIndices[m_CurrentBufferIdx].push_back(idx + s_icosahedronIndices[i]);
}

void DebugDrawer::DrawIcosphere3D(const floral::vec3f& i_origin, const f32 i_radius, const floral::vec4f& i_color)
{
}

// ---------------------------------------------

void DebugDrawer::DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const floral::vec4f& i_color)
{
	floral::vec3f minCorner = i_position - floral::vec3f(i_size / 2.0f);
	floral::vec3f maxCorner = i_position + floral::vec3f(i_size / 2.0f);
	DrawSolidBox3D(minCorner, maxCorner, i_color);
}

void DebugDrawer::DrawSolidBox3D(const floral::vec3f& i_minCorner, const floral::vec3f& i_maxCorner, const floral::vec4f& i_color)
{
	floral::vec3f v2(i_minCorner);
	floral::vec3f v4(i_maxCorner);
	floral::vec3f v0(v4.x, v2.y, v4.z);
	floral::vec3f v1(v4.x, v2.y, v2.z);
	floral::vec3f v3(v2.x, v2.y, v4.z);
	floral::vec3f v5(v4.x, v4.y, v2.z);
	floral::vec3f v6(v2.x, v4.y, v2.z);
	floral::vec3f v7(v2.x, v4.y, v4.z);

	u32 currentIdx = m_DebugSurfaceVertices[m_CurrentBufferIdx].get_size();

	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v0, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v1, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v2, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v3, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v4, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v5, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v6, i_color } );
	m_DebugSurfaceVertices[m_CurrentBufferIdx].push_back(VertexPC { v7, i_color } );

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 6);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 5);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);

	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 4);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 7);
	m_DebugSurfaceIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
}

void DebugDrawer::DrawText3D(const_cstr i_str, const floral::vec3f& i_position)
{
	s32 len = strlen(i_str);
	f32 strLenFrag = 0;
	for (s32 i = 0; i < len; i++)
	{
		u8 ascii = i_str[i];
		strLenFrag += m_Glyphs[ascii - 32].advanceX;
        if (i < len - 1)
        {
            s32 kernX = stbtt_GetCodepointKernAdvance(&fontInfo, i_str[i], i_str[i + 1]);
            strLenFrag += k_scaleFactor * kernX;
        }
	}

	// TODO: remove hardcode
	floral::vec2f px = floral::vec2f(2.0f, 2.0f) / floral::vec2f(1280.0f, 720.0f);
	floral::vec2f dt = floral::vec2f(1.0f, 1.0f) / k_atlasSize;
	floral::vec2f leftOrg(-strLenFrag * 0.5f * px.x, 0.0f);
	floral::vec4f i_color = floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f);

	for (s32 i = 0; i < len; i++)
	{
		u8 ascii = i_str[i];
		const GlyphInfo& charInfo = m_Glyphs[ascii - 32];

		if (ascii != ' ')
		{
			const floral::vec2f s0(charInfo.aX * dt.x, charInfo.aY * dt.y);
			const floral::vec2f s1((charInfo.aX + charInfo.width) * dt.x, (charInfo.aY + charInfo.height) * dt.y);

			DebugTextVertex v[4];
			floral::vec2f dim = floral::vec2f(charInfo.width, charInfo.height) * px;
			floral::vec2f pos = leftOrg + floral::vec2f(charInfo.offsetX, -charInfo.offsetY) * px;
			v[0] = DebugTextVertex { i_position, floral::vec4f(pos.x, pos.y, s0.x, s0.y), i_color };
			v[1] = DebugTextVertex { i_position, floral::vec4f(pos.x + dim.x, pos.y, s1.x, s0.y), i_color };
			v[2] = DebugTextVertex { i_position, floral::vec4f(pos.x + dim.x, pos.y - dim.y, s1.x, s1.y), i_color };
			v[3] = DebugTextVertex { i_position, floral::vec4f(pos.x, pos.y - dim.y, s0.x, s1.y), i_color };

			u32 currentIdx = m_DebugTextVertices[m_CurrentBufferIdx].get_size();
			m_DebugTextVertices[m_CurrentBufferIdx].push_back(v[0]);
			m_DebugTextVertices[m_CurrentBufferIdx].push_back(v[1]);
			m_DebugTextVertices[m_CurrentBufferIdx].push_back(v[2]);
			m_DebugTextVertices[m_CurrentBufferIdx].push_back(v[3]);

			m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
			m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 3);
			m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);

			m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 2);
			m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 1);
			m_DebugTextIndices[m_CurrentBufferIdx].push_back(currentIdx + 0);
		}

        f32 advanceX = charInfo.advanceX;
        if (i < len - 1)
        {
            s32 kernX = stbtt_GetCodepointKernAdvance(&fontInfo, i_str[i], i_str[i + 1]);
            advanceX += k_scaleFactor * kernX;
        }
		leftOrg.x += advanceX * px.x;
	}
}

// ---------------------------------------------

void DebugDrawer::Initialize(floral::filesystem<FreelistArena>* i_fs)
{
	CLOVER_VERBOSE("Initializing DebugDrawer");
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	LinearArena* tempArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	tempArena->free_all();
	floral::absolute_path iFilePath = floral::get_application_directory();
	//floral::relative_path fontPath = floral::build_relative_path("LiberationMono-Regular.ttf");
	floral::relative_path fontPath = floral::build_relative_path("arial.ttf");
	floral::concat_path(&iFilePath, fontPath);
	floral::file_info iFile = floral::open_file_read(i_fs, iFilePath);
	if (iFile.file_size <= 0)
	{
		CLOVER_ERROR("No font");
	}
	p8 ttfData = (p8)tempArena->allocate(iFile.file_size);
	floral::read_all_file(iFile, ttfData);
	floral::close_file(iFile);

    // font bitmap creation
	#if 0
	insigne::texture_desc_t texDesc;
    {
        const size k_charCount = 96;

        m_Glyphs.init(k_charCount, m_MemoryArena);

        FLORAL_ASSERT(stbtt_InitFont(&fontInfo, ttfData, 0));
        k_scaleFactor = stbtt_ScaleForPixelHeight(&fontInfo, k_fontSize);
        s32 kerningTableLen = stbtt_GetKerningTableLength(&fontInfo);
        stbtt_kerningentry* kerningTable = m_MemoryArena->allocate_array<stbtt_kerningentry>(kerningTableLen);
        fontInfo.gpos = 0;
        stbtt_GetKerningTable(&fontInfo, kerningTable, kerningTableLen);

        s32 ascent = 0, descent = 0, lineGap = 0;
        stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

        for (size i = 0; i < k_charCount; i++)
        {
            s32 charWidth = 0, charHeight = 0;
            s32 ch = i + 32;

            s32 x0 = 0, x1 = 0, y0 = 0, y1 = 0;
            stbtt_GetCodepointBitmapBox(&fontInfo, ch, k_scaleFactor, k_scaleFactor, &x0, &y0, &x1, &y1);
            charWidth = x1 - x0;
            charHeight = y1 - y0;

            s32 advanceX = 0, offsetX = 0, offsetY = y0, leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&fontInfo, ch, &advanceX, &leftSideBearing);
            advanceX = (s32)(advanceX * k_scaleFactor);
            offsetX = (s32)(leftSideBearing * k_scaleFactor);
            offsetY += (s32)(ascent * k_scaleFactor);

            p8 bmData = nullptr;
            if (ch == 32)
            {
                charWidth = advanceX;
                charHeight = k_fontSize;
                size bitmapSize = charWidth * charHeight;
                bmData = (p8)tempArena->allocate(bitmapSize);
                memset(bmData, 0, bitmapSize);
            }
            else
            {
                size bitmapSize = charWidth * charHeight;
                bmData = (p8)tempArena->allocate(bitmapSize);
                stbtt_MakeCodepointBitmap(&fontInfo, bmData, charWidth, charHeight, charWidth, k_scaleFactor, k_scaleFactor, ch);
            }

            GlyphInfo newGlyph {
                ch,
                charWidth, charHeight,
                advanceX, offsetX, offsetY,
                0, 0,
                bmData
            };

            m_Glyphs.push_back(newGlyph);
        }

        // pack into an atlas
        f32 requiredArea = 0.0f;
        for (size i = 0; i < k_charCount; i++)
        {
            requiredArea += (m_Glyphs[i].width + 2 * k_padding) * (m_Glyphs[i].height + 2 * k_padding);
        }
        f32 guessSize = sqrtf(requiredArea) * 1.3f;
        s32 imageSize = floral::next_pow2((s32)guessSize);
		
        k_atlasSize = floral::vec2f(imageSize, imageSize);
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

        insigne::prepare_texture_desc(texDesc);

        // create the atlas
        s32 offsetX = k_padding, offsetY = k_padding;
        p8 atlas = (p8)texDesc.data;
        for (size i = 0; i < k_charCount; i++)
        {
            GlyphInfo& glyph = m_Glyphs[i];
            glyph.aX = offsetX;
            glyph.aY = offsetY;
            // copy glyph's image to atlas
            for (s32 y = 0; y < glyph.height; y++)
            {
                for (s32 x = 0; x < glyph.width; x++)
                {
                    atlas[(offsetY + y) * imageSize + offsetX + x] =
                        glyph.bmData[y * glyph.width + x];
                }
            }

            offsetX += (glyph.width + 2 * k_padding);
            if (offsetX >= (imageSize - glyph.width - 2 * k_padding))
            {
                offsetX = k_padding;
                offsetY += (k_fontSize + 2 * k_padding);
                if (offsetY > (imageSize - k_fontSize - 2 * k_padding))
                {
                    break;
                }
            }
        }
    }

	//m_CharacterData = m_MemoryArena->allocate_array<stbtt_bakedchar>(96);
	//stbtt_BakeFontBitmap(ttfData, 0, 40.0f, (p8)texDesc.data, 1024, 1024, 32, 96, m_CharacterData);

	m_FontAtlas = insigne::create_texture(texDesc);
	#endif
	// DebugLine and DebugSurface has already been registered
	static const u32 s_verticesLimit = 1u << 17;
	static const s32 s_indicesLimit = 1u << 18;
	m_DebugVertices[0].init(s_verticesLimit, m_MemoryArena);
	m_DebugVertices[1].init(s_verticesLimit, m_MemoryArena);
	m_DebugIndices[0].init(s_indicesLimit, m_MemoryArena);
	m_DebugIndices[1].init(s_indicesLimit, m_MemoryArena);
	m_DebugSurfaceVertices[0].init(s_verticesLimit, m_MemoryArena);
	m_DebugSurfaceVertices[1].init(s_verticesLimit, m_MemoryArena);
	m_DebugSurfaceIndices[0].init(s_indicesLimit, m_MemoryArena);
	m_DebugSurfaceIndices[1].init(s_indicesLimit, m_MemoryArena);
	m_DebugTextVertices[0].init(s_verticesLimit, m_MemoryArena);
	m_DebugTextVertices[1].init(s_verticesLimit, m_MemoryArena);
	m_DebugTextIndices[0].init(s_indicesLimit, m_MemoryArena);
	m_DebugTextIndices[1].init(s_indicesLimit, m_MemoryArena);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_MB(32);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_VB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_MB(16);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_IB = newIB;
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_MB(16);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_SurfaceVB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_MB(8);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_SurfaceIB = newIB;
	}

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_MB(16);
		desc.stride = sizeof(DebugTextVertex);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		m_TextVB = newVB;
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_MB(8);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		m_TextIB = newIB;
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ub_handle_t newUB = insigne::create_ub(desc);

		m_UB = newUB;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/internal/debug_draw_vs");
		desc.fs_path = floral::path("/internal/debug_draw_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		s32 ubSlot = insigne::get_material_uniform_block_slot(m_Material, "ub_XForm");
		m_Material.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_UB };
		m_Material.render_state.depth_write = false;
		m_Material.render_state.depth_test = true;
		m_Material.render_state.depth_func = insigne::compare_func_e::func_less_or_equal;
		m_Material.render_state.cull_face = false;
		m_Material.render_state.blending = true;
		m_Material.render_state.blend_equation = insigne::blend_equation_e::func_add;
		m_Material.render_state.blend_func_sfactor = insigne::factor_e::fact_src_alpha;
		m_Material.render_state.blend_func_dfactor = insigne::factor_e::fact_one_minus_src_alpha;
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_XForm", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_TextVS);
		strcpy(desc.fs, s_TextFS);
		desc.vs_path = floral::path("/internal/debug_text_draw_vs");
		desc.fs_path = floral::path("/internal/debug_text_draw_fs");

		m_TextShader = insigne::create_shader(desc);
		insigne::infuse_material(m_TextShader, m_TextMaterial);

		insigne::helpers::assign_uniform_block(m_TextMaterial, "ub_XForm", 0, 0, m_UB);
		insigne::helpers::assign_texture(m_TextMaterial, "u_Tex", m_FontAtlas);

		m_TextMaterial.render_state.depth_write = false;
		m_TextMaterial.render_state.depth_test = false;
		m_TextMaterial.render_state.cull_face = true;
		m_TextMaterial.render_state.blending = true;
		m_TextMaterial.render_state.blend_equation = insigne::blend_equation_e::func_add;
		m_TextMaterial.render_state.blend_func_sfactor = insigne::factor_e::fact_src_alpha;
		m_TextMaterial.render_state.blend_func_dfactor = insigne::factor_e::fact_one_minus_src_alpha;
	}
	// flush the initialization pass
	insigne::dispatch_render_pass();
	g_StreammingAllocator.free(tempArena);
}

void DebugDrawer::CleanUp()
{
	CLOVER_VERBOSE("Cleaning up DebugDrawer");

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

void DebugDrawer::Render(const floral::mat4x4f& i_wvp)
{
	m_Data[m_CurrentBufferIdx].WVP = i_wvp;
	insigne::update_ub(m_UB, &m_Data[m_CurrentBufferIdx], sizeof(MyData), 0);

	if (m_DebugIndices[m_CurrentBufferIdx].get_size() > 0)
	{
		insigne::draw_surface<DebugLine>(m_VB, m_IB, m_Material);
	}

	if (m_DebugSurfaceIndices[m_CurrentBufferIdx].get_size() > 0)
	{
		insigne::draw_surface<DebugSurface>(m_SurfaceVB, m_SurfaceIB, m_Material);
	}

#if 0
	if (m_DebugTextIndices[m_CurrentBufferIdx].get_size() > 0)
	{
		insigne::draw_surface<DebugTextSurface>(m_TextVB, m_TextIB, m_TextMaterial);
	}
#endif
}

void DebugDrawer::BeginFrame()
{
	m_CurrentBufferIdx = (m_CurrentBufferIdx + 1) % 2;
	m_DebugVertices[m_CurrentBufferIdx].empty();
	m_DebugIndices[m_CurrentBufferIdx].empty();
	m_DebugSurfaceVertices[m_CurrentBufferIdx].empty();
	m_DebugSurfaceIndices[m_CurrentBufferIdx].empty();
	m_DebugTextVertices[m_CurrentBufferIdx].empty();
	m_DebugTextIndices[m_CurrentBufferIdx].empty();
}

void DebugDrawer::EndFrame()
{
	if (m_DebugVertices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_vb(m_VB, &(m_DebugVertices[m_CurrentBufferIdx][0]), m_DebugVertices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_vb(m_VB, nullptr, 0, 0);

	if (m_DebugIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_ib(m_IB, &(m_DebugIndices[m_CurrentBufferIdx][0]), m_DebugIndices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_ib(m_IB, nullptr, 0, 0);

	if (m_DebugSurfaceVertices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_vb(m_SurfaceVB, &(m_DebugSurfaceVertices[m_CurrentBufferIdx][0]), m_DebugSurfaceVertices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_vb(m_SurfaceVB, nullptr, 0, 0);

	if (m_DebugSurfaceIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_ib(m_SurfaceIB, &(m_DebugSurfaceIndices[m_CurrentBufferIdx][0]), m_DebugSurfaceIndices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_ib(m_SurfaceIB, nullptr, 0, 0);

	if (m_DebugTextVertices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_vb(m_TextVB, &(m_DebugTextVertices[m_CurrentBufferIdx][0]), m_DebugTextVertices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_vb(m_TextVB, nullptr, 0, 0);

	if (m_DebugTextIndices[m_CurrentBufferIdx].get_size() > 0)
		insigne::update_ib(m_TextIB, &(m_DebugTextIndices[m_CurrentBufferIdx][0]), m_DebugTextIndices[m_CurrentBufferIdx].get_size(), 0);
	else insigne::update_ib(m_TextIB, nullptr, 0, 0);
}

//----------------------------------------------
static DebugDrawer* s_DebugDrawer;

static const floral::vec4f k_Colors[] = {
	floral::vec4f(0.0f, 0.0f, 0.5f, 1.0f),		// blue
	floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f),		// bright blue
	floral::vec4f(0.5f, 0.0f, 0.0f, 1.0f),		// red
	floral::vec4f(0.5f, 0.0f, 0.5f, 1.0f),		// magenta
	floral::vec4f(0.5f, 0.0f, 1.0f, 1.0f),		// violet
	floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f),		// bright red
	floral::vec4f(1.0f, 0.0f, 0.5f, 1.0f),		// purple
	floral::vec4f(1.0f, 0.0f, 1.0f, 1.0f),		// bright magenta
	floral::vec4f(0.0f, 0.5f, 0.0f, 1.0f),		// green
	floral::vec4f(0.0f, 0.5f, 0.5f, 1.0f),		// cyan
	floral::vec4f(0.0f, 0.5f, 1.0f, 1.0f),		// sky blue
	floral::vec4f(0.5f, 0.5f, 0.0f, 1.0f),		// yellow
	floral::vec4f(0.5f, 0.5f, 0.5f, 1.0f),		// gray
	floral::vec4f(0.5f, 0.5f, 1.0f, 1.0f),		// pale blue
	floral::vec4f(1.0f, 0.5f, 0.0f, 1.0f),		// orange
	floral::vec4f(1.0f, 0.5f, 0.5f, 1.0f),		// pink
	floral::vec4f(1.0f, 0.5f, 1.0f, 1.0f),		// pale magenta
	floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f),		// bright green
	floral::vec4f(0.0f, 1.0f, 0.5f, 1.0f),		// sea green
	floral::vec4f(0.0f, 1.0f, 1.0f, 1.0f),		// bright cyan
	floral::vec4f(0.5f, 1.0f, 0.0f, 1.0f),		// lime green
	floral::vec4f(0.5f, 1.0f, 0.5f, 1.0f),		// pale green
	floral::vec4f(0.5f, 1.0f, 1.0f, 1.0f),		// pale cyan
	floral::vec4f(1.0f, 1.0f, 0.0f, 1.0f),		// bright yellow
	floral::vec4f(1.0f, 1.0f, 0.5f, 1.0f),		// pale yellow
};
static const size k_ColorsCount = 25;

namespace debugdraw
{

void Initialize(floral::filesystem<FreelistArena>* i_fs)
{
	FLORAL_ASSERT(s_DebugDrawer == nullptr);
	s_DebugDrawer = g_PersistanceResourceAllocator.allocate<DebugDrawer>();
	s_DebugDrawer->Initialize(i_fs);
}

void CleanUp()
{
	s_DebugDrawer->CleanUp();
}

void BeginFrame()
{
	s_DebugDrawer->BeginFrame();
}

void EndFrame()
{
	s_DebugDrawer->EndFrame();
}

void Render(const floral::mat4x4f& i_wvp)
{
	s_DebugDrawer->Render(i_wvp);
}

void DrawLine3D(const floral::vec3f& i_x0, const floral::vec3f& i_x1, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawLine3D(i_x0, i_x1, i_color);
}

void DrawQuad3D(const floral::vec3f& i_p0, const floral::vec3f& i_p1, const floral::vec3f& i_p2, const floral::vec3f& i_p3, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawQuad3D(i_p0, i_p1, i_p2, i_p3, i_color);
}

void DrawAABB3D(const floral::aabb3f& i_aabb, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawAABB3D(i_aabb, i_color);
}

void DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const floral::vec4f& i_color)
{
	s_DebugDrawer->DrawPoint3D(i_position, i_size, i_color);
}

void DrawPoint3D(const floral::vec3f& i_position, const f32 i_size, const size i_colorIdx)
{
	s_DebugDrawer->DrawPoint3D(i_position, i_size, k_Colors[i_colorIdx % k_ColorsCount]);
}

void DrawText3D(const_cstr i_str, const floral::vec3f& i_position)
{
	//s_DebugDrawer->DrawText3D(i_str, i_position);
}

}

}
