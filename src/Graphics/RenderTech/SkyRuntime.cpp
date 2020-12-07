#include "SkyRuntime.h"

#include <calyx/context.h>

#include <floral/containers/array.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/MaterialParser.h"

namespace stone
{
namespace tech
{

static const s32 k_transmittanceTextureWidth = 256;
static const s32 k_transmittanceTextureHeight = 64;

static const s32 k_scatteringTextureRSize = 32;
static const s32 k_scatteringTextureMuSize = 128;
static const s32 k_scatteringTextureMuSSize = 32;
static const s32 k_scatteringTextureNuSize = 8;

static const s32 k_scatteringTextureWidth = k_scatteringTextureNuSize * k_scatteringTextureMuSSize;
static const s32 k_scatteringTextureHeight = k_scatteringTextureMuSize;
static const s32 k_scatteringTextureDepth = k_scatteringTextureRSize;

static const s32 k_irrandianceTextureWidth = 64;
static const s32 k_irrandianceTextureHeight = 16;

//-------------------------------------------------------------------

SkyRuntime::SkyRuntime()
	: m_TexDataArenaRegion { "stone/dynamic/sky runtime", SIZE_MB(128), &m_TexDataArena }
{
}

//-------------------------------------------------------------------

SkyRuntime::~SkyRuntime()
{
}

//-------------------------------------------------------------------

ICameraMotion* SkyRuntime::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr SkyRuntime::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void SkyRuntime::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/tech/sky");
	floral::push_directory(m_FileSystem, wdir);

	g_MemoryManager.initialize_allocator(m_TexDataArenaRegion);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	f32 aspectRatio = (f32)commonCtx->window_height / (f32)commonCtx->window_width;

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();

	const f32 k_size = 0.8f;
	floral::inplace_array<geo2d::VertexPT, 4> vertices;
	vertices.push_back({ { -k_size * aspectRatio, k_size }, { 0.0f, 1.0f } });
	vertices.push_back({ { -k_size * aspectRatio, -k_size }, { 0.0f, 0.0f } });
	vertices.push_back({ { k_size * aspectRatio, -k_size }, { 1.0f, 0.0f } });
	vertices.push_back({ { k_size * aspectRatio, k_size }, { 1.0f, 1.0f } });

	floral::inplace_array<s32, 6> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	m_Quad = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
			&indices[0], 6, insigne::buffer_usage_e::static_draw, true);

	m_MemoryArena->free_all();
	floral::relative_path matPath = floral::build_relative_path("sky.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	bool createResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	m_TransmittanceTexture = LoadRawHDRTexture2D("transmittance_texture.rtex2d", k_transmittanceTextureWidth, k_transmittanceTextureHeight, 3);
	m_ScatteringTexture = LoadRawHDRTexture3D("scattering_texture.rtex3d", k_scatteringTextureWidth, k_scatteringTextureHeight, k_scatteringTextureDepth, 4);
	m_IrradianceTexture = LoadRawHDRTexture2D("irradiance_texture.rtex2d", k_irrandianceTextureWidth, k_irrandianceTextureHeight, 3);

	insigne::helpers::assign_texture(m_MSPair.material, "u_TransmittanceTex", m_TransmittanceTexture);
	insigne::helpers::assign_texture(m_MSPair.material, "u_ScatteringTex", m_ScatteringTexture);
	insigne::helpers::assign_texture(m_MSPair.material, "u_IrradianceTex", m_IrradianceTexture);
#if 0
	m_TexData = m_TexDataArena.allocate_array<floral::vec3f>(256 * 32 * 32);
	for (s32 d = 0; d < 32; d++)
	{
		for (s32 h = 0; h < 32; h++)
		{
			for (s32 w = 0; w < 256; w++)
			{
				m_TexData[d * 256 * 32 + h * 256 + w] = floral::vec3f(w, h, d);
			}
		}
	}

	insigne::texture_desc_t texDesc;
	texDesc.width = 256;
	texDesc.height = 32;
	texDesc.depth = 32;
	texDesc.format = insigne::texture_format_e::hdr_rgb_high;
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.wrap_s = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_t = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_r = insigne::wrap_e::clamp_to_edge;
	texDesc.dimension = insigne::texture_dimension_e::tex_3d;
	texDesc.has_mipmap = false;
	texDesc.data = m_TexData;

	m_Texture = insigne::create_texture(texDesc);
	insigne::helpers::assign_texture(m_MSPair[5].material, "u_MainTex", m_Texture);
#endif
}

//-------------------------------------------------------------------

void SkyRuntime::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Controller##SkyRuntime");
	ImGui::End();
}

//-------------------------------------------------------------------

void SkyRuntime::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void SkyRuntime::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_MemoryManager.destroy_allocator(m_TexDataArenaRegion);

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------

insigne::texture_handle_t SkyRuntime::LoadRawHDRTexture2D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_channel)
{
	floral::relative_path iFilePath = floral::build_relative_path(i_texFile);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	ssize sizeBytes = i_w * i_h * i_channel * sizeof(f32);
	FLORAL_ASSERT(iFile.file_size == sizeBytes);
	f32* texData = (f32*)m_TexDataArena.allocate(sizeBytes);
	floral::read_all_file(iFile, texData);
	floral::close_file(iFile);

	insigne::texture_desc_t texDesc;
	texDesc.width = i_w;
	texDesc.height = i_h;
	texDesc.format = insigne::texture_format_e::hdr_rgb_high;
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.dimension = insigne::texture_dimension_e::tex_2d;
	texDesc.has_mipmap = false;
	texDesc.data = texData;

	return insigne::create_texture(texDesc);
}

//-------------------------------------------------------------------

insigne::texture_handle_t SkyRuntime::LoadRawHDRTexture3D(const_cstr i_texFile, const s32 i_w, const s32 i_h, const s32 i_d, const s32 i_channel)
{
	floral::relative_path iFilePath = floral::build_relative_path(i_texFile);
	floral::file_info iFile = floral::open_file_read(m_FileSystem, iFilePath);
	ssize sizeBytes = i_w * i_h * i_d * i_channel * sizeof(f32);
	FLORAL_ASSERT(iFile.file_size == sizeBytes);
	f32* texData = (f32*)m_TexDataArena.allocate(sizeBytes);
	floral::read_all_file(iFile, texData);
	floral::close_file(iFile);

	insigne::texture_desc_t texDesc;
	texDesc.width = i_w;
	texDesc.height = i_h;
	texDesc.depth = i_d;
	if (i_channel == 3)
	{
		texDesc.format = insigne::texture_format_e::hdr_rgb_high;
	}
	else if (i_channel == 4)
	{
		texDesc.format = insigne::texture_format_e::hdr_rgba_high;
	}
	else
	{
		FLORAL_ASSERT(false);
	}
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.wrap_s = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_t = insigne::wrap_e::clamp_to_edge;
	texDesc.wrap_r = insigne::wrap_e::clamp_to_edge;
	texDesc.dimension = insigne::texture_dimension_e::tex_3d;
	texDesc.has_mipmap = false;
	texDesc.data = texData;

	return insigne::create_texture(texDesc);
}

//-------------------------------------------------------------------
}
}
