#include "SceneLoader.h"

#include <floral/io/nativeio.h>

#include <calyx/context.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include "InsigneImGui.h"

#include "Graphics/CbSceneLoader.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace perf
{
//-------------------------------------------------------------------

SceneLoader::SceneLoader()
{
}

//-------------------------------------------------------------------

SceneLoader::~SceneLoader()
{
}

//-------------------------------------------------------------------

ICameraMotion* SceneLoader::GetCameraMotion()
{
	return nullptr;
}

//-------------------------------------------------------------------

const_cstr SceneLoader::GetName() const
{
	return k_name;
}

//-------------------------------------------------------------------

void SceneLoader::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);
	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_SceneDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(16));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	f32 aspectRatio = (f32)commonCtx->window_width / (f32)commonCtx->window_height;

	insigne::register_surface_type<geo3d::SurfacePNTT>();
	insigne::register_surface_type<geo2d::SurfacePT>();

	// prebake splitsum
	{
		floral::inplace_array<geo2d::VertexPT, 4> ssVertices;
		ssVertices.push_back({ { -1.0f, -1.0f }, { 0.0f, 0.0f } });
		ssVertices.push_back({ { 1.0f, -1.0f }, { 1.0f, 0.0f } });
		ssVertices.push_back({ { 1.0f, 1.0f }, { 1.0f, 1.0f } });
		ssVertices.push_back({ { -1.0f, 1.0f }, { 0.0f, 1.0f } });

		floral::inplace_array<s32, 6> ssIndices;
		ssIndices.push_back(0);
		ssIndices.push_back(1);
		ssIndices.push_back(2);
		ssIndices.push_back(2);
		ssIndices.push_back(3);
		ssIndices.push_back(0);

		helpers::SurfaceGPU ssQuad = helpers::CreateSurfaceGPU(&ssVertices[0], 4, sizeof(geo2d::VertexPT),
				&ssIndices[0], 6, insigne::buffer_usage_e::static_draw, true);

		m_MemoryArena->free_all();
		mat_loader::MaterialShaderPair splitSumMSPair;
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(floral::path("tests/tech/pbr/pbr_splitsum.mat"), m_MemoryArena);
		const bool ssMaterialResult = mat_loader::CreateMaterial(&splitSumMSPair, matDesc, nullptr, m_MaterialDataArena);
		FLORAL_ASSERT(ssMaterialResult);

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "main_color");
		atm.texture_format = insigne::texture_format_e::hdr_rg;
		atm.texture_dimension = insigne::texture_dimension_e::tex_2d;
		desc.color_attachments->push_back(atm);
		desc.width = 512; desc.height = 512;
		desc.has_depth = false;

		insigne::framebuffer_handle_t fb = insigne::create_framebuffer(desc);
		insigne::dispatch_render_pass();

		insigne::begin_render_pass(fb);
		insigne::draw_surface<geo2d::SurfacePT>(ssQuad.vb, ssQuad.ib, splitSumMSPair.material);
		insigne::end_render_pass(fb);
		insigne::dispatch_render_pass();

		m_SplitSumTexture = insigne::extract_color_attachment(fb, 0);
	}

	const floral::vec3f k_cameraPosition(-79.5f, 30.0f, -10.0f);
	floral::mat4x4f viewMatrix = floral::construct_lookat_point(floral::vec3f(0.0f, 1.0f, 0.0f),
			k_cameraPosition,
			floral::vec3f(0.0f, 10.0f, 0.0f));
	floral::mat4x4f projectionMatrix = floral::construct_perspective(0.01f, 400.0f, 45.0f, aspectRatio);
	m_SceneData.viewProjectionMatrix = projectionMatrix * viewMatrix;
	m_SceneData.cameraPosition = floral::vec4f(k_cameraPosition, 1.0f);
	{
		m_MemoryArena->free_all();
		// loading SH
		floral::file_info shFile = floral::open_file("tests/perf/scene_loader/sponza/materials/env/venice_sunset.cbsh");
		floral::file_stream shStream;
		shStream.buffer = (p8)m_MemoryArena->allocate(shFile.file_size);
		floral::read_all_file(shFile, shStream);
		floral::close_file(shFile);

		for (s32 i = 0; i < 9; i++)
		{
			floral::vec3f shCoeff(0.0f, 0.0f, 0.0f);
			shStream.read(&shCoeff);
			m_SceneData.sh[i] = floral::vec4f(shCoeff, 0.0f);
		}
	}

	{
		insigne::ubdesc_t ubDesc;
		ubDesc.region_size = floral::next_pow2(size(sizeof(SceneData)));
		ubDesc.data = &m_SceneData;
		ubDesc.data_size = sizeof(SceneData);
		ubDesc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_SceneUB = insigne::copy_create_ub(ubDesc);
	}

	m_LightingData.lightDirection = floral::vec4f(floral::normalize(floral::vec3f(0.7399f, 0.6428f, 0.1983f)), 0.0f);
	m_LightingData.lightIntensity = floral::vec4f(5.0f, 5.0f, 5.0f, 0.0f);
	{
		insigne::ubdesc_t ubDesc;
		ubDesc.region_size = floral::next_pow2(size(sizeof(LightingData)));
		ubDesc.data = &m_LightingData;
		ubDesc.data_size = sizeof(LightingData);
		ubDesc.usage = insigne::buffer_usage_e::dynamic_draw;
		m_LightingUB = insigne::copy_create_ub(ubDesc);
	}

	m_MemoryArena->free_all();
	const cbscene::Scene scene = cbscene::LoadSceneData(floral::path("tests/perf/scene_loader/sponza/sponza.gltf.cbscene"),
			m_MemoryArena, m_SceneDataArena);

	m_MaterialArray.reserve(scene.nodesCount, m_SceneDataArena);
	m_ModelDataArray.reserve(scene.nodesCount, m_SceneDataArena);
	for (size i = 0; i < scene.nodesCount; i++)
	{
		c8 modelFile[512];
		sprintf(modelFile, "tests/perf/scene_loader/sponza/%s", scene.nodeFileNames[i]);
		m_MemoryArena->free_all();
		cbmodel::Model<geo3d::VertexPNTT> model = cbmodel::LoadModelData<geo3d::VertexPNTT>(
				floral::path(modelFile),
				cbmodel::VertexAttribute::Position | cbmodel::VertexAttribute::Normal | cbmodel::VertexAttribute::Tangent | cbmodel::VertexAttribute::TexCoord,
				m_MemoryArena, m_SceneDataArena);
		c8 materialPath[512];
		sprintf(materialPath, "tests/perf/scene_loader/sponza/materials/%s.mat", model.materialName);
		mat_loader::MaterialShaderPair msPair = _LoadMaterial(floral::path(materialPath));
		helpers::SurfaceGPU modelGPU = helpers::CreateSurfaceGPU(model.verticesData, model.verticesCount, sizeof(geo3d::VertexPNTT),
				model.indicesData, model.indicesCount, insigne::buffer_usage_e::static_draw, false);
		m_ModelDataArray.push_back(ModelRegistry{ model, msPair, modelGPU });
	}

	m_MemoryArena->free_all();
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(
			floral::path("tests/perf/scene_loader/hdr_pfx.pfx"),
			m_MemoryArena);
	m_PostFXChain.Initialize(pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);

	floral::vec2f fxaaTexelSize(1.0f / commonCtx->window_width, 1.0f / commonCtx->window_height);
	m_PostFXChain.SetValueVec2("ub_FXAAConfigs.iu_TexelSize", fxaaTexelSize);
}

