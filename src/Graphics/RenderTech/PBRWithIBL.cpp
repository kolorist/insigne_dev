#include "PBRWithIBL.h"

#include <floral/containers/fast_array.h>
#include <floral/comgeo/shapegen.h>

#include <clover/Logger.h>

#include <insigne/system.h>
#include <insigne/ut_render.h>
#include <insigne/ut_buffers.h>
#include <insigne/ut_shading.h>
#include <insigne/ut_textures.h>

#include <calyx/context.h>

#include "InsigneImGui.h"
#include "Graphics/SurfaceDefinitions.h"
#include "Graphics/DebugDrawer.h"
#include "Graphics/CBFormats.h"

// ---------------------------------------------
#include "PBRShaders.inl"
// ---------------------------------------------

namespace stone
{
namespace tech
{

static const_cstr s_CubeMapVS = R"(
#version 300 es
layout (location = 0) in highp vec3 l_Position_L;

layout(std140) uniform ub_Scene
{
	highp mat4 iu_XForm;
	highp mat4 iu_WVP;
	mediump vec4 iu_CameraPos;					// we only use .xyz (w is always 0)
};

out mediump vec3 v_SampleDir_W;

void main() {
	highp vec4 pos_W = vec4(l_Position_L, 1.0f);
	v_SampleDir_W = pos_W.xyz;
	gl_Position = iu_WVP * pos_W;
}
)";

static const_cstr s_CubeMapFS = R"(#version 300 es

layout (location = 0) out mediump vec3 o_Color;

uniform mediump samplerCube u_Tex;
in mediump vec3 v_SampleDir_W;

layout(std140) uniform ub_SH
{
	mediump vec3 iu_SH[9];
};

mediump vec3 eval_sh_irradiance(in mediump vec3 i_normal)
{
	const mediump float c0 = 0.2820947918f;		// sqrt(1.0 / (4.0 * pi))
	const mediump float c1 = 0.4886025119f;		// sqrt(3.0 / (4.0 * pi))
	const mediump float c2 = 1.092548431f;		// sqrt(15.0 / (4.0 * pi))
	const mediump float c3 = 0.3153915653f;		// sqrt(5.0 / (16.0 * pi))
	const mediump float c4 = 0.5462742153f;		// sqrt(15.0 / (16.0 * pi))

	const mediump float a0 = 3.141593f;
	const mediump float a1 = 2.094395f;
	const mediump float a2 = 0.785398f;

	//TODO: we can pre-multiply those above factors with SH coeffs before transfering them
	//to the GPU

	return
		a0 * c0 * iu_SH[0]

		- a1 * c1 * i_normal.x * iu_SH[1]
		+ a1 * c1 * i_normal.y * iu_SH[2]
		- a1 * c1 * i_normal.z * iu_SH[3]

		+ a2 * c2 * i_normal.z * i_normal.x * iu_SH[4]
		- a2 * c2 * i_normal.x * i_normal.y * iu_SH[5]
		+ a2 * c3 * (3.0 * i_normal.y * i_normal.y - 1.0) * iu_SH[6]
		- a2 * c2 * i_normal.y * i_normal.z * iu_SH[7]
		+ a2 * c4 * (i_normal.z * i_normal.z - i_normal.x * i_normal.x) * iu_SH[8];
}

void main()
{
	mediump vec3 outColor = textureLod(u_Tex, v_SampleDir_W, 1.3f).rgb;
	//mediump vec3 outColor = eval_sh_irradiance(normalize(v_SampleDir_W));
	o_Color = outColor;
}
)";

static const floral::vec3f s_lookAts[] =
{
	floral::vec3f(1.0f, 0.0f, 0.0f),
	floral::vec3f(-1.0f, 0.0f, 0.0f),
	floral::vec3f(0.0f, 1.0f, 0.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, 0.0f, 1.0f),
	floral::vec3f(0.0f, 0.0f, -1.0f)
};

