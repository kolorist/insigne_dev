#include "Quad2DRasterize.h"

#include <floral/io/nativeio.h>

#include <calyx/context.h>

#include <clover.h>

#include <insigne/commons.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_render.h>
#include <insigne/ut_textures.h>

#include <algorithm>

namespace stone
{

static const_cstr s_VertexShader = R"(#version 300 es
layout (location = 0) in highp vec2 l_Position;
layout (location = 1) in mediump vec2 l_TexCoord;

out mediump vec2 v_TexCoord;

void main()
{
	v_TexCoord = l_TexCoord;
	gl_Position = vec4(l_Position, 0.0f, 1.0f);
}
)";

static const_cstr s_FragmentShader = R"(#version 300 es
layout (location = 0) out mediump vec4 o_Color;

uniform mediump sampler2D u_Tex;

in mediump vec2 v_TexCoord;

void main()
{
	o_Color = vec4(texture(u_Tex, v_TexCoord).rgb, 1.0f);
}
)";

//----------------------------------------------
// CW
struct SimpleVertex
{
	floral::vec2i Position;
	floral::vec3f Color;
};

s32 Orient2D(const floral::vec2i& a, const floral::vec2i& b, const floral::vec2i& c)
{
	return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

s32 Min3i(s32 a, s32 b, s32 c)
{
	return std::min(a, std::min(b, c));
}

s32 Max3i(s32 a, s32 b, s32 c)
{
	return std::max(a, std::max(b, c));
}

const bool IsTopEdge(const floral::vec2i& a, const floral::vec2i& b)
{
	return (a.y == b.y) && (a.x < b.x);
}

const bool IsLeftEdge(const floral::vec2i& a, const floral::vec2i& b)
{
	return (a.y > b.y);
}

const bool IsTopOrLeftEdge(const floral::vec2i& a, const floral::vec2i& b)
{
	return IsTopEdge(a, b) || IsLeftEdge(a, b);
}

void RasterizeTriangle2D(SimpleVertex i_tri[], const s32 i_du, const s32 i_dv, f32* o_toBuffer)
{
	// triangle bounding box
	s32 minX = Min3i(i_tri[0].Position.x, i_tri[1].Position.x, i_tri[2].Position.x);
	s32 minY = Min3i(i_tri[0].Position.y, i_tri[1].Position.y, i_tri[2].Position.y);
	s32 maxX = Max3i(i_tri[0].Position.x, i_tri[1].Position.x, i_tri[2].Position.x);
	s32 maxY = Max3i(i_tri[0].Position.y, i_tri[1].Position.y, i_tri[2].Position.y);

	// clip against buffer bound
	minX = std::max(minX, 0);
	minY = std::max(minY, 0);
	maxX = std::min(maxX, i_du - 1);
	maxY = std::min(maxY, i_dv - 1);

	// rasterize
	floral::vec2i p;
	s32 w = Orient2D(i_tri[0].Position, i_tri[1].Position, i_tri[2].Position);
	for (p.y = minY; p.y < maxY; p.y++)
	{
		for (p.x = minX; p.x < maxX; p.x++)
		{
			// fill rule
			s32 bias0 = IsTopOrLeftEdge(i_tri[1].Position, i_tri[2].Position) ? 0 : -1;
			s32 bias1 = IsTopOrLeftEdge(i_tri[2].Position, i_tri[0].Position) ? 0 : -1;
			s32 bias2 = IsTopOrLeftEdge(i_tri[0].Position, i_tri[1].Position) ? 0 : -1;
			// compute baricentric coordinates
			s32 w0 = Orient2D(i_tri[1].Position, i_tri[2].Position, p) + bias0;
			s32 w1 = Orient2D(i_tri[2].Position, i_tri[0].Position, p) + bias1;
			s32 w2 = Orient2D(i_tri[0].Position, i_tri[1].Position, p) + bias2;
			if (w0 >= 0 && w1 >= 0 && w2 >= 0)
			{
				FLORAL_ASSERT(w0 + w1 + w2 <= w);
				f32 b0 = (f32)w0 / (f32)w;
				f32 b1 = (f32)w1 / (f32)w;
				f32 b2 = 1.0f - b0 - b1;
				floral::vec3f c = b0 * i_tri[0].Color + b1 * i_tri[1].Color + b2 * i_tri[2].Color;
				ssize pid = (p.y * i_du + p.x) * 3;
				o_toBuffer[pid] = c.x;
				o_toBuffer[pid + 1] = c.y;
				o_toBuffer[pid + 2] = c.z;
			}
		}
	}
}

void RasterizeQuad2D(SimpleVertex i_quad[], const s32 i_du, const s32 i_dv, p8 o_toBuffer)
{
	for (ssize u = 0; u < i_du; u++)
	{
		for (ssize v = 0; v < i_dv; v++)
		{
			//f32 t = (f32)u / (f32)i_du;
			f32 t = (f32)v / (f32)i_dv;
			u8 c = t * 255;
			ssize pid = (v * i_du + u) * 3;
			o_toBuffer[pid] = c;
			o_toBuffer[pid + 1] = 0;
			o_toBuffer[pid + 2] = 0;
		}
	}
}

//----------------------------------------------

Quad2DRasterize::Quad2DRasterize()
{
	m_MemoryArena = g_PersistanceResourceAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
}

Quad2DRasterize::~Quad2DRasterize()
{
}

void Quad2DRasterize::OnInitialize()
{
	{
		VertexPT vertices[4];
		vertices[0].Position = floral::vec2f(-1.0f, -1.0f);
		vertices[0].TexCoord = floral::vec2f(0.0f, 0.0f);

		vertices[1].Position = floral::vec2f(1.0f, -1.0f);
		vertices[1].TexCoord = floral::vec2f(1.0f, 0.0f);

		vertices[2].Position = floral::vec2f(1.0f, 1.0f);
		vertices[2].TexCoord = floral::vec2f(1.0f, 1.0f);

		vertices[3].Position = floral::vec2f(-1.0f, 1.0f);
		vertices[3].TexCoord = floral::vec2f(0.0f, 1.0f);

		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.stride = sizeof(VertexPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::vb_handle_t newVB = insigne::create_vb(desc);
		insigne::copy_update_vb(newVB, vertices, 4, sizeof(VertexPT), 0);
		m_VB = newVB;
	}

	{
		u32 indices[6];
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 2;
		indices[3] = 2;
		indices[4] = 3;
		indices[5] = 0;

		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(4);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		insigne::ib_handle_t newIB = insigne::create_ib(desc);
		insigne::copy_update_ib(newIB, indices, 6, 0);
		m_IB = newIB;
	}

	// upload texture
	{
		const s32 bufferDim = 512;
		insigne::texture_desc_t texDesc;
		texDesc.width = bufferDim;
		texDesc.height = bufferDim;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::linear;
		texDesc.mag_filter = insigne::filtering_e::linear;
		texDesc.dimension = insigne::texture_dimension_e::tex_2d;
		texDesc.has_mipmap = false;
		const size dataSize = insigne::prepare_texture_desc(texDesc);
		f32* pData = (f32*)texDesc.data;

		memset(pData, 0, dataSize);

#if 0
		SimpleVertex vtx[6];
		vtx[0].Position = floral::vec2i(0, 0);
		vtx[1].Position = floral::vec2i(512, 0);
		vtx[2].Position = floral::vec2i(512, 512);
		vtx[3].Position = floral::vec2i(512, 512);
		vtx[4].Position = floral::vec2i(0, 512);
		vtx[5].Position = floral::vec2i(0, 0);

		vtx[0].Color = floral::vec3f(0.0f, 0.0f, 0.0f);
		vtx[1].Color = floral::vec3f(0.0f, 0.0f, 0.0f);
		vtx[2].Color = floral::vec3f(1.0f, 0.0f, 0.0f);
		vtx[3].Color = floral::vec3f(1.0f, 0.0f, 0.0f);
		vtx[4].Color = floral::vec3f(0.0f, 0.0f, 0.0f);
		vtx[5].Color = floral::vec3f(0.0f, 0.0f, 0.0f);

		RasterizeTriangle2D(&vtx[0], bufferDim, bufferDim, pData);
		RasterizeTriangle2D(&vtx[3], bufferDim, bufferDim, pData);
#else
		floral::file_info rf = floral::open_file("patches.dat");
		floral::file_stream is;
		is.buffer = (p8)m_MemoryArena->allocate(rf.file_size);
		floral::read_all_file(rf, is);

		ssize verticesCount = 0;
		is.read(&verticesCount);
		floral::fixed_array<floral::vec2i, LinearArena> texVtx;
		texVtx.reserve(verticesCount, m_MemoryArena);
		for (ssize i = 0; i < verticesCount; i++)
		{
			floral::vec2i pv;
			is.read(&pv);
			texVtx.push_back(pv);
		}
		ssize indicesCount = 0;
		is.read(&indicesCount);
		floral::fixed_array<s32, LinearArena> texIdx;
		texIdx.reserve(indicesCount, m_MemoryArena);
		floral::fixed_array<floral::vec3f, LinearArena> patchColor;
		patchColor.reserve(indicesCount / 4, m_MemoryArena);
		for (ssize i = 0; i < indicesCount; i += 4)
		{
			s32 idx[4];
			is.read(&idx[0]);
			is.read(&idx[1]);
			is.read(&idx[2]);
			is.read(&idx[3]);
			floral::vec3f c;
			is.read(&c);

			texIdx.push_back(idx[0]);
			texIdx.push_back(idx[1]);
			texIdx.push_back(idx[2]);
			texIdx.push_back(idx[3]);
			patchColor.push_back(c);
		}
		floral::close_file(rf);

		// index correction
		for (ssize i = 0; i < indicesCount; i++)
		{
			s32 idx = texIdx[i];
			floral::vec2i tp = texVtx[idx];
			const size firstIdx = texVtx.find(tp);
			if (firstIdx != idx)
			{
				texIdx[i] = firstIdx;
			}
		}

		for (ssize i = 0; i < indicesCount; i += 4)
		{
			s32 idx[4];
			idx[0] = texIdx[i];
			idx[1] = texIdx[i + 1];
			idx[2] = texIdx[i + 2];
			idx[3] = texIdx[i + 3];

			floral::vec3f color[4];
			{
				ssize c = 0;
				for (ssize j = 0; j < indicesCount; j++)
				{
					if (idx[0] == texIdx[j])
					{
						ssize pidx = j / 4;
						color[0] += patchColor[pidx];
						c++;
					}
				}
				FLORAL_ASSERT(c <= 4);
				color[0] /= c;
			}
			{
				ssize c = 0;
				for (ssize j = 0; j < indicesCount; j++)
				{
					if (idx[1] == texIdx[j])
					{
						ssize pidx = j / 4;
						color[1] += patchColor[pidx];
						c++;
					}
				}
				FLORAL_ASSERT(c <= 4);
				color[1] /= c;
			}
			{
				ssize c = 0;
				for (ssize j = 0; j < indicesCount; j++)
				{
					if (idx[2] == texIdx[j])
					{
						ssize pidx = j / 4;
						color[2] += patchColor[pidx];
						c++;
					}
				}
				FLORAL_ASSERT(c <= 4);
				color[2] /= c;
			}
			{
				ssize c = 0;
				for (ssize j = 0; j < indicesCount; j++)
				{
					if (idx[3] == texIdx[j])
					{
						ssize pidx = j / 4;
						color[3] += patchColor[pidx];
						c++;
					}
				}
				FLORAL_ASSERT(c <= 4);
				color[3] /= c;
			}

			SimpleVertex vtx[6];
			vtx[0].Position = texVtx[idx[2]];
			vtx[1].Position = texVtx[idx[1]];
			vtx[2].Position = texVtx[idx[0]];
			vtx[3].Position = texVtx[idx[0]];
			vtx[4].Position = texVtx[idx[3]];
			vtx[5].Position = texVtx[idx[2]];

			vtx[0].Color = color[2];
			vtx[1].Color = color[1];
			vtx[2].Color = color[0];
			vtx[3].Color = color[0];
			vtx[4].Color = color[3];
			vtx[5].Color = color[2];

			RasterizeTriangle2D(&vtx[0], bufferDim, bufferDim, pData);
			RasterizeTriangle2D(&vtx[3], bufferDim, bufferDim, pData);
		}
#endif

		m_Texture = insigne::create_texture(texDesc);
	}

	// tonemap shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_VertexShader);
		strcpy(desc.fs, s_FragmentShader);
		desc.vs_path = floral::path("/scene/quad_vs");
		desc.fs_path = floral::path("/scene/quad_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);

		// static uniform data
		{
			s32 texSlot = insigne::get_material_texture_slot(m_Material, "u_Tex");
			m_Material.textures[texSlot].value = m_Texture;
		}
	}
}

void Quad2DRasterize::OnUpdate(const f32 i_deltaMs)
{
}

void Quad2DRasterize::OnDebugUIUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Quad2DRasterize Controller");
	ImGui::End();
}

void Quad2DRasterize::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<SurfacePT>(m_VB, m_IB, m_Material);
	IDebugUI::OnFrameRender(i_deltaMs);

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void Quad2DRasterize::OnCleanUp()
{
}

}
