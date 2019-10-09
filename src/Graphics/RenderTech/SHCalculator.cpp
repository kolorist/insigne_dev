#include "SHCalculator.h"

#include <floral/containers/array.h>
#include <floral/comgeo/shapegen.h>
#include <floral/math/transform.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <calyx/context.h>

#include "InsigneImGui.h"
#include "Graphics/prt.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/CBTexDefinitions.h"

// ---------------------------------------------
#include "SHCalculatorShaders.inl"
// ---------------------------------------------

namespace stone
{
namespace tech
{

static const_cstr k_SuiteName = "sh calculator";

//----------------------------------------------

SHCalculator::SHCalculator()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_TemporalArena(nullptr)
	, m_ComputingSH(false)
	, m_SHReady(false)
	, m_Counter(0)
	, m_CamPos(2.0f, 0.0f, 0.0f)
{
}

SHCalculator::~SHCalculator()
{
}

const_cstr SHCalculator::GetName() const
{
	return k_SuiteName;
}

void SHCalculator::OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfaceP>();

	// memory arena
	m_TemporalArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(16));

	{
		// ico sphere
		floral::fixed_array<VertexP, FreelistArena> sphereVertices;
		floral::fixed_array<s32, FreelistArena> sphereIndices;

		sphereVertices.reserve(4096, m_TemporalArena);
		sphereIndices.reserve(8192, m_TemporalArena);

		sphereVertices.resize(4096);
		sphereIndices.resize(8192);

		floral::reset_generation_transforms_stack();
		floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
				0, sizeof(VertexP),
				(s32)floral::geo_vertex_format_e::position,
				&sphereVertices[0], &sphereIndices[0]);
		sphereVertices.resize(genResult.vertices_generated);
		sphereIndices.resize(genResult.indices_generated);

		{
			insigne::vbdesc_t desc;
			desc.region_size = SIZE_KB(32);
			desc.stride = sizeof(VertexP);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_ProbeVB = insigne::create_vb(desc);
			insigne::copy_update_vb(m_ProbeVB, &sphereVertices[0], sphereVertices.get_size(), sizeof(VertexP), 0);
		}

		{
			insigne::ibdesc_t desc;
			desc.region_size = SIZE_KB(16);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_ProbeIB = insigne::create_ib(desc);
			insigne::copy_update_ib(m_ProbeIB, &sphereIndices[0], sphereIndices.get_size(), 0);
		}
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = 512;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_UB = insigne::create_ub(desc);

		calyx::context_attribs* commonCtx = calyx::get_context_attribs();
		m_CameraMotion.SetScreenResolution(commonCtx->window_width, commonCtx->window_height);

		m_SceneData.WVP = m_CameraMotion.GetWVP();
		memset(m_SceneData.SH, 0, sizeof(m_SceneData.SH));
		insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_ProbeFS);
		desc.vs_path = floral::path("/demo/probe_vs");
		desc.fs_path = floral::path("/demo/probe_fs");

		m_ProbeShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ProbeShader, m_ProbeMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_ProbeMaterial, "ub_Scene");
			m_ProbeMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 256, m_UB };
		}
	}

	{
#if 0
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/grace_probe.cbtex");
		floral::file_stream dataStream;
		dataStream.buffer = (p8)m_TemporalArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		cb::CBTexture2DHeader header;
		dataStream.read(&header);

		FLORAL_ASSERT(header.colorRange == cb::ColorRange::HDR);
#endif
		const u32 res = 512;

		insigne::texture_desc_t texDesc;
		//texDesc.width = header.resolution;
		//texDesc.height = header.resolution;
		texDesc.width = res;
		texDesc.height = res;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::linear;
		texDesc.mag_filter = insigne::filtering_e::linear;
		texDesc.dimension = insigne::texture_dimension_e::tex_2d;
		texDesc.has_mipmap = false;

		const size dataSize = insigne::prepare_texture_desc(texDesc);

		m_TextureData = (f32*)g_PersistanceResourceAllocator.allocate(dataSize);
		m_RadTextureData = (f32*)g_PersistanceResourceAllocator.allocate(dataSize);
		memset(m_RadTextureData, 0, dataSize);
		m_IrrTextureData = (f32*)g_PersistanceResourceAllocator.allocate(dataSize);
		memset(m_IrrTextureData, 0, dataSize);

		//m_TextureResolution = header.resolution;
		m_TextureResolution = res;
#if 0
		dataStream.read_bytes(m_TextureData, dataSize);
#else
		generate_debug_light_probe(m_TextureData, res, 500, 0);
#endif
		memcpy(texDesc.data, m_TextureData, dataSize);
		m_Texture = insigne::create_texture(texDesc);
	}
}