//-------------------------------------------------------------------

void SceneLoader::_OnUpdate(const f32 i_deltaMs)
{
	debugdraw::DrawLine3D(floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec3f(4.0f, 0.0f, 1.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 0.5f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec3f(0.0f, 4.0f, 1.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 0.5f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec3f(0.0f, 0.0f, 5.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 0.5f));
}

//-------------------------------------------------------------------

void SceneLoader::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
	for (size i = 0; i < m_ModelDataArray.get_size(); i++)
	{
		const ModelRegistry& model = m_ModelDataArray[i];
		insigne::draw_surface<geo3d::SurfacePNTT>(model.modelGPU.vb, model.modelGPU.ib, model.msPair.material);
	}
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

void SceneLoader::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);

	insigne::unregister_surface_type<geo2d::SurfacePT>();
	insigne::unregister_surface_type<geo3d::SurfacePNTT>();

	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_SceneDataArena);
	g_StreammingAllocator.free(m_MemoryArena);
}

//-------------------------------------------------------------------

mat_loader::MaterialShaderPair SceneLoader::_LoadMaterial(const floral::path& i_path)
{
	floral::crc_string key(i_path.pm_PathStr);
	for (size i = 0; i < m_MaterialArray.get_size(); i++)
	{
		if (m_MaterialArray[i].key == key)
		{
			return m_MaterialArray[i].msPair;
		}
	}

	m_MemoryArena->free_all();

	MaterialKeyPair materialKeyPair;
	materialKeyPair.key = key;
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(i_path, m_MemoryArena);
	bool matLoadResult = mat_loader::CreateMaterial(&materialKeyPair.msPair, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(matLoadResult == true);
	insigne::helpers::assign_uniform_block(materialKeyPair.msPair.material, "ub_Scene", 0, 0, m_SceneUB);
	insigne::helpers::assign_uniform_block(materialKeyPair.msPair.material, "ub_Lighting", 0, 0, m_LightingUB);
	insigne::helpers::assign_texture(materialKeyPair.msPair.material, "u_SplitSumTex", m_SplitSumTexture);
	m_MaterialArray.push_back(materialKeyPair);

	return m_MaterialArray[m_MaterialArray.get_size() - 1].msPair;
}

//-------------------------------------------------------------------
}
}
