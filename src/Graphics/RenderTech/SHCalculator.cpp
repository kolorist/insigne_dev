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
#include "Graphics/stb_image.h"
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

enum class ProjectionScheme
{
	LightProbe,
	HStrip,
	Equirectangular,
	Invalid
};

static const_cstr k_ProjectionSchemeStr[] = {
	"light probe",
	"h strip",
	"equirectangular",
	"invalid"
};

static const_cstr k_Images[] = {
	"<none>",
	"grace_probe.hdr",
	"alexs_appartment_probe.hdr",
	"arboretum_probe.hdr",
	"uvchecker_hstrip.hdr",
	"alexs_appartment_hstrip.hdr",
	"alexs_appartment_equi.hdr"
};

static const_cstr k_SuiteName = "ibl calculator";

//----------------------------------------------

const ProjectionScheme get_projection_from_size(const s32 i_width, const s32 i_height)
{
	const f32 aspectRatio = (f32)i_width / (f32)i_height;
	if (i_width == i_height)
	{
		// faceSize = 512;
		FLORAL_ASSERT(i_width == 512);
		return ProjectionScheme::LightProbe;
	}
	else if (i_height * 6 == i_width)
	{
		// faceSize = 256 * 256
		FLORAL_ASSERT(i_height == 256);
		return ProjectionScheme::HStrip;
	}
	else if (fabs(aspectRatio - 2.0) < 0.0001)
	{
		// faceSize = 256* 256
		FLORAL_ASSERT(i_height == 512);
		return ProjectionScheme::Equirectangular;
	}
	return ProjectionScheme::Invalid;
}

void trim_image(f32* i_imgData, f32* o_outData, const s32 i_x, const s32 i_y, const s32 i_width, const s32 i_height,
		const s32 i_imgWidth, const s32 i_imgHeight, const s32 i_channelCount)
{
	s32 outScanline = 0;
	for (s32 i = i_y; i < (i_y + i_height); i++) {
		memcpy(
				&o_outData[outScanline * i_width * i_channelCount],
				&i_imgData[(i * i_imgWidth + i_x) * i_channelCount],
				i_height * i_channelCount * sizeof(f32));
		outScanline++;
	}
}

//----------------------------------------------

