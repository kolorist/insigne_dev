#include "TextureStreaming.h"

#include <floral/containers/array.h>
#include <floral/assert/assert.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{
namespace perf
{

static const_cstr k_SuiteName = "texture streaming";

TextureStreaming::TextureStreaming()
{
}

TextureStreaming::~TextureStreaming()
{
}

const_cstr TextureStreaming::GetName() const
{
	return k_SuiteName;
}

void TextureStreaming::OnInitialize(floral::filesystem<FreelistArena>* i_fs)
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();
	// register surfaces
	insigne::register_surface_type<SurfacePT>();
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(1));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	m_TextureDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(2));

	const f32 aspectRatio = 16.0f / 9.0f;
	const f32 w = 0.5;
	const f32 h = w * aspectRatio;
	floral::inplace_array<VertexPT, 4> vertices;
	vertices.push_back(VertexPT { { -w, -h }, { 0.0f, 0.0f } });
	vertices.push_back(VertexPT { { w, -h }, { 1.0f, 0.0f } });
	vertices.push_back(VertexPT { { w, h }, { 1.0f, 1.0f } });
	vertices.push_back(VertexPT { { -w, h }, { 0.0f, 1.0f } });

	floral::inplace_array<s32, 6> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;
		
		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(VertexPT), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_IB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_IB, &indices[0], indices.get_size(), 0);
	}

	{
		m_TexData = m_TextureDataArena->allocate_array<floral::vec3f>(128 * 128);

		for (size i = 0; i < 128; i++)
		{
			m_TexData[i] = floral::vec3f(1.0f, 0.0, 0.0);
		}

		insigne::texture_desc_t texDesc;
		texDesc.width = 128;
		texDesc.height = 128;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::nearest;
		texDesc.mag_filter = insigne::filtering_e::nearest;
		texDesc.dimension = insigne::texture_dimension_e::tex_2d;
		texDesc.has_mipmap = false;
		texDesc.data = nullptr;

		m_Texture = insigne::create_texture(texDesc);

		const size dataSize = insigne::prepare_texture_desc(texDesc);
		insigne::copy_update_texture(m_Texture, m_TexData, dataSize);
	}

	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(
			floral::path("gfx/mat/texture_streaming.mat"), m_MemoryArena);
	const bool result = mat_loader::CreateMaterial(&m_MSPair, matDesc, m_MaterialDataArena);

	insigne::helpers::assign_texture(m_MSPair.material, "u_Tex", m_Texture);

	FLORAL_ASSERT(result == true);
}

void TextureStreaming::OnUpdate(const f32 i_deltaMs)
{
	static size frameIdx = 0;
	frameIdx++;
	if (frameIdx % 30 == 0)
	{
		for (size i = 0; i < 128; i++)
		{
			m_TexData[i + 128 * (frameIdx / 30)] = floral::vec3f(1.0f, 0.0, 0.0);
		}
		insigne::copy_update_texture(m_Texture, m_TexData, 128 * 128 * sizeof(floral::vec3f));
	}
}

void TextureStreaming::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<SurfacePT>(m_VB, m_IB, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void TextureStreaming::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
	insigne::unregister_surface_type<SurfacePT>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

}
}