static const floral::vec3f s_upDirs[] =
{
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, 0.0f, 1.0f),
	floral::vec3f(0.0f, 0.0f, -1.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f),
	floral::vec3f(0.0f, -1.0f, 0.0f)
};

static const insigne::cubemap_face_e s_faces[] =
{
	insigne::cubemap_face_e::positive_x,
	insigne::cubemap_face_e::negative_x,
	insigne::cubemap_face_e::positive_y,
	insigne::cubemap_face_e::negative_y,
	insigne::cubemap_face_e::positive_z,
	insigne::cubemap_face_e::negative_z
};

static const_cstr k_SuiteName = "pbr with ibl";

PBRWithIBL::PBRWithIBL()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 0.0f, 0.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_MemoryArena(nullptr)
{
}

PBRWithIBL::~PBRWithIBL()
{
}

const_cstr PBRWithIBL::GetName() const
{
	return k_SuiteName;
}

void PBRWithIBL::OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfacePNTBT>();
	insigne::register_surface_type<SurfaceSkybox>();
	insigne::register_surface_type<SurfacePT>();

	// memory arena
	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(48));

	{
		m_MemoryArena->free_all();
		// ico sphere
		floral::fast_fixed_array<VertexPNTBT, LinearArena> vtxData(2048, m_MemoryArena);
		floral::fast_fixed_array<s32, LinearArena> idxData(8192, m_MemoryArena);
		vtxData.resize(2048);
		idxData.resize(8192);

		floral::geo_generate_result_t genResult = floral::generate_unit_uvsphere_3d(
				0, sizeof(VertexPNTBT),
				floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal | floral::geo_vertex_format_e::tex_coord,
				&vtxData[0], &idxData[0]);

		vtxData.resize(genResult.vertices_generated);
		idxData.resize(genResult.indices_generated);

		size facesCount = idxData.get_size() / 3;
		for (size i = 0; i < facesCount; i++)
		{
			s32 idx0 = idxData[3 * i];
			s32 idx1 = idxData[3 * i + 1];
			s32 idx2 = idxData[3 * i + 2];
			floral::vec3f p0 = vtxData[idx0].Position;
			floral::vec3f p1 = vtxData[idx1].Position;
			floral::vec3f p2 = vtxData[idx2].Position;
			floral::vec2f uv0 = vtxData[idx0].TexCoord;
			floral::vec2f uv1 = vtxData[idx1].TexCoord;
			floral::vec2f uv2 = vtxData[idx2].TexCoord;
			floral::vec3f nm = vtxData[idx0].Normal;

			floral::vec3f e0 = p1 - p0;
			floral::vec3f e1 = p2 - p0;
			floral::vec2f deltaUV0 = uv1 - uv0;
			floral::vec2f deltaUV1 = uv2 - uv0;

			f32 f = 1.0f / (deltaUV0.x * deltaUV1.y - deltaUV0.y * deltaUV1.x);

			floral::vec3f tgt, biTgt;
			tgt.x = f * (deltaUV1.y * e0.x - deltaUV0.y * e1.x);
			tgt.y = f * (deltaUV1.y * e0.y - deltaUV0.y * e1.y);
			tgt.z = f * (deltaUV1.y * e0.z - deltaUV0.y * e1.z);
			vtxData[idx0].Tangent = floral::normalize(tgt);
			vtxData[idx1].Tangent = floral::normalize(tgt);
			vtxData[idx2].Tangent = floral::normalize(tgt);

			biTgt.x = f * (-deltaUV1.x * e0.x + deltaUV0.x * e1.x);
			biTgt.y = f * (-deltaUV1.x * e0.y + deltaUV0.x * e1.y);
			biTgt.z = f * (-deltaUV1.x * e0.z + deltaUV0.x * e1.z);
			vtxData[idx0].Binormal = floral::normalize(biTgt);
			vtxData[idx1].Binormal = floral::normalize(biTgt);
			vtxData[idx2].Binormal = floral::normalize(biTgt);
		}

		m_SphereSurface =
			helpers::CreateSurfaceGPU(&vtxData[0], genResult.vertices_generated, sizeof(VertexPNTBT),
					&idxData[0], genResult.indices_generated,
					insigne::buffer_usage_e::static_draw);
	}

	{
		m_MemoryArena->free_all();
		floral::fast_fixed_array<VertexP, LinearArena> vtxData(1024, m_MemoryArena);
		floral::fast_fixed_array<s32, LinearArena> idxData(1024, m_MemoryArena);
		vtxData.resize(1024);
		idxData.resize(1024);

		floral::reset_generation_transforms_stack();

		floral::geo_generate_result_t genResult = floral::generate_unit_box_3d(
				0, sizeof(VertexP),
				floral::geo_vertex_format_e::position,
				&vtxData[0], &idxData[0]);

		vtxData.resize(genResult.vertices_generated);
		idxData.resize(genResult.indices_generated);

		// upload
		{
			insigne::vbdesc_t desc;
			desc.region_size = SIZE_KB(1);
			desc.stride = sizeof(VertexP);
			desc.data = &vtxData[0];
			desc.count = genResult.vertices_generated;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_VB = insigne::copy_create_vb(desc);
		}
		{
			insigne::ibdesc_t desc;
			desc.region_size = SIZE_KB(1);
			desc.data = &idxData[0];
			desc.count = genResult.indices_generated;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_IB = insigne::copy_create_ib(desc);
		}
	}

	{
		m_MemoryArena->free_all();
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/alexs_appartment.prb");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		floral::vec3f sh[9];
		dataStream.read(&sh);
		for (ssize i = 0; i < 9; i++)
		{
			m_SHData.SH[i] = floral::vec4f(sh[i], 0.0f);
		}
		s32 faceSize = 0;
		dataStream.read(&faceSize);

		insigne::texture_desc_t demoTexDesc;
		demoTexDesc.width = faceSize;
		demoTexDesc.height = faceSize;
		demoTexDesc.format = insigne::texture_format_e::hdr_rgb;
		demoTexDesc.min_filter = insigne::filtering_e::linear_mipmap_linear;
		demoTexDesc.mag_filter = insigne::filtering_e::linear;
		demoTexDesc.dimension = insigne::texture_dimension_e::tex_cube;
		demoTexDesc.has_mipmap = true;
		const size dataSize = insigne::prepare_texture_desc(demoTexDesc);
		p8 pData = (p8)demoTexDesc.data;
		// > This is where it get *really* interesting
		// 	Totally opposite of normal 2D texture mapping, CubeMapping define the origin of the texture sampling coordinate
		// 	from the lower left corner. OmegaLUL
		// > Reason: historical reason (from Renderman)
		dataStream.read_bytes((p8)demoTexDesc.data, dataSize);

		m_CubeMapTex = insigne::create_texture(demoTexDesc);

		m_MemoryArena->free_all();
	}

	// UB
	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_SceneUB = insigne::create_ub(desc);

		calyx::context_attribs* commonCtx = calyx::get_context_attribs();
		m_CameraMotion.SetScreenResolution(commonCtx->window_width, commonCtx->window_height);

		m_SceneData.XForm = floral::mat4x4f(1.0f);
		m_SceneData.WVP = m_CameraMotion.GetWVP();
		m_SceneData.CameraPos = floral::vec4f(m_CameraMotion.GetPosition(), 0.0f);

		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
	}

	{
		m_LightData.worldLightDirection = floral::vec4f(1.0f, 1.0f, 1.0f, 0.0f); // normalized in shader
		m_LightData.worldLightDirection = floral::vec4f(2.0f, 2.0f, 2.0f, 0.0f);
		m_LightData.pointLightsCount = 0;

		insigne::ubdesc_t desc;
		desc.region_size = 1024;
		desc.data = &m_LightData;
		desc.data_size = sizeof(LightData);
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_LightUB = insigne::copy_create_ub(desc);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = 2048;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_SHUB = insigne::create_ub(desc);
		insigne::copy_update_ub(m_SHUB, &m_SHData, sizeof(SHData), 0);
	}

	// Framebuffer
	{
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "main_color");
		atm.texture_format = insigne::texture_format_e::hdr_rg;
		atm.texture_dimension = insigne::texture_dimension_e::tex_2d;
		desc.color_attachments->push_back(atm);
		desc.width = 512; desc.height = 512;
		desc.has_depth = false;

		m_BrdfFB = insigne::create_framebuffer(desc);
	}

	{
		calyx::context_attribs* commonCtx = calyx::get_context_attribs();

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));

		desc.width = commonCtx->window_width;
		desc.height = commonCtx->window_height;

		m_PostFXBuffer = insigne::create_framebuffer(desc);
	}

	// Tone Mapping
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

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_BRDF_IntegrationVS);
		strcpy(desc.fs, s_BRDF_IntegrationFS);
		desc.vs_path = floral::path("/demo/brdf_vs");
		desc.fs_path = floral::path("/demo/brdf_fs");

		m_BRDFShader = insigne::create_shader(desc);
		insigne::infuse_material(m_BRDFShader, m_BRDFMaterial);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Light", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_SH", insigne::param_data_type_e::param_ub));

		desc.reflection.textures->push_back(insigne::shader_param_t("u_SpecMap", insigne::param_data_type_e::param_sampler_cube));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_SplitSumLUT", insigne::param_data_type_e::param_sampler2d));

		desc.reflection.textures->push_back(insigne::shader_param_t("u_AlbedoTex", insigne::param_data_type_e::param_sampler2d));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_NormalTex", insigne::param_data_type_e::param_sampler2d));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_AttributeTex", insigne::param_data_type_e::param_sampler2d));

		strcpy(desc.vs, s_PBRWithIBLVS);
		strcpy(desc.fs, s_PBRWithIBLFS);
		desc.vs_path = floral::path("/demo/pbr_vs");
		desc.fs_path = floral::path("/demo/pbr_fs");

		m_PBRShader = insigne::create_shader(desc);
		insigne::infuse_material(m_PBRShader, m_PBRMaterial);

		insigne::helpers::assign_uniform_block(m_PBRMaterial, "ub_Scene", 0, 0, m_SceneUB);
		insigne::helpers::assign_uniform_block(m_PBRMaterial, "ub_Light", 0, 0, m_LightUB);
		insigne::helpers::assign_uniform_block(m_PBRMaterial, "ub_SH", 0, 0, m_SHUB);

		insigne::helpers::assign_texture(m_PBRMaterial, "u_SpecMap", m_CubeMapTex);
		insigne::helpers::assign_texture(m_PBRMaterial, "u_SplitSumLUT", insigne::extract_color_attachment(m_BrdfFB, 0));
	}

	m_BakingBRDF = true;
	m_LoadingTextures = false;
	m_PBRReady = false;
}

