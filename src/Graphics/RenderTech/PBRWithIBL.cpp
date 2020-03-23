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

static const_cstr s_CubeMapFS = R"(
#version 300 es

layout (location = 0) out mediump vec3 o_Color;

uniform mediump samplerCube u_Tex;
in mediump vec3 v_SampleDir_W;

void main()
{
	mediump vec3 outColor = textureLod(u_Tex, v_SampleDir_W, 0.0).rgb;
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
	, m_NeedBakePMREM(false)
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
	insigne::register_surface_type<SurfaceP>();
	insigne::register_surface_type<SurfaceSkybox>();
	insigne::register_surface_type<SurfacePT>();

	// memory arena
	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(48));

	{
		m_MemoryArena->free_all();
		// ico sphere
		floral::fast_fixed_array<VertexP, LinearArena> sphereVertices;
		floral::fast_fixed_array<s32, LinearArena> sphereIndices;

		sphereVertices.reserve(4096, m_MemoryArena);
		sphereIndices.reserve(8192, m_MemoryArena);

		sphereVertices.resize(4096);
		sphereIndices.resize(8192);

		floral::reset_generation_transforms_stack();
		floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
				2, 0, sizeof(VertexP),
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

			m_SphereVB = insigne::create_vb(desc);
			insigne::copy_update_vb(m_SphereVB, &sphereVertices[0], sphereVertices.get_size(), sizeof(VertexP), 0);
		}

		{
			insigne::ibdesc_t desc;
			desc.region_size = SIZE_KB(16);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_SphereIB = insigne::create_ib(desc);
			insigne::copy_update_ib(m_SphereIB, &sphereIndices[0], sphereIndices.get_size(), 0);
		}
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
		floral::file_info texFile = floral::open_file("gfx/envi/textures/demo/cubeuvchecker.cbskb");
		floral::file_stream dataStream;

		dataStream.buffer = (p8)m_MemoryArena->allocate(texFile.file_size);
		floral::read_all_file(texFile, dataStream);
		floral::close_file(texFile);

		c8 magicChars[4];
		dataStream.read_bytes(magicChars, 4);

		s32 colorRange = 0;
		s32 colorSpace = 0;
		s32 colorChannel = 0;
		f32 encodeGamma = 0.0f;
		s32 mipsCount = 0;
		u32 resolution = 0;
		dataStream.read<s32>(&colorRange);
		dataStream.read<s32>(&colorSpace);
		dataStream.read<s32>(&colorChannel);
		dataStream.read<f32>(&encodeGamma);
		dataStream.read<s32>(&mipsCount);
		//dataStream.read<u32>(&resolution);

		insigne::texture_desc_t demoTexDesc;
		demoTexDesc.width = 512;
		demoTexDesc.height = 512;
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
		m_SceneData.CameraPos = floral::vec4f(6.0f, 6.0f, 6.0f, 0.0f);

		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
	}

	{
		m_Projection.near_plane = 0.01f;
		m_Projection.far_plane = 100.0f;
		m_Projection.fov = 90.0f;
		m_Projection.aspect_ratio = 1.0f;

		m_View.position = floral::vec3f(0.0f, 0.0f, 0.0f);
		m_View.look_at = floral::vec3f(1.0f, 0.0f, 0.0f);
		m_View.up_direction = floral::vec3f(0.0f, 1.0f, 0.0f);

		floral::mat4x4f p = floral::construct_perspective(m_Projection);
		floral::mat4x4f v = floral::construct_lookat_point(m_View);
		m_IBLSceneData.WVP = p * v;
		m_IBLSceneData.CameraPos = floral::vec4f(m_View.position, 0.0f);

		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_IBLBakeSceneUB = insigne::create_ub(desc);
		insigne::copy_update_ub(m_IBLBakeSceneUB, &m_IBLSceneData, sizeof(IBLBakeSceneData), 0);
	}

	m_PrefilterConfigs.roughness = floral::vec2f(0.8f, 0.0f);
	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;
		desc.alignment = 0;

		m_PrefilterUB = insigne::create_ub(desc);
		insigne::copy_update_ub(m_PrefilterUB, &m_PrefilterConfigs, sizeof(PrefilterConfigs), 0);
	}

	// Framebuffer
	{
		calyx::context_attribs* commonCtx = calyx::get_context_attribs();

		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		desc.color_attachments->push_back(insigne::color_attachment_t("main_color", insigne::texture_format_e::hdr_rgb));

		desc.width = commonCtx->window_width;
		desc.height = commonCtx->window_height;

		m_PostFXBuffer = insigne::create_framebuffer(desc);
	}

	{
		insigne::framebuffer_desc_t desc = insigne::create_framebuffer_desc();
		insigne::color_attachment_t atm;
		strcpy(atm.name, "main_color_cube");
		atm.texture_format = insigne::texture_format_e::hdr_rgb_half;
		atm.texture_dimension = insigne::texture_dimension_e::tex_cube;
		desc.color_attachments->push_back(atm);
		desc.width = 256; desc.height = 256;
		desc.has_depth = false;
		desc.color_has_mipmap = true;

		m_SpecularFB = insigne::create_framebuffer(desc);
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
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler_cube));

		strcpy(desc.vs, s_CubeMapVS);
		strcpy(desc.fs, s_CubeMapFS);
		desc.vs_path = floral::path("/demo/cubemap_vs");
		desc.fs_path = floral::path("/demo/cubemap_fs");

		m_CubeMapShader = insigne::create_shader(desc);
		insigne::infuse_material(m_CubeMapShader, m_CubeMapMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_CubeMapMaterial, "ub_Scene");
			m_CubeMapMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_SceneUB };
		}
		{
			s32 texSlot = insigne::get_material_texture_slot(m_CubeMapMaterial, "u_Tex");
			//m_CubeMapMaterial.textures[texSlot].value = insigne::extract_color_attachment(m_SpecularFB, 0);
			m_CubeMapMaterial.textures[texSlot].value = m_CubeMapTex;
		}
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_PrefilterConfigs", insigne::param_data_type_e::param_ub));
		desc.reflection.textures->push_back(insigne::shader_param_t("u_Tex", insigne::param_data_type_e::param_sampler_cube));

		strcpy(desc.vs, s_PMREMVS);
		strcpy(desc.fs, s_PMREMFS);
		desc.vs_path = floral::path("/demo/pmrem_vs");
		desc.fs_path = floral::path("/demo/pmrem_fs");

		m_PMREMShader = insigne::create_shader(desc);
		insigne::infuse_material(m_PMREMShader, m_PMREMMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_PMREMMaterial, "ub_Scene");
			m_PMREMMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_IBLBakeSceneUB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_PMREMMaterial, "ub_PrefilterConfigs");
			m_PMREMMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_PrefilterUB };
		}
		{
			s32 texSlot = insigne::get_material_texture_slot(m_PMREMMaterial, "u_Tex");
			m_PMREMMaterial.textures[texSlot].value = m_CubeMapTex;
		}
	}
}

