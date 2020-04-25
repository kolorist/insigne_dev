#include "Blending.h"

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

Blending::Blending()
{
}

//-------------------------------------------------------------------

Blending::~Blending()
{
}

//-------------------------------------------------------------------

ICameraMotion* Blending::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr Blending::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void Blending::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();

	{
		floral::inplace_array<geo2d::VertexPT, 4> vertices;
		vertices.push_back({ { -0.8f, 0.8f }, { 0.0f, 1.0f } });
		vertices.push_back({ { -0.8f, 0.6f }, { 0.0f, 0.0f } });
		vertices.push_back({ { 0.8f, 0.6f }, { 1.0f, 0.0f } });
		vertices.push_back({ { 0.8f, 0.8f }, { 1.0f, 1.0f } });

		floral::inplace_array<s32, 6> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(0);

		m_Quad0 = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
				&indices[0], 6, insigne::buffer_usage_e::static_draw, true);
		m_MemoryArena->free_all();
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(
				floral::path("tests/perf/blending/solid.mat"), m_MemoryArena);

		const bool createResult = mat_loader::CreateMaterial(&m_MSPair0, matDesc, m_MaterialDataArena);
		FLORAL_ASSERT(createResult == true);
	}

	{
		floral::inplace_array<geo2d::VertexPT, 4> vertices;
		vertices.push_back({ { -0.7f, 0.9f }, { 0.0f, 1.0f } });
		vertices.push_back({ { -0.7f, -0.9f }, { 0.0f, 0.0f } });
		vertices.push_back({ { -0.6f, -0.9f }, { 1.0f, 0.0f } });
		vertices.push_back({ { -0.6f, 0.9f }, { 1.0f, 1.0f } });

		floral::inplace_array<s32, 6> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(0);

		m_TQuad0 = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
				&indices[0], 6, insigne::buffer_usage_e::static_draw, true);
		m_MemoryArena->free_all();
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(
				floral::path("tests/perf/blending/transparent_50.mat"), m_MemoryArena);

		const bool createResult = mat_loader::CreateMaterial(&m_TMSPair0, matDesc, m_MaterialDataArena);
		FLORAL_ASSERT(createResult == true);
	}
	// TODO: gamma correction
}

//-------------------------------------------------------------------

void Blending::_OnUpdate(const f32 i_deltaMs)
{
}

//-------------------------------------------------------------------

void Blending::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePT>(m_Quad0.vb, m_Quad0.ib, m_MSPair0.material);

	insigne::draw_surface<geo2d::SurfacePT>(m_TQuad0.vb, m_TQuad0.ib, m_TMSPair0.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void Blending::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
