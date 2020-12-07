#include "Textures.h"

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
namespace perf
{
//-------------------------------------------------------------------

Textures::Textures()
	: m_ShaderIdx(0)
	, m_TexDataArenaRegion { "stone/dynamic/textures", SIZE_MB(128), &m_TexDataArena }
{
}

//-------------------------------------------------------------------

Textures::~Textures()
{
}

//-------------------------------------------------------------------

ICameraMotion* Textures::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr Textures::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void Textures::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/perf/textures");
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
	floral::relative_path matPath = floral::build_relative_path("no_filtering.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	bool createResult = mat_loader::CreateMaterial(&m_MSPair[0], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("linear_filtering.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[1], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("trilinear_filtering.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[2], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("trilinear_4_samples.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[3], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("trilinear_8_samples.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[4], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	matPath = floral::build_relative_path("tex3d_slice.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[5], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

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
}

//-------------------------------------------------------------------

void Textures::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Controller##Textures");
	ImGui::Text("Choose shaders:");
	if (ImGui::RadioButton("no filtering", m_ShaderIdx == 0))
	{
		m_ShaderIdx = 0;
	}
	if (ImGui::RadioButton("linear filtering", m_ShaderIdx == 1))
	{
		m_ShaderIdx = 1;
	}
	if (ImGui::RadioButton("trilinear filtering", m_ShaderIdx == 2))
	{
		m_ShaderIdx = 2;
	}
	if (ImGui::RadioButton("4 texture samples", m_ShaderIdx == 3))
	{
		m_ShaderIdx = 3;
	}
	if (ImGui::RadioButton("8 texture samples", m_ShaderIdx == 4))
	{
		m_ShaderIdx = 4;
	}
	if (ImGui::RadioButton("3d texture", m_ShaderIdx == 5))
	{
		m_ShaderIdx = 5;
	}
	ImGui::End();
}

//-------------------------------------------------------------------

void Textures::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair[m_ShaderIdx].material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void Textures::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_MemoryManager.destroy_allocator(m_TexDataArenaRegion);

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
