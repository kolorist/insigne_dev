#include "PBR.h"

#include <floral/containers/array.h>
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

static const_cstr k_SuiteName = "pbr";

//----------------------------------------------

PBR::PBR()
	: m_CameraMotion(
		floral::camera_view_t { floral::vec3f(6.0f, 6.0f, 6.0f), floral::vec3f(-3.0f, -3.0f, -3.0f), floral::vec3f(0.0f, 1.0f, 0.0f) },
		floral::camera_persp_t { 0.01f, 100.0f, 60.0f, 16.0f / 9.0f })
	, m_MemoryArena(nullptr)
{
}

PBR::~PBR()
{
}

const_cstr PBR::GetName() const
{
	return k_SuiteName;
}

void PBR::OnInitialize()
{
	CLOVER_VERBOSE("Initializing '%s' TestSuite", k_SuiteName);
	// snapshot begin state
	m_BuffersBeginStateId = insigne::get_buffers_resource_state();
	m_ShadingBeginStateId = insigne::get_shading_resource_state();
	m_TextureBeginStateId = insigne::get_textures_resource_state();
	m_RenderBeginStateId = insigne::get_render_resource_state();

	// register surfaces
	insigne::register_surface_type<SurfacePC>();
	insigne::register_surface_type<SurfacePT>();
	insigne::register_surface_type<SurfacePN>();

	// memory arena
	m_MemoryArena = g_StreammingAllocator.allocate_arena<LinearArena>(SIZE_MB(2));

	{
		// ico sphere
		floral::fixed_array<VertexPN, LinearArena> sphereVertices;
		floral::fixed_array<s32, LinearArena> sphereIndices;

		sphereVertices.reserve(4096, m_MemoryArena);
		sphereIndices.reserve(8192, m_MemoryArena);

		sphereVertices.resize(4096);
		sphereIndices.resize(8192);

		floral::reset_generation_transforms_stack();
		floral::geo_generate_result_t genResult = floral::generate_unit_icosphere_3d(
				2, 0, sizeof(VertexPN),
				floral::geo_vertex_format_e::position | floral::geo_vertex_format_e::normal,
				&sphereVertices[0], &sphereIndices[0]);
		sphereVertices.resize(genResult.vertices_generated);
		sphereIndices.resize(genResult.indices_generated);

		{
			insigne::vbdesc_t desc;
			desc.region_size = SIZE_KB(32);
			desc.stride = sizeof(VertexPN);
			desc.data = nullptr;
			desc.count = 0;
			desc.usage = insigne::buffer_usage_e::static_draw;

			m_SphereVB = insigne::create_vb(desc);
			insigne::copy_update_vb(m_SphereVB, &sphereVertices[0], sphereVertices.get_size(), sizeof(VertexPN), 0);
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

	//
	floral::inplace_array<VertexPC, 3> vertices;
	vertices.push_back(VertexPC { { 0.0f, 0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } });
	vertices.push_back(VertexPC { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } });
	vertices.push_back(VertexPC { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } });

	floral::inplace_array<s32, 3> indices;
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);

	{
		insigne::vbdesc_t desc;
		desc.region_size = SIZE_KB(2);
		desc.stride = sizeof(VertexPC);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;
		
		m_VB = insigne::create_vb(desc);
		insigne::copy_update_vb(m_VB, &vertices[0], vertices.get_size(), sizeof(VertexPC), 0);
	}

	{
		insigne::ibdesc_t desc;
		desc.region_size = SIZE_KB(1);
		desc.data = nullptr;
		desc.count = 0;
		desc.usage = insigne::buffer_usage_e::static_draw;

		m_IB = insigne::create_ib(desc);
		insigne::copy_update_ib(m_IB, &indices[0], indices.get_size(), 0);
	}

	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();

		strcpy(desc.vs, s_VertexShaderCode);
		strcpy(desc.fs, s_FragmentShaderCode);
		desc.vs_path = floral::path("/demo/simple_vs");
		desc.fs_path = floral::path("/demo/simple_fs");

		m_Shader = insigne::create_shader(desc);
		insigne::infuse_material(m_Shader, m_Material);
	}

	// UBs
	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_SceneUB = insigne::create_ub(desc);

		m_SceneData.XForm = floral::mat4x4f(1.0f);
		m_SceneData.WVP = m_CameraMotion.GetWVP();
		m_SceneData.CameraPos = floral::vec4f(6.0f, 6.0f, 6.0f, 0.0f);

		insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_LightUB = insigne::create_ub(desc);

		m_LightData.LightDirection = floral::vec4f(6.0f, 6.0f, 0.0f, 0.0f);
		m_LightData.LightIntensity = floral::vec4f(10.0f, 10.0f, 10.0f, 0.0f);

		insigne::copy_update_ub(m_LightUB, &m_LightData, sizeof(LightData), 0);
	}

	{
		insigne::ubdesc_t desc;
		desc.region_size = 256;
		desc.data = nullptr;
		desc.data_size = 0;
		desc.usage = insigne::buffer_usage_e::dynamic_draw;

		m_SurfaceUB = insigne::create_ub(desc);

		m_SurfaceData.BaseColor = floral::vec4f(1.0f, 0.3f, 0.4f, 0.0f);
		m_SurfaceData.Attributes = floral::vec4f(0.4f, 0.5f, 0.0f, 0.0f);

		insigne::copy_update_ub(m_SurfaceUB, &m_SurfaceData, sizeof(SurfaceData), 0);
	}

	// PBR shader
	{
		insigne::shader_desc_t desc = insigne::create_shader_desc();
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Scene", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Light", insigne::param_data_type_e::param_ub));
		desc.reflection.uniform_blocks->push_back(insigne::shader_param_t("ub_Surface", insigne::param_data_type_e::param_ub));

		strcpy(desc.vs, s_PBRVS);
		strcpy(desc.fs, s_PBRFS);
		desc.vs_path = floral::path("/demo/pbr_vs");
		desc.fs_path = floral::path("/demo/pbr_fs");

		m_PBRShader = insigne::create_shader(desc);
		insigne::infuse_material(m_PBRShader, m_PBRMaterial);

		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_PBRMaterial, "ub_Scene");
			m_PBRMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_SceneUB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_PBRMaterial, "ub_Light");
			m_PBRMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_LightUB };
		}
		{
			s32 ubSlot = insigne::get_material_uniform_block_slot(m_PBRMaterial, "ub_Surface");
			m_PBRMaterial.uniform_blocks[ubSlot].value = insigne::ubmat_desc_t { 0, 0, m_SurfaceUB };
		}
	}

	// PBR
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
}

