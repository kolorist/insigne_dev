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

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
