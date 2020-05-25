#include "GammaCorrection.h"

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

GammaCorrection::GammaCorrection()
{
}

//-------------------------------------------------------------------

GammaCorrection::~GammaCorrection()
{
}

//-------------------------------------------------------------------

ICameraMotion* GammaCorrection::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr GammaCorrection::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void GammaCorrection::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();

	floral::inplace_array<geo2d::VertexPT, 4> vertices;
	vertices.push_back({ { -0.5f, 0.5f }, { 0.0f, 1.0f } });
	vertices.push_back({ { -0.5f, -0.5f }, { 0.0f, 0.0f } });
	vertices.push_back({ { 0.5f, -0.5f }, { 1.0f, 0.0f } });
	vertices.push_back({ { 0.5f, 0.5f }, { 1.0f, 1.0f } });

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
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(
			floral::path("tests/perf/gamma_correction/gamma.mat"), m_MemoryArena);

	const bool pbrMaterialResult = mat_loader::CreateMaterial(&m_MSPair, matDesc, m_MaterialDataArena);
	FLORAL_ASSERT(pbrMaterialResult == true);
}

//-------------------------------------------------------------------

void GammaCorrection::_OnUpdate(const f32 i_deltaMs)
{
}

//-------------------------------------------------------------------

void GammaCorrection::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void GammaCorrection::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