void PBR::OnUpdate(const f32 i_deltaMs)
{
	// Logic update
	m_CameraMotion.OnUpdate(i_deltaMs);
	m_SceneData.WVP = m_CameraMotion.GetWVP();
}

void PBR::OnRender(const f32 i_deltaMs)
{
	insigne::copy_update_ub(m_SceneUB, &m_SceneData, sizeof(SceneData), 0);
	insigne::begin_render_pass(m_PostFXBuffer);
	//insigne::draw_surface<SurfacePC>(m_VB, m_IB, m_Material);
	insigne::draw_surface<SurfacePN>(m_SphereVB, m_SphereIB, m_PBRMaterial);
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

void PBR::OnCleanUp()
{
	CLOVER_VERBOSE("Cleaning up '%s' TestSuite", k_SuiteName);

	m_MemoryArena = nullptr;
	g_StreammingAllocator.free_all();

	insigne::unregister_surface_type<SurfacePN>();
	insigne::unregister_surface_type<SurfacePT>();
	insigne::unregister_surface_type<SurfacePC>();

	insigne::cleanup_render_resource(m_RenderBeginStateId);
	insigne::cleanup_textures_resource(m_TextureBeginStateId);
	insigne::cleanup_shading_resource(m_ShadingBeginStateId);
	insigne::cleanup_buffers_resource(m_BuffersBeginStateId);

	insigne::dispatch_render_pass();
	insigne::wait_finish_dispatching();
}

}
}
