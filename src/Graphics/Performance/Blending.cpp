#include "Blending.h"

#include <floral/containers/array.h>

#include <clover/Logger.h>

#include <calyx/context.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/MaterialParser.h"

#define OPTIMIZE_OVERDRAW 1

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

Blending::Blending(const_cstr i_pfxConfig)
	: m_PostFXConfig(i_pfxConfig)
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
	floral::relative_path wdir = floral::build_relative_path("tests/perf/blending");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();
	insigne::register_surface_type<geo3d::SurfacePC>();

	{
		floral::inplace_array<geo3d::VertexPC, 4> vertices;
		vertices.push_back({ { -0.8f, 0.8f, 0.0f }, { 0.0f, 2.0f, 0.0f, 1.0f } });
		vertices.push_back({ { -0.8f, -0.8f, 0.0f }, { 0.0f, 2.0f, 0.0f, 1.0f } });
		vertices.push_back({ { 0.8f, -0.8f, 0.0f }, { 0.0f, 2.0f, 0.0f, 1.0f } });
		vertices.push_back({ { 0.8f, 0.8f, 0.0f }, { 0.0f, 2.0f, 0.0f, 1.0f } });

		floral::inplace_array<s32, 6> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(0);

		m_Quad0 = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo3d::VertexPC),
				&indices[0], 6, insigne::buffer_usage_e::static_draw, true);
	}

#if OPTIMIZE_OVERDRAW
	{
		floral::inplace_array<geo3d::VertexPC, 4> vertices;
		vertices.push_back({ { -0.7f, 0.9f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });
		vertices.push_back({ { -0.7f, 0.5f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });
		vertices.push_back({ {  0.0f, 0.5f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });
		vertices.push_back({ {  0.0f, 0.9f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });

		floral::inplace_array<s32, 6> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(0);

		m_TQuad0 = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo3d::VertexPC),
				&indices[0], 6, insigne::buffer_usage_e::static_draw, true);

		vertices[0].position = floral::vec3f(-0.7f, 0.5f, -0.1f);
		vertices[1].position = floral::vec3f(-0.7f, -0.9f, -0.1f);
		vertices[2].position = floral::vec3f(-0.0f, -0.9f, -0.1f);
		vertices[3].position = floral::vec3f(-0.0f, 0.5f, -0.1f);

		m_TQuad00 = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo3d::VertexPC),
				&indices[0], 6, insigne::buffer_usage_e::static_draw, true);
	}
#else
	{
		floral::inplace_array<geo3d::VertexPC, 8> vertices;
		vertices.push_back({ { -0.7f, 0.9f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });
		vertices.push_back({ { -0.7f, 0.5f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });
		vertices.push_back({ {  0.0f, 0.5f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });
		vertices.push_back({ {  0.0f, 0.9f, -0.1f }, { 1.0f, 0.0f, 0.0f, 0.5f } });

		vertices.push_back({ { -0.7f, 0.5f, -0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
		vertices.push_back({ { -0.7f, -0.9f, -0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
		vertices.push_back({ {  0.0f, -0.9f, -0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
		vertices.push_back({ {  0.0f, 0.5f, -0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f } });

		floral::inplace_array<s32, 12> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(0);

		indices.push_back(4);
		indices.push_back(5);
		indices.push_back(6);
		indices.push_back(6);
		indices.push_back(7);
		indices.push_back(4);

		m_TQuad0 = helpers::CreateSurfaceGPU(&vertices[0], 8, sizeof(geo3d::VertexPC),
				&indices[0], 12, insigne::buffer_usage_e::static_draw, true);
	}
#endif

	{
		m_MemoryArena->free_all();
		floral::relative_path matPath = floral::build_relative_path("solid.mat");
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
		bool createResult = mat_loader::CreateMaterial(&m_MSPairSolid, m_FileSystem, matDesc, m_MaterialDataArena);
		FLORAL_ASSERT(createResult == true);

		m_MemoryArena->free_all();
		matPath = floral::build_relative_path("transparent.mat");
		matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
		createResult = mat_loader::CreateMaterial(&m_MSPairTransparent, m_FileSystem, matDesc, m_MaterialDataArena);
		FLORAL_ASSERT(createResult == true);
	}

	m_MemoryArena->free_all();
	floral::relative_path pfxPath = floral::build_relative_path(m_PostFXConfig);
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(m_FileSystem, pfxPath, m_MemoryArena);
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	m_PostFXChain.Initialize(m_FileSystem, pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);
}

//-------------------------------------------------------------------

void Blending::_OnUpdate(const f32 i_deltaMs)
{
}

//-------------------------------------------------------------------

void Blending::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
#if OPTIMIZE_OVERDRAW
	// solid: front to back
	insigne::draw_surface<geo3d::SurfacePC>(m_TQuad00.vb, m_TQuad00.ib, m_MSPairSolid.material);
	insigne::draw_surface<geo3d::SurfacePC>(m_Quad0.vb, m_Quad0.ib, m_MSPairSolid.material);

	// transparent
	insigne::draw_surface<geo3d::SurfacePC>(m_TQuad0.vb, m_TQuad0.ib, m_MSPairTransparent.material);
#else
	// background - transparent
	insigne::draw_surface<geo3d::SurfacePC>(m_Quad0.vb, m_Quad0.ib, m_MSPairTransparent.material);
	// sprite - transparent
	insigne::draw_surface<geo3d::SurfacePC>(m_TQuad0.vb, m_TQuad0.ib, m_MSPairTransparent.material);
#endif
	m_PostFXChain.EndMainOutput();

	m_PostFXChain.Process();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	m_PostFXChain.Present();
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

//-------------------------------------------------------------------

void Blending::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo3d::SurfacePC>();
	insigne::unregister_surface_type<geo2d::SurfacePT>();

	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