void PBRWithIBL::OnUpdate(const f32 i_deltaMs)
{
	// DebugUI
	ImGui::Begin("Controller");
	ImGui::Text("ibl");
	if (ImGui::Button("Generate PMREM"))
	{
		m_NeedBakePMREM = true;
	}
	ImGui::End();

	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();

	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(2.0f, 0.0f, 0.0f), floral::vec4f(1.0f, 0.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 2.0f, 0.0f), floral::vec4f(0.0f, 1.0f, 0.0f, 1.0f));
	debugdraw::DrawLine3D(floral::vec3f(0.0f), floral::vec3f(0.0f, 0.0f, 2.0f), floral::vec4f(0.0f, 0.0f, 1.0f, 1.0f));
}

void PBRWithIBL::OnRender(const f32 i_deltaMs)
{
	if (m_NeedBakePMREM)
	{
		for (s32 m = 0; m < 9; m++)
		{
			f32 r = (f32)m / 4.0f;
			m_PrefilterConfigs.roughness = floral::vec2f(r, 0.0f);
			insigne::copy_update_ub(m_PrefilterUB, &m_PrefilterConfigs, sizeof(PrefilterConfigs), 0);
			for (size i = 0; i < 6; i++)
			{
				m_View.look_at = s_lookAts[i];
				m_View.up_direction = s_upDirs[i];
				floral::mat4x4f p = floral::construct_perspective(m_Projection);
				floral::mat4x4f v = floral::construct_lookat_point(m_View);
				m_IBLSceneData.WVP = p * v;
				insigne::copy_update_ub(m_IBLBakeSceneUB, &m_IBLSceneData, sizeof(IBLBakeSceneData), 0);

				insigne::begin_render_pass(m_SpecularFB, s_faces[i], m);
				insigne::draw_surface<SurfaceSkybox>(m_VB, m_IB, m_PMREMMaterial);
				insigne::end_render_pass(m_SpecularFB);
				insigne::dispatch_render_pass();
			}
		}
		m_NeedBakePMREM = false;
	}

	insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
	insigne::begin_render_pass(m_PostFXBuffer);
	insigne::draw_surface<SurfaceP>(m_SphereVB, m_SphereIB, m_CubeMapMaterial);
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

}
}
