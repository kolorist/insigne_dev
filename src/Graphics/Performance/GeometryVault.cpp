#include "GeometryVault.h"

#include <floral/comgeo/shapegen.h>
#include <floral/math/rng.h>

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
	: m_SurfaceIdx(0)
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
	m_TemporalArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(32));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));

	// register surfaces
	insigne::register_surface_type<geo3d::SurfacePNC>();

	{
		m_TemporalArena->free_all();
		floral::fast_fixed_array<geo3d::VertexPNC, LinearArena> sphereVertices;
		floral::fast_fixed_array<s32, LinearArena> sphereIndices;

		const size numIndices = 1 << 18;
		sphereVertices.reserve(numIndices, m_TemporalArena);
		sphereIndices.reserve(numIndices, m_TemporalArena);

#if 0
		sphereVertices.resize(8192);
		sphereIndices.resize(8192);
		floral::reset_generation_transforms_stack();
		floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
				3, 0, sizeof(geo3d::VertexPNC),
				floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
				&sphereVertices[0], &sphereIndices[0]);
		sphereVertices.resize(genResult.vertices_generated);
		sphereIndices.resize(genResult.indices_generated);
#else
		floral::rng rng = floral::rng(1234);
		floral::geo_generate_result_t genResult;
		int trianglesCount = sphereIndices.get_capacity() / 3;
		genResult.vertices_generated = trianglesCount * 3;
		genResult.indices_generated = genResult.vertices_generated;
		for (int i = 0; i < trianglesCount; i++)
		{
			geo3d::VertexPNC p0, p1, p2;
			p0.position = floral::vec3f(rng.get_f32() * 4.0f - 2.0f, rng.get_f32() * 2.0f - 1.0f, rng.get_f32() * 2.0f - 1.0f);
			p1.position = p0.position + floral::vec3f(rng.get_f32(), rng.get_f32(), rng.get_f32()) * 0.4f;
			p2.position = p0.position + floral::vec3f(rng.get_f32(), rng.get_f32(), rng.get_f32()) * 0.4f;

			p0.normal = floral::vec3f(rng.get_f32() * 2.0f - 1.0f, rng.get_f32() * 2.0f - 1.0f, rng.get_f32() * 2.0f - 1.0f);
			p1.normal = p0.normal;
			p2.normal = p0.normal;

			p0.color = floral::vec4f(0.0f, 0.0f, 0.0f, 0.0f);
			p1.color = floral::vec4f(0.0f, 0.0f, 0.0f, 0.0f);
			p2.color = floral::vec4f(0.0f, 0.0f, 0.0f, 0.0f);

			sphereIndices.push_back(i * 3);
			sphereIndices.push_back(i * 3 + 1);
			sphereIndices.push_back(i * 3 + 2);
			sphereVertices.push_back(p0);
			sphereVertices.push_back(p1);
			sphereVertices.push_back(p2);
		}
#endif

		m_Surface[0] = helpers::CreateSurfaceGPU(&sphereVertices[0], genResult.vertices_generated, sizeof(geo3d::VertexPNC),
				&sphereIndices[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);
		insigne::dispatch_render_pass();

		floral::fast_fixed_array<u32, LinearArena> remapTable;
		floral::fast_fixed_array<u32, LinearArena> remapIndices;
		floral::fast_fixed_array<geo3d::VertexPNC, LinearArena> remapVertices;
		remapTable.reserve(genResult.indices_generated, m_TemporalArena);
		remapTable.resize(genResult.indices_generated);
		size vtxCount = meshopt_generateVertexRemap(&remapTable[0], (u32*)&sphereIndices[0], genResult.indices_generated,
				&sphereVertices[0], genResult.vertices_generated, sizeof(geo3d::VertexPNC));
		remapIndices.reserve(genResult.indices_generated, m_TemporalArena);
		remapIndices.resize(genResult.indices_generated);
		remapVertices.reserve(vtxCount, m_TemporalArena);
		remapVertices.resize(vtxCount);
		meshopt_remapIndexBuffer(&remapIndices[0], (u32*)&sphereIndices[0], genResult.indices_generated, &remapTable[0]);
		meshopt_remapVertexBuffer(&remapVertices[0], &sphereVertices[0], vtxCount, sizeof(geo3d::VertexPNC), &remapTable[0]);

		meshopt_optimizeVertexCache(&remapIndices[0], &remapIndices[0], genResult.indices_generated, vtxCount);
		meshopt_optimizeOverdraw(&remapIndices[0], &remapIndices[0], genResult.indices_generated, &remapVertices[0].position.x, vtxCount, sizeof(geo3d::VertexPNC), 1.05f);
		meshopt_optimizeVertexFetch(&remapVertices[0], &remapIndices[0], genResult.indices_generated, &remapVertices[0], vtxCount, sizeof(geo3d::VertexPNC));

		m_Surface[1] = helpers::CreateSurfaceGPU(&remapVertices[0], vtxCount, sizeof(geo3d::VertexPNC),
				&remapIndices[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);
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
	ImGui::Begin("Controller##GeometryVault");
	ImGui::Text("Choose geometry:");
	if (ImGui::RadioButton("original", m_SurfaceIdx == 0))
	{
		m_SurfaceIdx = 0;
	}
	if (ImGui::RadioButton("otm step 1", m_SurfaceIdx == 1))
	{
		m_SurfaceIdx = 1;
	}
	ImGui::End();
}

void GeometryVault::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo3d::SurfacePNC>(m_Surface[m_SurfaceIdx].vb, m_Surface[m_SurfaceIdx].ib, m_MSPair.material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void GeometryVault::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo3d::SurfacePNC>();

	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_TemporalArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

//-------------------------------------------------------------------
}
}
