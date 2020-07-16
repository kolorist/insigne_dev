#include "PBRHelmet.h"

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <calyx/context.h>

#include <clover/Logger.h>

#include <floral/math/utils.h>

#include "InsigneImGui.h"

#include "Graphics/MaterialParser.h"
#include "Graphics/TextureLoader.h"
#include "Graphics/GLTFLoader.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/CbModelLoader.h"
#include "Graphics/DebugDrawer.h"

namespace stone
{
namespace tech
{
// -------------------------------------------------------------------

PBRHelmet::PBRHelmet()
	: m_frameIndex(0)
	, m_elapsedTime(0.0f)
{
}

PBRHelmet::~PBRHelmet()
{
}

ICameraMotion* PBRHelmet::GetCameraMotion()
{
	return nullptr;
}

const_cstr PBRHelmet::GetName() const
{
	return k_name;
}

void PBRHelmet::_OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_name);

	floral::relative_path wdir = floral::build_relative_path("tests/tech/pbr");
	floral::push_directory(m_FileSystem, wdir);

	// register surfaces
	insigne::register_surface_type<geo3d::SurfacePNTT>();
	insigne::register_surface_type<geo2d::SurfacePT>();

	m_MemoryArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));
	m_MaterialDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_KB(256));
	m_ModelDataArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(1));
	m_PostFXArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(4));

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
		floral::relative_path matPath = floral::build_relative_path("pbr_splitsum.mat");
		mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem, matPath, m_MemoryArena);
		const bool ssMaterialResult = mat_loader::CreateMaterial(&splitSumMSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
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

	m_MemoryArena->free_all();
	cbmodel::Model<geo3d::VertexPNTT> model = cbmodel::LoadModelData<geo3d::VertexPNTT>(m_FileSystem,
			floral::build_relative_path("DamagedHelmet.gltf_mesh_0.cbmodel"),
			cbmodel::VertexAttribute::Position | cbmodel::VertexAttribute::Normal | cbmodel::VertexAttribute::Tangent | cbmodel::VertexAttribute::TexCoord,
			m_MemoryArena, m_ModelDataArena);

	m_SurfaceGPU = helpers::CreateSurfaceGPU(model.verticesData, model.verticesCount, sizeof(geo3d::VertexPNTT),
			model.indicesData, model.indicesCount, insigne::buffer_usage_e::static_draw, false);

	const floral::vec3f k_camPos = floral::vec3f(0.0f, 5.1f, 7.0f);
	m_view = floral::construct_lookat_point(
			floral::vec3f(0.0f, 1.0f, 0.0f),
			k_camPos,
			floral::vec3f(0.0f, 0.0f, 0.0f));
	m_projection = floral::construct_perspective(
			0.01f, 100.0f, 16.3125f, 16.0f / 9.0f);
	m_SceneData.cameraPos = floral::vec4f(k_camPos, 0.0f);
	m_SceneData.viewProjectionMatrix = m_projection * m_view;
	{
		// loading SH
		m_MemoryArena->free_all();
		floral::file_info shFile = floral::open_file_read(m_FileSystem, floral::build_relative_path("out.cbsh"));
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

	insigne::ubdesc_t desc;
	desc.region_size = floral::next_pow2(size(sizeof(SceneData)));
	desc.data = &m_SceneData;
	desc.data_size = sizeof(SceneData);
	desc.usage = insigne::buffer_usage_e::dynamic_draw;
	m_SceneUB = insigne::copy_create_ub(desc);

	m_MemoryArena->free_all();
	mat_parser::MaterialDescription matDesc = mat_parser::ParseMaterial(m_FileSystem,
			floral::build_relative_path("pbr_helmet.mat"), m_MemoryArena);

	const bool pbrMaterialResult = mat_loader::CreateMaterial(&m_MSPair, m_FileSystem, matDesc, m_MemoryArena, m_MaterialDataArena);
	FLORAL_ASSERT(pbrMaterialResult == true);

	insigne::helpers::assign_uniform_block(m_MSPair.material, "ub_Scene", 0, 0, m_SceneUB);
	insigne::helpers::assign_texture(m_MSPair.material, "u_SplitSumTex", m_SplitSumTexture);

	m_MemoryArena->free_all();
	pfx_parser::PostEffectsDescription pfxDesc = pfx_parser::ParsePostFX(m_FileSystem,
			floral::build_relative_path("hdr_pfx.pfx"),
			m_MemoryArena);
	calyx::context_attribs* commonCtx = calyx::get_context_attribs();
	m_PostFXChain.Initialize(m_FileSystem, pfxDesc, floral::vec2f(commonCtx->window_width, commonCtx->window_height), m_PostFXArena);

	floral::vec2f fxaaTexelSize(1.0f / commonCtx->window_width, 1.0f / commonCtx->window_height);
	m_PostFXChain.SetValueVec2("ub_FXAAConfigs.iu_TexelSize", fxaaTexelSize);
}

void PBRHelmet::_OnUpdate(const f32 i_deltaMs)
{
	ImGui::Begin("Controller##PBRHelmet");
	static bool autoRotate = false;
	ImGui::Checkbox("Auto rotate", &autoRotate);
	ImGui::End();

	// coordinate
	const s32 k_gridRange = 6;
	const f32 k_gridSpacing = 0.25f;
	const f32 maxCoord = k_gridRange * k_gridSpacing;
	for (s32 i = -k_gridRange; i <= k_gridRange; i++)
	{
		debugdraw::DrawLine3D(floral::vec3f(i * k_gridSpacing, 0.0f, -maxCoord),
				floral::vec3f(i * k_gridSpacing, 0.0f, maxCoord),
				floral::vec4f(0.3f, 0.3f, 0.3f, 0.3f));
		debugdraw::DrawLine3D(floral::vec3f(-maxCoord, 0.0f, i * k_gridSpacing),
				floral::vec3f(maxCoord, 0.0f, i * k_gridSpacing),
				floral::vec4f(0.3f, 0.3f, 0.3f, 0.3f));
	}

	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(1.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 1.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 1.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));

	if (m_PostFXChain.IsTAAEnabled())
	{
		calyx::context_attribs* commonCtx = calyx::get_context_attribs();

		if (autoRotate)
		{
			m_elapsedTime += i_deltaMs;
			const f32 k_factor = floral::to_radians(m_elapsedTime / 40.0f);
			const floral::vec3f k_camPos = floral::vec3f(7.0f * sinf(k_factor), 5.1f, 7.0f * cosf(k_factor));
			m_view = floral::construct_lookat_point(
					floral::vec3f(0.0f, 1.0f, 0.0f),
					k_camPos,
					floral::vec3f(0.0f, 0.0f, 0.0f));
			m_SceneData.cameraPos = floral::vec4f(k_camPos, 0.0f);
		}

		floral::mat4x4f wvp;
		if (!autoRotate)
		{
			wvp = pfx_chain::get_jittered_matrix(m_frameIndex, commonCtx->window_width, commonCtx->window_height) * m_projection * m_view;
		}
		else
		{
			wvp = m_projection * m_view;
		}
		m_SceneData.viewProjectionMatrix = wvp;
		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
	}
	m_frameIndex++;
}

void PBRHelmet::_OnRender(const f32 i_deltaMs)
{
	m_PostFXChain.BeginMainOutput();
	insigne::draw_surface<geo3d::SurfacePNTT>(m_SurfaceGPU.vb, m_SurfaceGPU.ib, m_MSPair.material);
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

void PBRHelmet::_OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_name);
	m_PostFXChain.CleanUp();

	insigne::unregister_surface_type<geo2d::SurfacePT>();
	insigne::unregister_surface_type<geo3d::SurfacePNTT>();

	g_StreammingAllocator.free(m_PostFXArena);
	g_StreammingAllocator.free(m_ModelDataArena);
	g_StreammingAllocator.free(m_MaterialDataArena);
	g_StreammingAllocator.free(m_MemoryArena);

	floral::pop_directory(m_FileSystem);
}

// -------------------------------------------------------------------
}
}