void SHCalculator::OnUpdate(const f32 i_deltaMs)
{
	// DebugUI
	ImGui::Begin("Controller");
	ImGui::Text("grace_probe");
	ImGui::Image(&m_Texture, ImVec2(128, 128));

	const u32 res = 512;
	if (ImGui::Button("Generate PosX"))
	{
		generate_debug_light_probe(m_TextureData, res, 500, 0);
		insigne::copy_update_texture(m_Texture, m_TextureData);
	}
	if (ImGui::Button("Generate NegX"))
	{
		generate_debug_light_probe(m_TextureData, res, 500, 1);
		insigne::copy_update_texture(m_Texture, m_TextureData);
	}
	if (ImGui::Button("Generate PosY"))
	{
		generate_debug_light_probe(m_TextureData, res, 500, 2);
		insigne::copy_update_texture(m_Texture, m_TextureData);
	}
	if (ImGui::Button("Generate NegY"))
	{
		generate_debug_light_probe(m_TextureData, res, 500, 3);
		insigne::copy_update_texture(m_Texture, m_TextureData);
	}
	if (ImGui::Button("Generate PosZ"))
	{
		generate_debug_light_probe(m_TextureData, res, 500, 4);
		insigne::copy_update_texture(m_Texture, m_TextureData);
	}
	if (ImGui::Button("Generate NegZ"))
	{
		generate_debug_light_probe(m_TextureData, res, 500, 5);
		insigne::copy_update_texture(m_Texture, m_TextureData);
	}

	if (ImGui::Button("Compute SH Coeffs"))
	{
		if (!m_ComputingSH)
		{
			m_ComputingSH = true;
			m_SHReady = false;
			m_TemporalArena->free_all();
			m_SHComputeTaskData.InputTexture = m_TextureData;
			m_SHComputeTaskData.OutputRadianceTex = m_RadTextureData;
			m_SHComputeTaskData.OutputIrradianceTex = m_IrrTextureData;
			m_SHComputeTaskData.Resolution = m_TextureResolution;
			m_SHComputeTaskData.LocalMemoryArena = m_TemporalArena->allocate_arena<LinearArena>(SIZE_MB(4));

			m_Counter.store(1);
			refrain2::Task newTask;
			newTask.pm_Instruction = &SHCalculator::ComputeSHCoeffs;
			newTask.pm_Data = (voidptr)&m_SHComputeTaskData;
			newTask.pm_Counter = &m_Counter;
			refrain2::g_TaskManager->PushTask(newTask);
		}
	}

	if (m_SHReady)
	{
		ImGui::Text("radiance");
		ImGui::SameLine(150, 20);
		ImGui::Text("irradiance");
		ImGui::Image(&m_RadianceTexture, ImVec2(128, 128));
		ImGui::SameLine(150, 20);
		ImGui::Image(&m_IrradianceTexture, ImVec2(128, 128));
		ImGui::Text("sh coeffs");
		for (u32 i = 0; i < 9; i++)
		{
			c8 bandStr[128];
			memset(bandStr, 0, 128);
			sprintf(bandStr, "coeff %d", i + 1);
			ImGui::InputFloat3(bandStr, &m_SHComputeTaskData.OutputCoeffs[i].x, 5, ImGuiInputTextFlags_ReadOnly);
			m_SceneData.SH[i] = floral::vec4f(m_SHComputeTaskData.OutputCoeffs[i], 0.0f);
		}
		insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);
	}
	ImGui::End();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

	if (m_ComputingSH)
	{
		if (refrain2::CheckForCounter(m_Counter, 0))
		{
			// upload!!!
			{
				insigne::texture_desc_t texDesc;
				texDesc.width = m_TextureResolution;
				texDesc.height = m_TextureResolution;
				texDesc.format = insigne::texture_format_e::hdr_rgb;
				texDesc.min_filter = insigne::filtering_e::linear;
				texDesc.mag_filter = insigne::filtering_e::linear;
				texDesc.dimension = insigne::texture_dimension_e::tex_2d;
				texDesc.has_mipmap = false;

				const size dataSize = insigne::prepare_texture_desc(texDesc);
				memcpy(texDesc.data, m_IrrTextureData, dataSize);
				m_IrradianceTexture = insigne::create_texture(texDesc);
			}

			{
				insigne::texture_desc_t texDesc;
				texDesc.width = m_TextureResolution;
				texDesc.height = m_TextureResolution;
				texDesc.format = insigne::texture_format_e::hdr_rgb;
				texDesc.min_filter = insigne::filtering_e::linear;
				texDesc.mag_filter = insigne::filtering_e::linear;
				texDesc.dimension = insigne::texture_dimension_e::tex_2d;
				texDesc.has_mipmap = false;

				const size dataSize = insigne::prepare_texture_desc(texDesc);
				memcpy(texDesc.data, m_RadTextureData, dataSize);
				m_RadianceTexture = insigne::create_texture(texDesc);
			}
			m_ComputingSH = false;
			m_SHReady = true;
		}
	}

	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(2.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 2.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
}

void SHCalculator::OnRender(const f32 i_deltaMs)
{
	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfaceP>(m_ProbeVB, m_ProbeIB, m_ProbeMaterial);
	debugdraw::Render(m_SceneData.WVP);
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void SHCalculator::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	g_StreammingAllocator.free(m_TemporalArena);
	m_TemporalArena = nullptr;

	insigne::unregister_surface_type<SurfaceP>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

//----------------------------------------------
refrain2::Task SHCalculator::ComputeSHCoeffs(voidptr i_data)
{
	SHComputeData* input = (SHComputeData*)i_data;
	s32 sqrtNSamples = 100;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = (sh_sample*)input->LocalMemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	highp_vec3_t shResult[9];
	sh_project_light_image(input->InputTexture, input->Resolution, NSamples, 9, samples, shResult);

	for (s32 i = 0; i < 9; i++)
	{
		CLOVER_DEBUG("(%f; %f; %f)", shResult[i].x, shResult[i].y, shResult[i].z);

		input->OutputCoeffs[i].x = (f32)shResult[i].x;
		input->OutputCoeffs[i].y = (f32)shResult[i].y;
		input->OutputCoeffs[i].z = (f32)shResult[i].z;
	}

	reconstruct_sh_radiance_light_probe(shResult, input->OutputRadianceTex, input->Resolution, 1024);
	reconstruct_sh_irradiance_light_probe(shResult, input->OutputIrradianceTex, input->Resolution, 1024, 0.4f);

	CLOVER_VERBOSE("Compute finished");
	return refrain2::Task();
}

}
}
