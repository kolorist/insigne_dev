#include "GeometryVault.h"

#include <floral/comgeo/shapegen.h>

#include <calyx/context.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/meshoptimizer/meshoptimizer.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

GeometryVault::GeometryVault()
{
}

GeometryVault::~GeometryVault()
{
}

ICameraMotion* GeometryVault::GetCameraMotion()
{
	return nullptr;
}

const_cstr GeometryVault::GetName() const
{
	return k_name;
}

void GeometryVault::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/perf/geometry_vault");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(4));
	m_TemporalArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));

	// register surfaces
	insigne::register_surface_type<geo3d::SurfacePC>();

	{
		m_TemporalArena->free_all();
		floral::fixed_array<VertexPC, LinearArena> sphereVertices;
		floral::fixed_array<s32, LinearArena> sphereIndices;

		sphereVertices.reserve(4096, m_TemporalArena);
		sphereIndices.reserve(8192, m_TemporalArena);

		sphereVertices.resize(4096);
		sphereIndices.resize(8192);

		floral::reset_generation_transforms_stack();
		floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
				3, 0, sizeof(VertexPC),
				floral::geo_vertex_format_e::position,
				&sphereVertices[0], &sphereIndices[0]);
		sphereVertices.resize(genResult.vertices_generated);
		sphereIndices.resize(genResult.indices_generated);

		m_Surface = helpers::CreateSurfaceGPU(&sphereVertices[0], genResult.vertices_generated, sizeof(geo3d::VertexPC),
				&sphereIndices[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);
	}

	m_MemoryArena->free_all();
	floral::relative_path matPath = floral::build_relative_path("vertexcolor.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	bool createResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);

	// UBs
	{
		const floral::vec3f k_camPos = floral::vec3f(0.0f, 5.1f, 7.0f);
		floral::mat4x4f view = floral::construct_lookat_point(
				floral::vec3f(0.0f, 1.0f, 0.0f),
				k_camPos,
				floral::vec3f(0.0f, 0.0f, 0.0f));
		floral::mat4x4f projection = floral::construct_perspective(0.01f, 100.0f, 16.3125f, 16.0f / 9.0f);
		m_SceneData.WVP = projection * view;

		insigne::ubdesc_t desc;
		desc.region_size = floral::next_pow2(size(sizeof(SceneData)));
		desc.data = &m_SceneData;
		desc.data_size = sizeof(SceneData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_SceneUB = insigne::copy_create_ub(desc);
	}

	insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Scene", 0, 0, m_SceneUB);
}

void GeometryVault::_OnUpdate(const f32 i_deltaMs)
{
}

void GeometryVault::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo3d::SurfacePC>(m_Surface.vb, m_Surface.ib, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GeometryVault::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo3d::SurfacePC>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_TemporalArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