SHCalculator::SHCalculator()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_TemporalArena(nullptr)
	, m_ComputingSH(false)
	, m_SHReady(false)
	, m_SpecReady(false)
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
	insigne::register_surface_type<SurfacePT>();

	// memory arena
	m_TemporalArena = g_StreammingAllocator.allocate_arena<FreelistArena>(SIZE_MB(48));

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
				3, 0, sizeof(VertexP),
				floral::geo_vertex_format_e::position,
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
		m_TextureData = (f32*)g_PersistanceResourceAllocator.allocate(SIZE_MB(6));
		m_RadTextureData = (f32*)g_PersistanceResourceAllocator.allocate(SIZE_KB(200));
		m_IrrTextureData = (f32*)g_PersistanceResourceAllocator.allocate(SIZE_KB(200));

		insigne::texture_desc_t texDesc;
		texDesc.width = 128;
		texDesc.height = 128;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::linear;
		texDesc.mag_filter = insigne::filtering_e::linear;
		texDesc.dimension = insigne::texture_dimension_e::tex_2d;
		texDesc.has_mipmap = false;
		texDesc.data = nullptr;

		m_IrradianceTexture = insigne::create_texture(texDesc);
		m_RadianceTexture = insigne::create_texture(texDesc);

		texDesc.width = 512;
		texDesc.height = 512;
		m_Texture[0] = insigne::create_texture(texDesc);

		texDesc.width = 256 * 6;
		texDesc.height = 256;
		m_Texture[1] = insigne::create_texture(texDesc);

		texDesc.width = 1024;
		texDesc.height = 512;
		m_Texture[2] = insigne::create_texture(texDesc);
		m_CurrentInputTexture = m_Texture[0];
		m_CurrentPreviewTexture = m_CurrentInputTexture;

		texDesc.width = 256;
		texDesc.height = 256;
		texDesc.dimension = insigne::texture_dimension_e::tex_cube;
		m_CurrentInputTexture3D = insigne::create_texture(texDesc);
	}

	// tonemap
	{
		calyx::context_attribs* commonCtx = calyx::get_context_attribs();

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));

		desc.width = commonCtx->window_width;
		desc.height = commonCtx->window_height;

		m_PostFXBuffer = insigne::create_framebuffer(desc);
	}

	floral::inplace_array<VertexPT, 4> ssVertices;
	ssVertices.push_back(VertexPT { { -1.0f, -1.0f }, { 0.0f, 0.0f } });
	ssVertices.push_back(VertexPT { { 1.0f, -1.0f }, { 1.0f, 0.0f } });
	ssVertices.push_back(VertexPT { { 1.0f, 1.0f }, { 1.0f, 1.0f } });
	ssVertices.push_back(VertexPT { { -1.0f, 1.0f }, { 0.0f, 1.0f } });

	floral::inplace_array<s32, 6> ssIndices;
	ssIndices.push_back(0);
	ssIndices.push_back(1);
	ssIndices.push_back(2);
	ssIndices.push_back(2);
	ssIndices.push_back(3);
	ssIndices.push_back(0);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPT);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_SSVB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_SSVB, &ssVertices[0], ssVertices.get_size(), sizeof(VertexPT), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_SSIB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_SSIB, &ssIndices[0], ssIndices.get_size(), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_ToneMapVS);
		strcpy(desc.fs, s_ToneMapFS);
		desc.vs_path = floral::path("/demo/tonemap_vs");
		desc.fs_path = floral::path("/demo/tonemap_fs");

		m_ToneMapShader = insigne::create_shader(desc);
		insigne::infuse_material(m_ToneMapShader, m_ToneMapMaterial);

		{
			ssize texSlot = insigne::get_material_texture_slot(m_ToneMapMaterial, "u_Tex");
			m_ToneMapMaterial.textures[texSlot].value = insigne::extract_color_attachment(m_PostFXBuffer, 0);
		}
	}

	// preview shader and material
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_Preview_LightProbeFS);
		desc.vs_path = floral::path("/demo/preview_probe_vs");
		desc.fs_path = floral::path("/demo/preview_lightprobe_fs");

		m_PreviewShader[0] = insigne::create_shader(desc);
		insigne::infuse_material(m_PreviewShader[0], m_PreviewMaterial[0]);
		insigne::helpers::assign_uniform_block(m_PreviewMaterial[0], "ub_Scene", 0, 256, m_UB);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler_cube));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_Preview_CubeMapHStripFS);
		desc.vs_path = floral::path("/demo/preview_probe_vs");
		desc.fs_path = floral::path("/demo/preview_cubemap_fs");

		m_PreviewShader[1] = insigne::create_shader(desc);
		insigne::infuse_material(m_PreviewShader[1], m_PreviewMaterial[1]);
		insigne::helpers::assign_uniform_block(m_PreviewMaterial[1], "ub_Scene", 0, 256, m_UB);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_ProbeVS);
		strcpy(desc.fs, s_Preview_LatLongFS);
		desc.vs_path = floral::path("/demo/preview_probe_vs");
		desc.fs_path = floral::path("/demo/preview_latlong_fs");

		m_PreviewShader[2] = insigne::create_shader(desc);
		insigne::infuse_material(m_PreviewShader[2], m_PreviewMaterial[2]);
		insigne::helpers::assign_uniform_block(m_PreviewMaterial[2], "ub_Scene", 0, 256, m_UB);
	}

	m_CurrentPreviewMat = &m_ProbeMaterial;
}

