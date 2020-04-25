#include "PostFX.h"

#include <clover/Logger.h>

#include <calyx/context.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>

#include "InsigneImGui.h"

#include "Graphics/PostFXParser.h"
#include "Graphics/MaterialParser.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------


PostFX::PostFX()
{
}

PostFX::~PostFX()
{
}

ICameraMotion* PostFX::GetCameraMotion()
{
	return nullptr;
}

const_cstr PostFX::GetName() const
{
	return k_name;
}

void PostFX::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(128));

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
			floral::path("tests/perf/post_effects/sample.mat"), m_MemoryArena);

	const bool pbrMaterialResult = mat_loader::CreateMaterial(&m_MSPair, matDesc, m_MaterialDataArena);
	FLORAL_ASSERT(pbrMaterialResult == true);

	m_MemoryArena->free_all();
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(
			floral::path("tests/perf/post_effects/posteffects.pfx"),
			m_MemoryArena);
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	m_PostFXChain.Initialize(pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);

	m_MemoryArena->free_all();
}

void PostFX::_OnUpdate(const f32 i_deltaMs)
{
}

void PostFX::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair.material);
	m_PostFXChain.EndMainOutput();

	m_PostFXChain.Process();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_PostFXChain.Present();
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void PostFX::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	m_PostFXChain.CleanUp();

	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
