#include "ShaderVault.h"

#include <calyx/context.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

ShaderVault::ShaderVault()
	: m_MaterialIndex(0)
{
}

ShaderVault::~ShaderVault()
{
}

ICameraMotion* ShaderVault::GetCameraMotion()
{
	return nullptr;
}

const_cstr ShaderVault::GetName() const
{
	return k_name;
}

void ShaderVault::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	floral::relative_path wdir = floral::build_relative_path("tests/perf/shader_vault");
	floral::push_directory(m_FileSystem, wdir);

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(2));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));

	// register surfaces
	insigne::register_surface_type<geo2d::SurfacePT>();

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	f32 aspectRatio = (f32)commonCtx->window_height / (f32)commonCtx->window_width;
	const f32 renderWidth = 800.0f;
	const f32 renderHeight = 480.0f;
	{
		m_SceneData.resolution.x = renderWidth;
		m_SceneData.resolution.y = renderHeight;
		m_SceneData.resolution.z = 2.0f / renderWidth;
		m_SceneData.resolution.w = 2.0f / renderHeight;
		m_SceneData.timeSeconds.x = 0.0f;
		m_SceneData.timeSeconds.y = 1.0f / 60.0f;

		insigne::ubdesc_t ubDesc;
		ubDesc.region_size = floral::next_pow2(size(sizeof(SceneData)));
		ubDesc.data = &m_SceneData;
		ubDesc.data_size = sizeof(SceneData);
		ubDesc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_SceneUB = insigne::copy_create_ub(ubDesc);
	}

	const f32 k_sizeX = renderWidth / (f32)commonCtx->window_width;
	const f32 k_sizeY = renderHeight / (f32)commonCtx->window_height;
	floral::inplace_array<geo2d::VertexPT, 4> vertices;
	vertices.push_back({ { -k_sizeX, k_sizeY }, { 0.0f, 1.0f } });
	vertices.push_back({ { -k_sizeX, -k_sizeY }, { 0.0f, 0.0f } });
	vertices.push_back({ { k_sizeX, -k_sizeY }, { 1.0f, 0.0f } });
	vertices.push_back({ { k_sizeX, k_sizeY }, { 1.0f, 1.0f } });

	floral::inplace_array<s32, 6> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(0);

	m_Quad[0] = helpers::CreateSurfaceGPU(&vertices[0], 4, sizeof(geo2d::VertexPT),
			&indices[0], 6, insigne::buffer_usage_e::static_draw, true);

	m_MemoryArena->free_all();
	floral::relative_path matPath = floral::build_relative_path("cloud_2d_org.mat");
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	bool createResult = mat_loader::CreateMaterial(&m_MSPair[0], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);
	insigne::helpers::assign_uniform_block(m_MSPair[0].material, "ub_Scene", 0, 0, m_SceneUB);

	matPath = floral::build_relative_path("cloud_2d_otm0.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[1], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);
	insigne::helpers::assign_uniform_block(m_MSPair[1].material, "ub_Scene", 0, 0, m_SceneUB);

	matPath = floral::build_relative_path("cloud_2d_otm1.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[2], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);
	insigne::helpers::assign_uniform_block(m_MSPair[2].material, "ub_Scene", 0, 0, m_SceneUB);

	matPath = floral::build_relative_path("tiny_cloud_2d_notex.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[3], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);
	insigne::helpers::assign_uniform_block(m_MSPair[3].material, "ub_Scene", 0, 0, m_SceneUB);

	matPath = floral::build_relative_path("tiny_cloud_2d_tex.mat");
	matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
	createResult = mat_loader::CreateMaterial(&m_MSPair[4], m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(createResult == true);
	insigne::helpers::assign_uniform_block(m_MSPair[4].material, "ub_Scene", 0, 0, m_SceneUB);
}

void ShaderVault::_OnUpdate(const f32 i_deltaMs)
{
	m_SceneData.timeSeconds.x += (i_deltaMs / 1000.0f);
	m_SceneData.timeSeconds.y = i_deltaMs / 1000.0f;
	insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(m_SceneData), 0);

	ImGui::Begin("Controller##ShaderVault");
	ImGui::Text("Choose shaders:");
	if (ImGui::RadioButton("1.1: original", m_MaterialIndex == 0))
	{
		m_MaterialIndex = 0;
	}
	if (ImGui::RadioButton("1.2: optimize algorithm", m_MaterialIndex == 1))
	{
		m_MaterialIndex = 1;
	}
	if (ImGui::RadioButton("1.3: optimize algorithm + mediump float", m_MaterialIndex == 2))
	{
		m_MaterialIndex = 2;
	}
	if (ImGui::RadioButton("2.1: original", m_MaterialIndex == 3))
	{
		m_MaterialIndex = 3;
	}
	if (ImGui::RadioButton("2.1: precomputed", m_MaterialIndex == 4))
	{
		m_MaterialIndex = 4;
	}
	ImGui::End();
}

void ShaderVault::_OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::draw_surface<geo2d::SurfacePT>(m_Quad.vb, m_Quad.ib, m_MSPair[m_MaterialIndex].material);

	RenderImGui();

	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void ShaderVault::_OnCleanUp()
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