void SHCalculator::OnUpdate(const f32 i_deltaMs)
{
	// DebugUI
	ImGui::Begin("Controller");

	static s32 currentImage = 0;
	if (ImGui::Combo("Input image", &currentImage, k_Images, IM_ARRAYSIZE(k_Images)))
	{
		_LoadHDRImage(k_Images[currentImage]);
	}

#if 0
	ImGui::Text("debug sh");
	if (ImGui::Button("PosX")) _ComputeDebugSH(0);
	ImGui::SameLine();
	if (ImGui::Button("NegX")) _ComputeDebugSH(1);
	ImGui::SameLine();
	if (ImGui::Button("PosY")) _ComputeDebugSH(2);
	ImGui::SameLine();
	if (ImGui::Button("NegY")) _ComputeDebugSH(3);
	ImGui::SameLine();
	if (ImGui::Button("PosZ")) _ComputeDebugSH(4);
	ImGui::SameLine();
	if (ImGui::Button("NegZ")) _ComputeDebugSH(5);
#endif
	ImGui::Separator();

	if (m_ImgLoaded)
	{
		ImGui::Text("Image preview (LDR, color clamped to [0..1])");
		s32 height = 128;
		s32 width = s32(m_InputTextureSize.x * (f32)height / m_InputTextureSize.y);
		ImGui::Image(&m_CurrentInputTexture, ImVec2(width, height));
		ImGui::Text("Projection: %s", k_ProjectionSchemeStr[m_Projection]);
		ImGui::Text("HDR range: [%4.2f, %4.2f, %4.2f] - [%4.2f, %4.2f, %4.2f]",
				m_MinHDR.x, m_MinHDR.y, m_MinHDR.z,
				m_MaxHDR.x, m_MaxHDR.y, m_MaxHDR.z);
		if (ImGui::Button("Preview 3D##cube"))
		{
			insigne::helpers::assign_texture(m_PreviewMaterial[m_Projection], "u_Tex", m_CurrentPreviewTexture);
			m_CurrentPreviewMat = &m_PreviewMaterial[m_Projection];
		}
	}

	if (ImGui::Button("Calculate"))
	{
		_ComputeSH();
	}

	ImGui::Separator();
	if (m_SpecReady)
	{
		ImGui::Text("PMREM");
	}
	else
	{
		ImGui::Text("Please calculate the PMREM");
	}

	ImGui::Separator();

	if (m_SHReady)
	{
		ImGui::Text("SH coeffs:");
		for (u32 i = 0; i < 9; i++)
		{
			c8 bandStr[128];
			memset(bandStr, 0, 128);
			sprintf(bandStr, "#%d", i + 1);
			ImGui::InputFloat3(bandStr, &m_SHComputeTaskData.OutputCoeffs[i].x, 5, ImGuiInputTextFlags_ReadOnly);
			m_SceneData.SH[i] = floral::vec4f(m_SHComputeTaskData.OutputCoeffs[i], 0.0f);
		}
		insigne::copy_update_ub(m_UB, &m_SceneData, sizeof(SceneData), 0);

		ImGui::Text("Results from SH Coeffs:");
		ImGui::Text("Radiance");
		ImGui::SameLine(150, 20);
		ImGui::Text("Irradiance");
		ImGui::Image(&m_RadianceTexture, ImVec2(128, 128));
		ImGui::SameLine(150, 20);
		ImGui::Image(&m_IrradianceTexture, ImVec2(128, 128));
	}
	else
	{
		ImGui::Text("Please calculate the SH");
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
				insigne::copy_update_texture(m_IrradianceTexture, m_IrrTextureData, dataSize);
				insigne::copy_update_texture(m_RadianceTexture, m_RadTextureData, dataSize);
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
	insigne::begin_render_pass(m_PostFXBuffer);
	insigne::draw_surface<SurfaceP>(m_ProbeVB, m_ProbeIB, *m_CurrentPreviewMat);
	debugdraw::Render(m_SceneData.WVP);
	insigne::end_render_pass(m_PostFXBuffer);
	insigne::dispatch_render_pass();

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePT>(m_SSVB, m_SSIB, m_ToneMapMaterial);
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

	insigne::unregister_surface_type<SurfacePT>();
	insigne::unregister_surface_type<SurfaceP>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

//----------------------------------------------

void SHCalculator::_ComputeSH()
{
	if (m_ComputingSH || !m_ImgLoaded)
	{
		return;
	}

	m_ComputingSH = true;
	m_SHReady = false;

	m_TemporalArena->free_all();
	m_SHComputeTaskData.InputTexture = m_TextureData;
	m_SHComputeTaskData.OutputRadianceTex = m_RadTextureData;
	m_SHComputeTaskData.OutputIrradianceTex = m_IrrTextureData;
	m_SHComputeTaskData.Resolution = m_TextureResolution;		// face size of the input image
	m_SHComputeTaskData.Projection = m_Projection;
	m_SHComputeTaskData.LocalMemoryArena = m_TemporalArena->allocate_arena<LinearArena>(SIZE_MB(4));
	m_SHComputeTaskData.DebugFaceIndex = 0;

	m_Counter.store(1);
	refrain2::Task newTask;
	newTask.pm_Instruction = &SHCalculator::ComputeSHCoeffs;
	newTask.pm_Data = (voidptr)&m_SHComputeTaskData;
	newTask.pm_Counter = &m_Counter;
	refrain2::g_TaskManager->PushTask(newTask);
}

void SHCalculator::_ComputeDebugSH(const u32 i_faceIdx)
{
	if (m_ComputingSH)
	{
		return;
	}

	m_ComputingSH = true;
	m_SHReady = false;

	m_TemporalArena->free_all();
	m_SHComputeTaskData.InputTexture = nullptr;
	m_SHComputeTaskData.OutputRadianceTex = m_RadTextureData;
	m_SHComputeTaskData.OutputIrradianceTex = m_IrrTextureData;
	m_SHComputeTaskData.Resolution = m_TextureResolution;
	m_SHComputeTaskData.LocalMemoryArena = m_TemporalArena->allocate_arena<LinearArena>(SIZE_MB(4));
	m_SHComputeTaskData.DebugFaceIndex = i_faceIdx;

	m_Counter.store(1);
	refrain2::Task newTask;
	newTask.pm_Instruction = &SHCalculator::ComputeDebugSHCoeffs;
	newTask.pm_Data = (voidptr)&m_SHComputeTaskData;
	newTask.pm_Counter = &m_Counter;
	refrain2::g_TaskManager->PushTask(newTask);
}

void SHCalculator::_LoadHDRImage(const_cstr i_fileName)
{
	if (strcmp(i_fileName, "<none>") == 0)
	{
		m_ComputingSH = false;
		m_SHReady = false;
		m_ImgLoaded = false;
		return;
	}

	c8 imagePath[1024];
	sprintf(imagePath, "gfx/envi/textures/demo/%s", i_fileName);
	CLOVER_DEBUG("Loading image: %s", imagePath);
	s32 x, y, n;
	f32* imageData = stbi_loadf(imagePath, &x, &y, &n, 0);

	ProjectionScheme projScheme = get_projection_from_size(x, y);
	FLORAL_ASSERT(projScheme != ProjectionScheme::Invalid);
	FLORAL_ASSERT(n == 3);

	switch (projScheme)
	{
		case ProjectionScheme::LightProbe:
			m_Projection = 0;
			m_TextureResolution = 512;
			m_InputTextureSize = floral::vec2i(512, 512);
			break;
		case ProjectionScheme::HStrip:
			m_Projection = 1;
			m_TextureResolution = 256;
			m_InputTextureSize = floral::vec2i(256 * 6, 256);
			break;
		case ProjectionScheme::Equirectangular:
			m_Projection = 2;
			m_TextureResolution = 256;
			m_InputTextureSize = floral::vec2i(1024, 512);
			break;
		default:
			m_TextureResolution = 0;
			m_InputTextureSize = floral::vec2i(0, 0);
			FLORAL_ASSERT(false);
			break;
	}

	m_CurrentInputTexture = m_Texture[m_Projection];
	m_CurrentPreviewTexture = m_Texture[m_Projection];

	insigne::texture_desc_t texDesc;
	// in order to fully display loaded image, we have to transfer all the image data
	texDesc.width = m_InputTextureSize.x;
	texDesc.height = m_InputTextureSize.y;
	texDesc.format = insigne::texture_format_e::hdr_rgb;
	texDesc.min_filter = insigne::filtering_e::linear;
	texDesc.mag_filter = insigne::filtering_e::linear;
	texDesc.dimension = insigne::texture_dimension_e::tex_2d;
	texDesc.has_mipmap = false;

	const size dataSize = insigne::prepare_texture_desc(texDesc);
	insigne::copy_update_texture(m_CurrentInputTexture, imageData, dataSize);

	if (m_Projection == 1)
	{
		f32* imgData3D = m_TemporalArena->allocate_array<f32>(256 * 256 * 3 * 6);

		f32* pOutBuffer = imgData3D;
		for (s32 i = 0; i < 6; i++)
		{
			trim_image(imageData, pOutBuffer, i * 256, 0,
					256, 256,
					m_InputTextureSize.x, m_InputTextureSize.y, 3);
			pOutBuffer += (256 * 256 * 3);
		}

		insigne::texture_desc_t texDesc;
		texDesc.width = 256;
		texDesc.height = 256;
		texDesc.format = insigne::texture_format_e::hdr_rgb;
		texDesc.min_filter = insigne::filtering_e::linear;
		texDesc.mag_filter = insigne::filtering_e::linear;
		texDesc.dimension = insigne::texture_dimension_e::tex_cube;
		texDesc.has_mipmap = false;

		const size dataSize = insigne::prepare_texture_desc(texDesc);
		FLORAL_ASSERT(dataSize == 256 * 256 * 3 * 6 * sizeof(f32));
		insigne::copy_update_texture(m_CurrentInputTexture3D, imgData3D, dataSize);
		m_TemporalArena->free(imgData3D);
		m_CurrentPreviewTexture = m_CurrentInputTexture3D;
	}

	m_MinHDR = floral::vec3f(9999.0f, 9999.0f, 9999.0f);
	m_MaxHDR = floral::vec3f(0.0f, 0.0f, 0.0f);
	for (s32 i = 0; i < x * y; i++)
	{
		// red
		if (imageData[i * 3] < m_MinHDR.x) m_MinHDR.x = imageData[i * 3];
		if (imageData[i * 3] > m_MaxHDR.x) m_MaxHDR.x = imageData[i * 3];

		// green
		if (imageData[i * 3 + 1] < m_MinHDR.y) m_MinHDR.y = imageData[i * 3 + 1];
		if (imageData[i * 3 + 1] > m_MaxHDR.y) m_MaxHDR.y = imageData[i * 3 + 1];

		// blue
		if (imageData[i * 3 + 2] < m_MinHDR.z) m_MinHDR.z = imageData[i * 3 + 2];
		if (imageData[i * 3 + 2] > m_MaxHDR.z) m_MaxHDR.z = imageData[i * 3 + 2];
	}

	memcpy(m_TextureData, imageData, dataSize);

	stbi_image_free(imageData);
	m_ImgLoaded = true;
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
	sh_project_light_image(input->InputTexture, input->Projection, input->Resolution, NSamples, 9, samples, shResult);

	for (s32 i = 0; i < 9; i++)
	{
		CLOVER_DEBUG("(%f; %f; %f)", shResult[i].x, shResult[i].y, shResult[i].z);

		input->OutputCoeffs[i].x = (f32)shResult[i].x;
		input->OutputCoeffs[i].y = (f32)shResult[i].y;
		input->OutputCoeffs[i].z = (f32)shResult[i].z;
	}

	reconstruct_sh_radiance_light_probe(shResult, input->OutputRadianceTex, 128, 1024);
	reconstruct_sh_irradiance_light_probe(shResult, input->OutputIrradianceTex, 128, 1024, 0.4f);

	CLOVER_VERBOSE("Compute finished");
	return refrain2::Task();
}

refrain2::Task SHCalculator::ComputeDebugSHCoeffs(voidptr i_data)
{
	SHComputeData* input = (SHComputeData*)i_data;
	s32 sqrtNSamples = 100;
	s32 NSamples = sqrtNSamples * sqrtNSamples;
	sh_sample* samples = (sh_sample*)input->LocalMemoryArena->allocate_array<sh_sample>(NSamples);
	sh_setup_spherical_samples(samples, sqrtNSamples);

	highp_vec3_t shResult[9];
	debug_sh_project_light_image(input->DebugFaceIndex, input->Resolution, NSamples, 9, samples, shResult);

	for (s32 i = 0; i < 9; i++)
	{
		CLOVER_DEBUG("(%f; %f; %f)", shResult[i].x, shResult[i].y, shResult[i].z);

		input->OutputCoeffs[i].x = (f32)shResult[i].x;
		input->OutputCoeffs[i].y = (f32)shResult[i].y;
		input->OutputCoeffs[i].z = (f32)shResult[i].z;
	}

	reconstruct_sh_radiance_light_probe(shResult, input->OutputRadianceTex, 128, 1024);
	reconstruct_sh_irradiance_light_probe(shResult, input->OutputIrradianceTex, 128, 1024, 0.4f);

	CLOVER_VERBOSE("Compute finished");
	return refrain2::Task();
}

}
}
