#include "Matrices.h"

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

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

Matrices::Matrices()
	: m_PostFXConfig("hdr_postfx.pfx")
{
}

//-------------------------------------------------------------------

Matrices::~Matrices()
{
}

//-------------------------------------------------------------------

ICameraMotion* Matrices::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr Matrices::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void Matrices::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/perf/matrices");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();
	insigne::register_surface_type<geo3d::SurfacePC>();

	{
		floral::inplace_array<geo3d::VertexPC, 4> vertices;
		vertices.push_back({ { -1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });
		vertices.push_back({ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });
		vertices.push_back({ { 1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });
		vertices.push_back({ { 1.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });

		floral::inplace_array<s32, 6> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(2);
		indices.push_back(3);
		indices.push_back(0);

		m_Quad = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo3d::VertexPC),
				&indices[0], 6, insigne::buffer_usage_e::static_draw, true);
	}

	{
		m_MemoryArena->free_all();
		floral::relative_path matPath = floral::build_relative_path("solid.mat");
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
		bool createResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MaterialDataArena);
		FLORAL_ASSERT(createResult == true);
	}

	m_MemoryArena->free_all();
	floral::relative_path pfxPath = floral::build_relative_path(m_PostFXConfig);
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(m_FileSystem, pfxPath, m_MemoryArena);
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	m_PostFXChain.Initialize(m_FileSystem, pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);
}

//-------------------------------------------------------------------

void Matrices::_OnUpdate(const f32 i_deltaMs)
{
}

//-------------------------------------------------------------------

void Matrices::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
	insigne::draw_surface<geo3d::SurfacePC>(m_Quad.vb, m_Quad.ib, m_MSPair.material);
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

void Matrices::_OnCleanUp()
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