void PBRWithIBL::OnUpdate(const f32 i_deltaMs)
{
	// DebugUI
	ImGui::Begin("Controller");
	ImGui::End();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();
	m_SceneData.CameraPos = floral::vec4f(m_CameraMotion.GetPosition(), 0.0f);

	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(2.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 2.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
}

void PBRWithIBL::OnRender(const f32 i_deltaMs)
{
	if (m_BakingBRDF)
	{
		insigne::begin_render_pass(m_BrdfFB);
		insigne::draw_surface<SurfacePT>(m_SSVB, m_SSIB, m_BRDFMaterial);
		insigne::end_render_pass(m_BrdfFB);
		insigne::dispatch_render_pass();
		m_BakingBRDF = false;
		m_LoadingTextures = true;
	}

	if (m_LoadingTextures)
	{
		m_AlbedoTex = _Load2DTexture(floral::path("gfx/go/textures/demo/test_metal_albedo.cbtex"));
		m_AttributeTex = _Load2DTexture(floral::path("gfx/go/textures/demo/test_metal_attribute.cbtex"));
		m_NormalTex = _Load2DTexture(floral::path("gfx/go/textures/demo/test_metal_normal.cbtex"));
		insigne::helpers::assign_texture(m_PBRMaterial, "u_AlbedoTex", m_AlbedoTex);
		insigne::helpers::assign_texture(m_PBRMaterial, "u_AttributeTex", m_AttributeTex);
		insigne::helpers::assign_texture(m_PBRMaterial, "u_NormalTex", m_NormalTex);
		m_LoadingTextures = false;
		m_PBRReady = true;
	}

	if (m_PBRReady)
	{
		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
		insigne::begin_render_pass(m_PostFXBuffer);
		insigne::draw_surface<SurfacePNTBT>(m_SphereSurface.vb, m_SphereSurface.ib, m_PBRMaterial);
		debugdraw::Render(m_SceneData.WVP);
		insigne::end_render_pass(m_PostFXBuffer);
		insigne::dispatch_render_pass();
	}

	insigne::begin_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);
	insigne::draw_surface<SurfacePT>(m_SSVB, m_SSIB, m_ToneMapMaterial);
	RenderImGui();
	insigne::end_render_pass(DEFAULT_FRAMEBUFFER_HANDLE);

	insigne::mark_present_render();
	insigne::dispatch_render_pass();
}

