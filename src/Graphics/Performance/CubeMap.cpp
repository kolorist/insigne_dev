#include "CubeMap.h"

#include <calyx/context.h>

#include <floral/containers/array.h>
#include <floral/comgeo/shapegen.h>
#include <floral/gpds/camera.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/MaterialParser.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

CubeMapTextures::CubeMapTextures()
{
}

//-------------------------------------------------------------------

CubeMapTextures::~CubeMapTextures()
{
}

//-------------------------------------------------------------------

ICameraMotion* CubeMapTextures::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr CubeMapTextures::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void CubeMapTextures::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(64));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	f32 aspectRatio = (f32)commonCtx->window_width / (f32)commonCtx->window_height;

	// register surfaces
	insigne::register_surface_type<geo3d::SurfaceP>();
	insigne::register_surface_type<geo2d::SurfacePT>();

	// ico sphere
	m_MemoryArena->free_all();
	floral::fast_fixed_array<geo3d::VertexP, FreelistArena> sphereVertices;
	floral::fast_fixed_array<s32, FreelistArena> sphereIndices;

	sphereVertices.reserve(4096, m_MemoryArena);
	sphereIndices.reserve(8192, m_MemoryArena);

	sphereVertices.resize(4096);
	sphereIndices.resize(8192);

	floral::reset_generation_transforms_stack();
	floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
			3, 0, sizeof(geo3d::VertexP),
			floral::geo_vertex_format_e::position,
			&sphereVertices[0], &sphereIndices[0]);
	m_Sphere = helpers::CreateSurfaceGPU(&sphereVertices[0], genResult.vertices_generated, sizeof(geo3d::VertexP),
			&sphereIndices[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);

	floral::reset_generation_transforms_stack();
	genResult = floral::generate_unit_icosphere_3d(
			2, 0, sizeof(geo3d::VertexP),
			floral::geo_vertex_format_e::position,
			&sphereVertices[0], &sphereIndices[0]);
	m_SkySphere = helpers::CreateSurfaceGPU(&sphereVertices[0], genResult.vertices_generated, sizeof(geo3d::VertexP),
			&sphereIndices[0], genResult.indices_generated, insigne::buffer_usage_e::static_draw, true);

	m_MemoryArena->free_all();
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(
			floral::path("tests/perf/textures/icosphere.mat"), m_MemoryArena);
	bool matLoadResult = mat_loader::CreateMaterial(&m_SphereMSPair, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(matLoadResult == true);

	m_MemoryArena->free_all();
	matDesc = mat_parser::ParseMaterial(floral::path("tests/perf/textures/skysphere.mat"), m_MemoryArena);
	matLoadResult = mat_loader::CreateMaterial(&m_SkyMSPair, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(matLoadResult == true);

	const floral::vec3f k_cameraPosition(6.0f, 0.0f, 0.0f);
	floral::mat4x4f viewMatrix = floral::construct_lookat_point(floral::vec3f(0.0f, 1.0f, 0.0f),
			k_cameraPosition,
			floral::vec3f(0.0f, 0.0f, 0.0f));
	floral::mat4x4f projectionMatrix = floral::construct_perspective(0.01f, 100.0f, 45.0f, aspectRatio);
	m_SceneData.viewProjectionMatrix = projectionMatrix * viewMatrix;
	m_SceneData.cameraPosition = floral::vec4f(k_cameraPosition, 1.0f);

	insigne::ubdesc_t ubDesc;
	ubDesc.region_size = floral::next_pow2(size(sizeof(SceneData)));
	ubDesc.data = &m_SceneData;
	ubDesc.data_size = sizeof(SceneData);
	ubDesc.usage = insigne::buffer_usage_e::dynamic_draw;
	m_SceneUB = insigne::copy_create_ub(ubDesc);

	insigne::helpers::assign_uniform_block(m_SphereMSPair.material, "ub_Scene", 0, 0, m_SceneUB);
	insigne::helpers::assign_uniform_block(m_SkyMSPair.material, "ub_Scene", 0, 0, m_SceneUB);

	m_MemoryArena->free_all();
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(
			floral::path("tests/perf/textures/hdr_pfx.pfx"),
			m_MemoryArena);
	m_PostFXChain.Initialize(pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);
}

//-------------------------------------------------------------------

void CubeMapTextures::_OnUpdate(const f32 i_deltaMs)
{
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(2.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 0.5f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 0.5f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 2.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 0.5f));
}

//-------------------------------------------------------------------

void CubeMapTextures::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
	insigne::draw_surface<geo3d::SurfaceP>(m_Sphere.vb, m_Sphere.ib, m_SphereMSPair.material);
	insigne::draw_surface<geo3d::SurfaceP>(m_SkySphere.vb, m_SkySphere.ib, m_SkyMSPair.material);
	debugdraw::Render(m_SceneData.viewProjectionMatrix);
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

void CubeMapTextures::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	insigne::unregister_surface_type<geo2d::SurfacePT>();
	insigne::unregister_surface_type<geo3d::SurfaceP>();

	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------
}
}