void PBRWithIBL::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	m_MemoryArena = nullptr;
	g_StreammingAllocator.free_all();

	insigne::unregister_surface_type<SurfacePT>();
	insigne::unregister_surface_type<SurfaceSkybox>();
	insigne::unregister_surface_type<SurfaceP>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

// ---------------------------------------------

const insigne::texture_handle_t PBRWithIBL::_Load2DTexture(const floral::path& i_path)
{
	m_MemoryArena->free_all();
	floral::file_info f = floral::open_file(i_path);
	floral::file_stream fs;
	fs.buffer = (p8)m_MemoryArena->allocate(f.file_size);
	floral::read_all_file(f, fs);
	floral::close_file(f);

	cymbi::CBTexture2DHeader header;
	fs.read<cymbi::CBTexture2DHeader>(&header);

	header.resolution = 1u << (header.mipsCount - 1);

	insigne::texture_desc_t desc;
	desc.width = header.resolution;
	desc.height = header.resolution;
	if (header.colorRange == cymbi::ColorRange::LDR)
	{
		if (header.colorChannel == cymbi::ColorChannel::RGBA)
		{
			desc.format = insigne::texture_format_e::rgba;
		}
		else
		{
			desc.format = insigne::texture_format_e::rgb;
		}
	}
	else
	{
		desc.format = insigne::texture_format_e::hdr_rgb;
	}
	desc.min_filter = insigne::filtering_e::linear_mipmap_linear;
	desc.mag_filter = insigne::filtering_e::linear;
	desc.dimension = insigne::texture_dimension_e::tex_2d;
	desc.has_mipmap = true;
	const size dataSize = insigne::prepare_texture_desc(desc);
	p8 pData = (p8)desc.data;
	// > This is where it get interesting
	// > When displaying image in the screen, the usual coordinate origin for us is in upper left corner
	// 	and the data stored in disk *may* in scanlines from top to bottom
	// > But the texture coordinate origin of OpenGL starts from bottom left corner
	// 	and the data stored will be read and displayed in a order from bottom to top
	// Thus, we have to store our texture data in disk in the order of bottom to top scanlines
	fs.read_bytes((p8)desc.data, dataSize);
	return insigne::create_texture(desc);
}

}
}
